
#include "fivecore/TaskScheduler.h"

#include <algorithm>

namespace fivecore::scheduler
{
    /// @copydoc fivecore::scheduler::TaskScheduler::Create
    TaskScheduler::TaskScheduler(CurlMultiContext* ctx, const SchedulerConfig& cfg)
        : ctx_(ctx), cfg_(cfg), tick_(cfg.timer_tick)
    {
    }

    /// @copydoc fivecore::scheduler::TaskScheduler::Create
    Result<std::unique_ptr<TaskScheduler>> TaskScheduler::Create(
        CurlMultiContext* ctx, const SchedulerConfig& cfg)
    {
        if (!ctx)
        {
            return Result<std::unique_ptr<TaskScheduler>>::Err(
                ErrorInfo::make(ErrorCode::CurlRuntimeError, "TaskScheduler::Create: ctx is null"));
        }
        if (cfg.max_concurrency == 0)
        {
            return Result<std::unique_ptr<TaskScheduler>>::Err(
                ErrorInfo::make(ErrorCode::CurlRuntimeError, "TaskScheduler::Create: max_concurrency must be > 0"));
        }

        auto sched = std::unique_ptr<TaskScheduler>(new TaskScheduler(ctx, cfg));

        // 注册完成广播：所有任务结束路径先记账/回填，再调用任务自身 on_finish
        ctx->set_finish_hook([raw = sched.get()](INetTask* task, CURLcode code, long http_code)
        {
            raw->on_task_finished(task, code, http_code);
        });

        // 启动 timer 线程（超时巡逻）
        sched->timer_thread_ = std::thread(&TaskScheduler::timer_loop, sched.get());

        return Result<std::unique_ptr<TaskScheduler>>::Ok(std::move(sched));
    }

    /// @copydoc fivecore::scheduler::TaskScheduler::~TaskScheduler
    TaskScheduler::~TaskScheduler()
    {
        stop();
        // 注销完成 hook（生命周期约定：ctx 后于调度器析构）
        ctx_->set_finish_hook(nullptr);
    }

