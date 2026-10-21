//
// Created by wufan on 2026/9/28.
//

/**
 * @file CurlMultiContext.h
 * @brief 基于 curl_multi 的异步 IO 多路复用上下文
 *
 * 内部持有一个独立 IO 线程，通过 curl_multi 驱动所有异步 HTTP 任务。
 * 业务线程通过 submit_task() 提交任务，任务完成后回调在 IO 线程中触发。
 *
 * @todo 改进：easy句柄支持
 */

#ifndef LIBFIVECORE_CURLMULTICONTEXT_H
#define LIBFIVECORE_CURLMULTICONTEXT_H
#pragma once

#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include <unordered_map>

#include "fivecore/INetTask.h"

namespace fivecore::http_async
{

    /**
     * @brief curl_multi 异步调度上下文
     *
     * 单 IO 线程 + curl_multi 驱动所有并发任务
     * pending_tasks_ 队列：业务线程提交的待启动任务
     * active_tasks_ map：正在 multi handle 上执行的任务（key 为裸指针，value 为 shared_ptr，
     * 保证任务在执行期间不被提前析构）
     *
     * 使用方式：通过 get_multi_context() 获取全局单例，调用 submit_task() 提交任务。
     */
    class CurlMultiContext
    {
    public:
        /**
         * @brief 创建一个 CurlMultiContext 实例（同时启动 IO 线程）
         * @return Ok(unique_ptr) 成功；Err(CurlMultiInitFailed) multi 句柄创建失败
         */
        static Result<std::unique_ptr<CurlMultiContext>> Create();

        /**
         * @brief 析构：自动调用 stop() 停止 IO 线程
         * @todo 缺陷：存在微妙竞态，pendingTask可能有残留提交任务
         */
        ~CurlMultiContext();

        /// 禁止拷贝
        CurlMultiContext(const CurlMultiContext&) = delete;
        /// 禁止拷贝赋值
        CurlMultiContext& operator=(const CurlMultiContext&) = delete;
        /// 禁止移动
        CurlMultiContext(CurlMultiContext&&) = delete;
        /// 禁止移动赋值
        CurlMultiContext& operator=(CurlMultiContext&&) = delete;

        /**
         * @brief 提交任务到待执行队列（业务线程调用，线程安全）
         * @param task 待执行的任务 shared_ptr
         * @return VoidResult，Ok 表示已入队
         */
        VoidResult submit_task(std::shared_ptr<INetTask> task);

        /**
         * @brief 停止 IO 线程并等待 join
         *
         * 会中止所有进行中的任务（触发 on_finish），析构时自动调用。
         */
        VoidResult stop();

        /// 任务完成 hook 类型：task 为结束的任务裸指针（调用方保证本次调用期间存活），
        /// code / http_code 即随后传给该任务 on_finish 的参数
        using FinishHook = std::function<void(INetTask* task, CURLcode code, long http_code)>;

        /**
         * @brief 注册任务完成 hook（供上层调度器记账与回填，至多一个，重复注册覆盖）
         *
         * 所有任务结束路径（setup 失败 / add 失败 / 传输完成 / 取消清扫 / stop 中止）
         * 均先在 IO 线程触发 hook，再调用任务自身的 on_finish。
         * 传 nullptr 注销。线程安全（IO 线程读取时短暂加锁拷贝）。
         * 约定：一个 CurlMultiContext 至多挂一个 hook 拥有者。
         */
        void set_finish_hook(FinishHook hook);

    private:
        CurlMultiContext();

        /// IO 线程主循环：消费 pending 队列 → curl_multi_perform → 处理完成消息
        void io_loop();

        CURLM* multi_handle_{nullptr};                                     ///< curl_multi 句柄
        std::queue<std::shared_ptr<INetTask>> pending_tasks_;              ///< 待执行任务队列
        std::unordered_map<INetTask*, std::shared_ptr<INetTask>> active_tasks_; ///< 执行中任务（持有 shared_ptr 保活）
        std::mutex mtx_queue_;                                             ///< pending / active 容器的互斥锁

        std::atomic<bool> stop_flag_{false}; ///< IO 线程停止标记
        std::thread io_thread_;              ///< IO 线程

        FinishHook finish_hook_;             ///< 完成广播 hook（受 mtx_queue_ 保护）

        /// 任务结束统一出口：先触发 finish_hook_，再调用 task->on_finish
        /// @note 调用方须保证 task 在本次调用期间存活（active_tasks_ 或调用者的 shared_ptr 保活）
        void finish_task(INetTask* task, CURLcode code, long http_code);
    };

    /**
     * @brief 获取全局单例 CurlMultiContext（首次调用时懒加载创建）
     * @return 全局上下文指针；仅在创建失败时返回 nullptr
     *
     * 线程安全；shutdown_multi_context() 之后再次调用会重新创建实例。
     */
    CurlMultiContext* get_multi_context();

    /**
     * @brief 显式关闭并销毁全局 CurlMultiContext 单例
     *
     * 停止 IO 线程（join）并执行 curl_multi_cleanup。
     *
     * 必须在 curl_global_cleanup() 之前调用：否则单例的静态析构会晚于
     * libcurl 全局清理（OpenSSL 3 下 atexit 顺序不确定），导致退出期
     * SSL_CTX / 连接缓存等资源泄漏。可重复调用，关闭后再次
     * get_multi_context() 会重新创建实例。
     */
    void shutdown_multi_context();

} // namespace fivecore::http_async

#endif //LIBFIVECORE_CURLMULTICONTEXT_H
