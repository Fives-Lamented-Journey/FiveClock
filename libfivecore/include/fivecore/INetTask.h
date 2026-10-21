

/**
 * @file INetTask.h
 * @brief 异步网络任务抽象基类
 *
 * 所有异步 HTTP 任务（HttpGetTask / HttpPostTask / HttpPutStreamTask /
 * HttpGetStreamTask）均继承自 INetTask，由 CurlMultiContext 统一调度。
 * 采用模板方法模式：setup_easy / on_write / on_read / on_finish 由子类实现。
 */

#ifndef LIBFIVECORE_INETTASK_H
#define LIBFIVECORE_INETTASK_H
#include <atomic>
#include <cstdint>

#include "fivecore/result.h"
#include  <curl/curl.h>

namespace fivecore::http_async
{
    /**
     * @brief 异步网络任务抽象基类
     *
     * 子类需实现 setup_easy() 配置 curl easy 句柄，以及 on_write / on_read /
     * on_finish 处理数据回调。cancel() 默认仅置原子标记，实际中止由
     * CurlMultiContext 在 IO 线程中移除 handle 时完成。
     */
    class INetTask
    {
    public:
        /**
         * @brief curl 写回调（C 风格静态函数），转发到 userdata 指向的 task->on_write
         */
        static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
        /**
         * @brief curl 读回调（C 风格静态函数），转发到 userdata 指向的 task->on_read
         */
        static size_t read_callback(char* ptr, size_t size, size_t nmemb, void* userdata);

        /// 虚析构
        virtual ~INetTask() = default;

        /**
         * @brief 取消任务（业务线程调用）
         *
         * 默认实现仅将 is_cancelled_ 置 true，不阻塞。CurlMultiContext 的 IO 线程
         * 在下一次 curl_multi_perform 轮询时会移除该 handle 并触发 on_finish。
         * @return VoidResult，一般恒为 Ok
         */
        virtual VoidResult cancel();

        /**
         * @brief 启动前配置 easy 句柄（IO 线程调用）
         *
         * 子类在此设置 CURLOPT_URL / CURLOPT_WRITEFUNCTION / CURLOPT_READFUNCTION
         * / CURLOPT_WRITEDATA / CURLOPT_READDATA 等选项。
         * @return true 配置成功；false 失败
         */
        virtual bool setup_easy() = 0;

    protected:
        /**
         * @brief 接收响应数据回调
         * @param ptr 数据缓冲区
         * @param size 每个元素大小
         * @param nmemb 元素个数
         * @return 实际处理的字节数（size * nmemb 表示全部消费）
         */
        virtual size_t on_write(char* ptr, size_t size, size_t nmemb) = 0;
        /**
         * @brief 提供上传数据回调
         * @param ptr 输出缓冲区
         * @param size 每个元素大小
         * @param nmemb 可写入元素个数
         * @return 实际写入字节数；返回 0 表示数据已读完
         */
        virtual size_t on_read(char* ptr, size_t size, size_t nmemb) = 0;
        /**
         * @brief 任务完成回调（成功 / 失败 / 取消均会触发）
         * @param curl_code curl 传输结果码
         * @param http_code HTTP 状态码
         */
        virtual void on_finish(CURLcode curl_code, long http_code) = 0;

        CURL* easy_ = nullptr;              // curl easy 句柄
        std::atomic<bool> is_cancelled_{false}; // 取消标记（原子，业务线程写 / IO 线程读）

        friend class CurlMultiContext;      // 允许 CurlMultiContext 直接操作 easy_
    };
}

#endif //LIBFIVECORE_INETTASK_H
