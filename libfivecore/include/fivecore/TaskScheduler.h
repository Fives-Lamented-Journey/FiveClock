//
// Created by wufan on 2026/10/4.
//

/**
 * @file TaskScheduler.h
 * @brief 简易任务调度器：并发上限 + FIFO 排队 + 全局超时 + 取消
 *
 * 基于 CurlMultiContext（组合关系，不持有所有权）之上的一层限流调度，
 * 解决大量备份任务瞬间提交时同时发起过多 HTTP 请求的问题。
 *
 * 线程模型：
 * - 单一互斥锁保护 等待队列 / 运行表 / 统计 全部状态（单锁原则，无锁序环）
 * - 业务线程：submit / cancel / stats / stop
 * - IO 线程（CurlMultiContext）：完成 hook 记账 + 回填下一个任务
 * - 独立 timer 线程：按 tick 轮询运行表，超时任务协作式 cancel
 *
 * 回调线程边界：
 * - CompletionCb 在 CurlMultiContext 的 IO 线程触发（与 INetTask::on_finish 同线程）
 * - 例外：取消"排队中"任务的回调在调用者线程同步触发
 * - Qt 业务侧如需更新 UI，须自行投递（QueuedConnection / invokeMethod）
 *
 * 生命周期约定：CurlMultiContext 必须先于 TaskScheduler 构造、后于其析构
 * （调度器析构时会向 ctx 注销完成 hook）。
 */

#ifndef LIBFIVECORE_TASKSCHEDULER_H
#define LIBFIVECORE_TASKSCHEDULER_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <curl/curl.h>

#include "fivecore/CurlMultiContext.h"  // CurlMultiContext 完整类型（INetTask.h 仅含 friend 声明）
#include "fivecore/result.h"

namespace fivecore::scheduler
{
    using http_async::CurlMultiContext;
    using http_async::INetTask;

    /// 任务 ID（0 为非法值，自增从 1 开始）
    using TaskId = std::uint64_t;
    /// 非法任务 ID 常量
    constexpr TaskId kInvalidTaskId = 0;

    /// 任务结束原因
    enum class FinishReason
    {
        Succeeded,  ///< 传输层成功（curl_code == CURLE_OK；HTTP 业务语义由任务自身回调判定）
        Failed,     ///< 传输层失败（含无法进入网络层的场景）
        Cancelled,  ///< 用户取消
        TimedOut    ///< 全局超时，被调度器强制取消
    };

    /// 单个任务的完成信息（CompletionCb 的参数）
    struct TaskCompletion
    {
        TaskId id = kInvalidTaskId;                          ///< submit 返回的任务 ID
        FinishReason reason = FinishReason::Failed;          ///< 结束原因
        CURLcode curl_code = CURLE_OK;                       ///< curl 结果码（Cancelled/TimedOut 时仅为参考）
        long http_code = 0;                                  ///< HTTP 状态码（可能为 0）
        std::chrono::steady_clock::duration queue_wait{};    ///< 排队耗时（提交→实际进入网络层）
        std::chrono::steady_clock::duration run_time{};      ///< 执行耗时（进入网络层→完成）
    };

    /// 任务完成回调
    using CompletionCb = std::function<void(const TaskCompletion&)>;

    /// 运行统计快照（埋点，供 GUI 展示）
    struct SchedulerStats
    {
        std::uint64_t submitted = 0;           ///< 累计提交数
        std::uint64_t succeeded = 0;           ///< 累计成功数
        std::uint64_t failed = 0;              ///< 累计失败数
        std::uint64_t cancelled = 0;           ///< 累计用户取消数
        std::uint64_t timed_out = 0;           ///< 累计超时数
        std::uint64_t total_queue_wait_ms = 0; ///< 累计排队耗时（ms），配合 submitted 求均值
        std::uint64_t bytes_transferred = 0;   ///< 预留：累计传输字节数（需任务上报，后续接入）
        std::size_t waiting = 0;               ///< 当前排队数（快照）
        std::size_t running = 0;               ///< 当前运行数（快照）
    };

    /// 调度器配置
    struct SchedulerConfig
    {
        std::size_t max_concurrency = 4;                            ///< 最大并发数
        std::chrono::milliseconds default_timeout{60000};           ///< 默认全局超时
        std::chrono::milliseconds timer_tick{100};                  ///< timer 线程轮询间隔
    };

    /**
     * @brief 简易任务调度器
     *
     * 提交逻辑：running < max_concurrency → 直接送入 CurlMultiContext；
     * 否则进入 FIFO 等待队列（无界），任一任务完成后自动回填队首。
     * 超时：运行超过 deadline 的任务被协作式 cancel()（复用 INetTask 原子标记 +
     * CurlMultiContext 的取消清扫），完成回调以 TimedOut 结束。
     *
     * 与 CurlMultiContext 的对接：通过 set_finish_hook 注册完成广播，
     * io_loop 所有任务结束路径先触发 hook（记账 + 回填），再调用任务自身回调。
     */
    class TaskScheduler
    {
    public:
        /**
         * @brief 创建调度器并启动 timer 线程
         * @param ctx 外部注入的 CurlMultiContext（组合关系，须保证先于调度器存活；
         *            一个 ctx 至多挂一个调度器，重复注册会覆盖先前 hook）
         * @param cfg 配置（并发上限 / 默认超时 / timer tick）
         * @return Ok(unique_ptr)；Err(ctx 为 nullptr / max_concurrency 为 0)
         */
        static Result<std::unique_ptr<TaskScheduler>> Create(
            CurlMultiContext* ctx, const SchedulerConfig& cfg = {});