    /// @copydoc fivecore::scheduler::TaskScheduler::submit
    Result<TaskId> TaskScheduler::submit(std::shared_ptr<INetTask> task,
                                         CompletionCb on_complete,
                                         std::optional<std::chrono::milliseconds> timeout)
    {
        if (!task)
        {
            return Result<TaskId>::Err(
                ErrorInfo::make(ErrorCode::CurlRuntimeError, "TaskScheduler::submit: null task"));
        }

        std::vector<DeferredCompletion> deferred;
        TaskId id = kInvalidTaskId;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (stop_flag_.load())
            {
                return Result<TaskId>::Err(
                    ErrorInfo::make(ErrorCode::ContextStopped, "TaskScheduler::submit: scheduler stopped"));
            }

            id = next_id_++;
            ++stats_.submitted;

            TrackedTask tt;
            tt.id = id;
            tt.task = std::move(task);
            tt.on_complete = std::move(on_complete);
            tt.submit_time = std::chrono::steady_clock::now();
            tt.timeout = timeout.value_or(cfg_.default_timeout);

            waiting_.push_back(std::move(tt));
            // 有空闲并发额度则立即下发；无法入网络层的任务收集到 deferred
            launch_next_locked(deferred);
        }
        for (auto& d : deferred)
        {
            d.first(d.second);
        }
        return Result<TaskId>::Ok(id);
    }

    /// @copydoc fivecore::scheduler::TaskScheduler::cancel
    VoidResult TaskScheduler::cancel(TaskId id)
    {
        if (id == kInvalidTaskId)
        {
            return VoidResult::Err(
                ErrorInfo::make(ErrorCode::CurlRuntimeError, "TaskScheduler::cancel: invalid task id"));
        }

        std::shared_ptr<INetTask> running_task;   // 命中运行表时置位
        DeferredCompletion deferred;              // 命中等待队列时置位
        bool queued_removed = false;

        {
            std::lock_guard<std::mutex> lk(mtx_);

            // 先查等待队列：排队任务从未进入网络层，直接移除
            auto it = std::find_if(waiting_.begin(), waiting_.end(),
                                   [id](const TrackedTask& t) { return t.id == id; });
            if (it != waiting_.end())
            {
                ++stats_.cancelled;
                deferred.second.id = id;
                deferred.second.reason = FinishReason::Cancelled;
                deferred.second.queue_wait = std::chrono::steady_clock::now() - it->submit_time;
                deferred.first = std::move(it->on_complete);
                waiting_.erase(it);
                queued_removed = true;
            }
            else
            {
                // 再查运行表：置标记后协作式取消，由 IO 线程的取消清扫摘除
                auto rit = std::find_if(running_.begin(), running_.end(),
                                        [id](const auto& kv) { return kv.second.id == id; });
                if (rit != running_.end())
                {
                    rit->second.user_cancelled = true;
                    running_task = rit->second.task;
                }
            }
        }

        if (queued_removed)
        {
            // 排队取消的回调在本线程同步触发
            if (deferred.first)
            {
                deferred.first(deferred.second);
            }
            return OkVoid();
        }
        if (running_task)
        {
            // 协作式取消：置原子标记即可，真正的摘除由 CurlMultiContext 完成；
            // 完成回调稍后经 hook 以 Cancelled 触发
            running_task->cancel();
            return OkVoid();
        }

        return VoidResult::Err(
            ErrorInfo::make(ErrorCode::CurlRuntimeError, "TaskScheduler::cancel: task not found or already finished"));
    }

    /// @copydoc fivecore::scheduler::TaskScheduler::set_concurrency_limit
    VoidResult TaskScheduler::set_concurrency_limit(std::size_t max_concurrency)
    {
        if (max_concurrency == 0)
        {
            return VoidResult::Err(
                ErrorInfo::make(ErrorCode::CurlRuntimeError, "TaskScheduler::set_concurrency_limit: must be > 0"));
        }

        std::vector<DeferredCompletion> deferred;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            cfg_.max_concurrency = max_concurrency;
            // 上调后立即尝试回填；下调仅影响后续回填判断，不中止已运行任务
            launch_next_locked(deferred);
        }
        for (auto& d : deferred)
        {
            d.first(d.second);
        }
        return OkVoid();
    }

    /// @copydoc fivecore::scheduler::TaskScheduler::stats
    SchedulerStats TaskScheduler::stats() const
    {
        std::lock_guard<std::mutex> lk(mtx_);
        SchedulerStats s = stats_;
        s.waiting = waiting_.size();
        s.running = running_.size();
        return s;
    }

    /// @copydoc fivecore::scheduler::TaskScheduler::stop
    VoidResult TaskScheduler::stop()
    {
        std::vector<DeferredCompletion> deferred;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (stop_flag_.load())
            {
                return OkVoid(); // 可重复调用
            }
            stop_flag_ = true;
            timer_cv_.notify_all();

            // 排队中任务：直接以 Cancelled 结束
            const auto now = std::chrono::steady_clock::now();
            while (!waiting_.empty())
            {
                TrackedTask tt = std::move(waiting_.front());
                waiting_.pop_front();
                ++stats_.cancelled;

                TaskCompletion tc;
                tc.id = tt.id;
                tc.reason = FinishReason::Cancelled;
                tc.queue_wait = now - tt.submit_time;
                if (tt.on_complete)
                {
                    deferred.emplace_back(std::move(tt.on_complete), std::move(tc));
                }
            }
            // 运行中任务：保留在 running_，等待 CurlMultiContext::stop 中止时
            // 经 hook 记账并触发回调；若业务随后直接销毁 ctx 而未调用 ctx->stop()，
            // 这些任务的完成回调不再触发（引用随本对象析构释放）
        }
        if (timer_thread_.joinable())
        {
            timer_thread_.join();
        }
        for (auto& d : deferred)
        {
            d.first(d.second);
        }
        return OkVoid();
    }

    /// @brief timer 线程主循环：周期巡逻运行表，超时任务置标记并协作式 cancel
    void TaskScheduler::timer_loop()
    {
        std::unique_lock<std::mutex> lk(mtx_);
        while (!stop_flag_.load())
        {
            timer_cv_.wait_for(lk, tick_);
            if (stop_flag_.load())
            {
                break;
            }

            const auto now = std::chrono::steady_clock::now();
            std::vector<std::shared_ptr<INetTask>> expired;
            for (auto& kv : running_)
            {
                auto& tt = kv.second;
                if (!tt.timed_out && !tt.user_cancelled && now >= tt.deadline)
                {
                    tt.timed_out = true;
                    expired.push_back(tt.task);
                }
            }

            // 解锁后再 cancel：任务实现可能持有自身内部锁，避免锁序交叉
            lk.unlock();
            for (auto& t : expired)
            {
                t->cancel();
            }
            // cancel 期间 stop() 可能已设置 stop_flag_ 并释放锁；
            // 重检避免无意义地重新竞争锁再进循环体
            if (stop_flag_.load())
            {
                break;
            }
            lk.lock();
        }
    }

    /// @brief 完成回调 hook（IO 线程触发）：记账 + 回填，之后任务自身 on_finish 才被调用
    void TaskScheduler::on_task_finished(INetTask* task, CURLcode code, long http_code)
    {
        std::vector<DeferredCompletion> deferred;
        CompletionCb cb;
        TaskCompletion tc;
        {
            std::lock_guard<std::mutex> lk(mtx_);

            auto it = running_.find(task);
            if (it == running_.end())
            {
                // 非本调度器管理的任务（业务直提交到 ctx），hook 放行不处理
                return;
            }
            TrackedTask tt = std::move(it->second);
            running_.erase(it);

            const auto now = std::chrono::steady_clock::now();
            tc.id = tt.id;
            tc.curl_code = code;
            tc.http_code = http_code;
            tc.queue_wait = tt.start_time - tt.submit_time;
            tc.run_time = now - tt.start_time;

            // 结束原因判定：用户取消 > 超时 > 传输结果
            if (tt.user_cancelled)      { tc.reason = FinishReason::Cancelled; ++stats_.cancelled; }
            else if (tt.timed_out)      { tc.reason = FinishReason::TimedOut;  ++stats_.timed_out; }
            else if (code == CURLE_OK)  { tc.reason = FinishReason::Succeeded; ++stats_.succeeded; }
            else                        { tc.reason = FinishReason::Failed;    ++stats_.failed; }

            // 同锁域回填：检查-决定-写入一次完成，防止并发超发
            launch_next_locked(deferred);

            cb = std::move(tt.on_complete);
        }
        // 解锁后触发用户回调，允许回调内重入 submit/cancel 等接口
        if (cb)
        {
            cb(tc);
        }
        for (auto& d : deferred)
        {
            d.first(d.second);
        }
    }

    /// @brief 把队首任务送入网络层（必须持锁调用；检查-决定-写入同锁域，防止超发）
    void TaskScheduler::launch_next_locked(std::vector<DeferredCompletion>& deferred)
    {
        while (!waiting_.empty() && running_.size() < cfg_.max_concurrency)
        {
            TrackedTask tt = std::move(waiting_.front());
            waiting_.pop_front();

            tt.start_time = std::chrono::steady_clock::now();
            tt.deadline = tt.start_time + tt.timeout;
            stats_.total_queue_wait_ms +=
                static_cast<std::uint64_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        tt.start_time - tt.submit_time).count());

            auto sub_res = ctx_->submit_task(tt.task);
            if (sub_res.is_err())
            {
                // 网络层已停止等场景：任务无法进入网络层，按 Failed 结束并继续排空
                ++stats_.failed;
                TaskCompletion tc;
                tc.id = tt.id;
                tc.reason = FinishReason::Failed;
                tc.curl_code = CURLE_FAILED_INIT;
                tc.queue_wait = tt.start_time - tt.submit_time;
                if (tt.on_complete)
                {
                    deferred.emplace_back(std::move(tt.on_complete), std::move(tc));
                }
                continue;
            }
            running_.emplace(tt.task.get(), std::move(tt));
        }
    }

} // namespace fivecore::scheduler
