//
// Created by wufan on 2026/9/28.
//
#include  "fivecore/CurlMultiContext.h"

#include <algorithm>
#include <curl/multi.h>

namespace fivecore::http_async
{
    // 单例，懒加载；g_ctx_mutex 同时保护首次创建与显式关闭
    static std::unique_ptr<CurlMultiContext> g_multi_ctx;
    static std::mutex g_ctx_mutex;

    /// @copydoc fivecore::http_async::get_multi_context
    CurlMultiContext* get_multi_context()
    {
        std::lock_guard<std::mutex> lk(g_ctx_mutex);
        if (!g_multi_ctx)
        {
            auto res = CurlMultiContext::Create();
            if (res.is_ok())
            {
                g_multi_ctx = std::move(res.value());
            }
        }
        return g_multi_ctx.get();
    }

    /// @copydoc fivecore::http_async::shutdown_multi_context
    void shutdown_multi_context()
    {
        std::lock_guard<std::mutex> lk(g_ctx_mutex);
        // reset 触发 ~CurlMultiContext → stop()：join IO 线程 + curl_multi_cleanup。
        // io_loop 不会回调 get/shutdown，持锁 reset 不存在重入死锁。
        g_multi_ctx.reset();
    }

    /// @copydoc fivecore::http_async::CurlMultiContext::set_finish_hook
    void CurlMultiContext::set_finish_hook(FinishHook hook)
    {
        std::lock_guard<std::mutex> lk(mtx_queue_);
        finish_hook_ = std::move(hook);
    }

    /// @brief 任务结束统一出口：先广播 hook（调度器记账/回填），再调用任务自身 on_finish
    /// @note  锁顺序：本函数短暂持有 mtx_queue_ 拷贝 hook 后即释放，再执行 hook 与 on_finish，
    ///        因此 hook/on_finish 内部可以安全地回调 submit_task 等接口，无重入死锁
    void CurlMultiContext::finish_task(INetTask* task, CURLcode code, long http_code)
    {
        FinishHook hook;
        {
            std::lock_guard<std::mutex> lk(mtx_queue_);
            hook = finish_hook_;
        }
        if (hook)
        {
            hook(task, code, http_code);
        }
        task->on_finish(code, http_code);
    }

    CurlMultiContext::CurlMultiContext()
    {
    }

    /// @copydoc fivecore::http_async::CurlMultiContext::Create
    Result<std::unique_ptr<CurlMultiContext>> CurlMultiContext::Create()
    {
        // Step1：创建上下文实例（私有构造，不会失败）
        auto ctx = std::unique_ptr<CurlMultiContext>(new CurlMultiContext());

        // Step2：尝试创建multi句柄
        ctx->multi_handle_ = curl_multi_init();
        if (!ctx->multi_handle_)
        {
            // 资源创建失败，unique_ptr会自动销毁ctx，返回错误Result
            return Result<std::unique_ptr<CurlMultiContext>>::Err(ErrorInfo::make(ErrorCode::CurlMultiInitFailed, "curl_multi_init return nullptr"));
        }

        // Step3：multi句柄成功，启动IO线程
        ctx->io_thread_ = std::thread(&CurlMultiContext::io_loop, ctx.get());

        // Step4：全部成功，返回ctx
        return Result<std::unique_ptr<CurlMultiContext>>::Ok(std::move(ctx));
    }

    /// @copydoc fivecore::http_async::CurlMultiContext::~CurlMultiContext
    CurlMultiContext::~CurlMultiContext()
    {
        stop();
    }

    /// @copydoc fivecore::http_async::CurlMultiContext::submit_task
    VoidResult CurlMultiContext::submit_task(std::shared_ptr<INetTask> task)
    {
        // 第一步：检查上下文是否已经停止
        if (stop_flag_.load())
        {
            return VoidResult::Err(ErrorInfo::make(ErrorCode::ContextStopped, "CurlMultiContext已经停止，不能提交新任务"));
        }

        // 第二步：锁保护，入队
        std::lock_guard<std::mutex> lk(mtx_queue_);
        pending_tasks_.push(std::move(task));
        return OkVoid();
    }