        /// 析构：自动 stop() 并向 ctx 注销完成 hook
        ~TaskScheduler();

        TaskScheduler(const TaskScheduler&) = delete;             ///< 禁止拷贝
        TaskScheduler& operator=(const TaskScheduler&) = delete;  ///< 禁止拷贝赋值
        TaskScheduler(TaskScheduler&&) = delete;                  ///< 禁止移动
        TaskScheduler& operator=(TaskScheduler&&) = delete;       ///< 禁止移动赋值

        /**
         * @brief 提交任务（线程安全）
         *
         * 有空闲并发额度则立即送入网络层，否则进入 FIFO 等待队列。
         * @param task 用户任务（其自身 finish_cb 负责业务结果处理，与本回调互不干扰）
         * @param on_complete 完成回调（IO 线程触发；可为空）
         * @param timeout 单任务超时覆盖；nullopt 时使用 SchedulerConfig::default_timeout
         * @return Ok(TaskId) 用于 cancel；Err(task 为空 / 调度器已 stop)
         */
        Result<TaskId> submit(std::shared_ptr<INetTask> task,
                              CompletionCb on_complete = nullptr,
                              std::optional<std::chrono::milliseconds> timeout = std::nullopt);

        /**
         * @brief 取消任务（线程安全，可重复调用）
         *
         * 排队中：直接移出队列，回调以 Cancelled 在调用者线程同步触发。
         * 运行中：置取消标记并协作式 cancel，回调以 Cancelled 在 IO 线程触发。
         * @return Ok(已受理)；Err(ID 非法 / 不存在 / 已完成)
         */
        VoidResult cancel(TaskId id);

        /// 动态调整并发上限（上调立即回填；下调仅影响后续判断，不中止已运行任务）
        VoidResult set_concurrency_limit(std::size_t max_concurrency);

        /// 统计快照（线程安全）
        SchedulerStats stats() const;

        /**
         * @brief 停止调度器并 join timer 线程
         *
         * 排队中任务以 Cancelled 结束；运行中任务交由 CurlMultiContext::stop 中止
         * （届时经 hook 记账，完成回调以 Cancelled/Failed 触发）。
         * 之后 submit 恒返 Err。可重复调用。
         */
        VoidResult stop();

    private:
        /// 延迟触发的完成回调（先在锁内收集，解锁后统一触发，避免用户回调重入死锁）
        using DeferredCompletion = std::pair<CompletionCb, TaskCompletion>;

        TaskScheduler(CurlMultiContext* ctx, const SchedulerConfig& cfg);

        /// 调度器内部记账条目（不对业务暴露）
        struct TrackedTask
        {
            TaskId id = kInvalidTaskId;                          ///< submit 分配的任务 ID
            std::shared_ptr<INetTask> task;                      ///< 用户任务（保活）
            CompletionCb on_complete;                            ///< 用户完成回调
            std::chrono::milliseconds timeout{};                 ///< 本任务生效的超时值
            std::chrono::steady_clock::time_point submit_time{}; ///< 入调度器时刻
            std::chrono::steady_clock::time_point start_time{};  ///< 进入网络层时刻
            std::chrono::steady_clock::time_point deadline{};    ///< start_time + timeout
            bool user_cancelled = false;                         ///< 用户取消标记
            bool timed_out = false;                              ///< 超时取消标记
        };

        /// timer 线程主循环：tick 轮询运行表，超时任务置标记并协作式 cancel
        void timer_loop();

        /// 完成回调 hook（IO 线程触发）：记账 + 回填，之后任务自身 on_finish 才被调用
        void on_task_finished(INetTask* task, CURLcode code, long http_code);

        /// 把队首任务送入网络层（必须持锁调用；检查-决定-写入同锁域，防止超发）。
        /// 无法进入网络层的任务按 Failed 收集到 deferred，由调用方解锁后触发回调
        void launch_next_locked(std::vector<DeferredCompletion>& deferred);

        CurlMultiContext* ctx_;                  ///< 外部注入，不持有所有权
        SchedulerConfig cfg_;                    ///< 可变配置（并发上限），受 mtx_ 保护
        std::chrono::milliseconds tick_;         ///< timer 轮询间隔（构造后不变）

        mutable std::mutex mtx_;                 ///< 唯一状态锁（等待/运行/统计/ID）
        std::deque<TrackedTask> waiting_;        ///< FIFO 等待队列
        std::unordered_map<INetTask*, TrackedTask> running_; ///< 运行表（key 为任务裸指针，与 hook 对齐）
        TaskId next_id_ = 1;                     ///< 任务 ID 分配器
        SchedulerStats stats_;                   ///< 累计统计

        std::thread timer_thread_;               ///< 独立超时检测线程
        std::condition_variable timer_cv_;       ///< stop 时提前唤醒 timer
        std::atomic<bool> stop_flag_{false};     ///< 停止标记
    };

} // namespace fivecore::scheduler

#endif //LIBFIVECORE_TASKSCHEDULER_H
