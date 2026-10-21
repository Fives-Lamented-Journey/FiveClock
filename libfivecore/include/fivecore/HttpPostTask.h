
/**
 * @file HttpPostTask.h
 * @brief 异步 POST JSON 任务
 */
#ifndef LIBFIVECORE_HTTPPOSTTASK_H
#define LIBFIVECORE_HTTPPOSTTASK_H

#include "fivecore/INetTask.h"
#include <string>
#include <functional>

// 前置声明，不在头文件引入 curl 完整头
struct curl_slist;

namespace fivecore::http_async {

    /**
     * @brief 异步 POST JSON 任务
     *
     * 自动设置 Content-Type: application/json 请求头，将 JSON 字符串作为请求体发送。
     */
    class HttpPostTask : public INetTask
    {
    public:

    public:
        /// 任务完成回调：curl 结果码 + HTTP 状态码 + 响应体字符串
        using HttpFinishCallback = std::function<void(CURLcode code, long http_code, std::string resp)>;

        /**
         * @brief 构造 POST 任务
         * @param url 请求 URL
         * @param json_body JSON 请求体
         * @param cb 完成回调
         */
        HttpPostTask(std::string url, std::string json_body, HttpFinishCallback cb);
        /// 析构：兜底释放 easy_ 和 headers_
        ~HttpPostTask() override;

        /// @copydoc fivecore::http_async::INetTask::setup_easy
        bool setup_easy() override;
    protected:
        /// @copydoc fivecore::http_async::INetTask::on_write
        size_t on_write(char* ptr, size_t size, size_t nmemb) override;
        /// @copydoc fivecore::http_async::INetTask::on_read
        size_t on_read(char* ptr, size_t size, size_t nmemb) override;
        /// @copydoc fivecore::http_async::INetTask::on_finish
        void on_finish(CURLcode code, long http_code) override;

    private:
        std::string url_;                      ///< 请求 URL
        std::string json_body_;                ///< JSON 请求体
        HttpFinishCallback finish_cb_;         ///< 完成回调
        std::string response_buf_;             ///< 响应体缓冲
        struct curl_slist* headers_{nullptr};  ///< 请求头链表（setup_easy 中构造，析构释放）
    };

} // namespace fivecore::http_async

#endif //LIBFIVECORE_HTTPPOSTTASK_H