    /// @copydoc fivecore::http_async::CurlMultiContext::stop
    VoidResult CurlMultiContext::stop()
    {
        if (stop_flag_.load())
        {
            return OkVoid();
        }
        stop_flag_ = true;
        if (io_thread_.joinable())
        {
            io_thread_.join();
        }

        // multi_handle_仍然有效，先摘除所有active任务
        if(multi_handle_)
        {
            for(auto& pair : active_tasks_)
            {
                auto& task = pair.second;
                curl_multi_remove_handle(multi_handle_, task->easy_);
                // 统一结束出口：stop 中止路径同样广播 hook（调度器记账）
                finish_task(task.get(), CURLE_ABORTED_BY_CALLBACK, 0L);
            }
            active_tasks_.clear();

            // 全部摘除完成，再销毁multi
            curl_multi_cleanup(multi_handle_);
            multi_handle_ = nullptr;
        }

        return OkVoid();
    }

    void CurlMultiContext::io_loop()
    {
        while (!stop_flag_.load())
        {
            // std::queue<std::shared_ptr<INetTask>> local_tasks;
            std::queue<std::shared_ptr<INetTask>> local_tasks;
            {
                std::lock_guard<std::mutex> lock(mtx_queue_);
                if (pending_tasks_.empty() && stop_flag_.load())
                {
                    break;
                }
                std::swap(local_tasks,pending_tasks_);
            }
            int runnings;


            while (!local_tasks.empty())
            {
                auto task = local_tasks.front();
                local_tasks.pop();

                // 提交前已被取消的任务：跳过网络层，直接走结束出口
                if (task->is_cancelled_.load())
                {
                    finish_task(task.get(), CURLE_ABORTED_BY_CALLBACK, 0L);
                    continue;
                }

                if (!task->setup_easy())
                {
                    // setup失败，直接回调结束，不加入multi
                    finish_task(task.get(), CURLE_FAILED_INIT, 0L);
                    continue;
                }
                // setup成功，加入multi
                auto add_ret = curl_multi_add_handle(multi_handle_, task->easy_);
                if(add_ret != CURLM_OK)
                {
                    finish_task(task.get(), CURLE_FAILED_INIT, 0L);
                    continue;
                }
                active_tasks_.insert({task.get(),task});
            }

            auto mcode = curl_multi_perform(multi_handle_,&runnings);
            if (mcode != CURLM_OK)
            {
                // multi全局出错，记录日志
                break;
            }

            // 读取已完成请求消息
            int msg_count = 0;
            CURLMsg* msg = nullptr;
            while ((msg = curl_multi_info_read(multi_handle_, &msg_count)) != nullptr)
            {
                if (msg->msg == CURLMSG_DONE)
                {
                    CURL* easy = msg->easy_handle;
                    INetTask* task_ptr = nullptr;
                    //约定：任务创建方需要将this指针赋值给PRIVATE指针
                    curl_easy_getinfo(easy, CURLINFO_PRIVATE, &task_ptr);
                    auto it = active_tasks_.find(task_ptr);
                    if (it != active_tasks_.end())
                    {
                        // 先摘除easy，脱离multi
                        curl_multi_remove_handle(multi_handle_, easy);

                        long http_code = 0;
                        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &http_code);
                        // 统一结束出口：hook + on_finish（map 中的 shared_ptr 保活）
                        finish_task(task_ptr, msg->data.result, http_code);

                        // 从活跃任务表移除，shared_ptr引用计数减一
                        active_tasks_.erase(it);
                    }
                }
            }

            // 取消清扫：强制摘除已取消的任务（协作式取消的兜底）。
            // 若任务卡在连接/等待响应阶段，读写回调不再触发，仅靠任务自身
            // 检查 is_cancelled_ 无法中止；由 IO 线程在此统一摘除，
            // 保证 cancel() 与超时取消最迟在下个 tick（~10ms）内生效
            for (auto it = active_tasks_.begin(); it != active_tasks_.end(); )
            {
                if (it->second->is_cancelled_.load())
                {
                    auto task = it->second;          // 先持有 shared_ptr，防止 finish 期间析构
                    INetTask* raw = task.get();
                    curl_multi_remove_handle(multi_handle_, raw->easy_);
                    long http_code = 0;
                    curl_easy_getinfo(raw->easy_, CURLINFO_RESPONSE_CODE, &http_code);
                    it = active_tasks_.erase(it);
                    finish_task(raw, CURLE_ABORTED_BY_CALLBACK, http_code);
                }
                else
                {
                    ++it;
                }
            }


            int fds = 0;
            //旧：
            // curl_multi_wait(multi_handle_, nullptr, 0, 10, &fds);
            // 新（curl 7.66+）：
            curl_multi_poll(multi_handle_, nullptr, 0, 10, &fds);
        }
    }
}
