//
// Created by wufan on 2026/9/28.
//

/**
 * @file HttpGetTask.h
 * @brief 异步 GET 任务（响应体全部读入内存 vector）
 */

#ifndef LIBFIVECORE_HTTPGETTASK_H
#define LIBFIVECORE_HTTPGETTASK_H

#include <functional>
#include <vector>

#include "fivecore/INetTask.h"

namespace fivecore::http_async
{
    /// 任务完成回调：curl 结果码 + HTTP 状态码 + 响应体字节向量
    using HttpFinishCallback = std::function<void(CURLcode code, long http_code, std::vector<uint8_t> body)>;

    /**
     * @brief 异步 GET 任务
     *
     * 将响应体累积到 std::vector<uint8_t>，适合小响应 / JSON 接口。
     */
    class HttpGetTask: public INetTask
    {
    public:
        /**
         * @brief 构造 GET 任务
         * @param url 请求 URL
         * @param cb 完成回调
         */
        HttpGetTask(std::string url, HttpFinishCallback cb);
        /// 析构：兜底释放 easy_
        ~HttpGetTask() override;

        /// @copydoc fivecore::http_async::INetTask::setup_easy
        bool setup_easy() override;

    protected:
        /// @copydoc fivecore::http_async::INetTask::on_write
        size_t on_write(char* ptr, size_t size, size_t nmemb) override;
        /// @copydoc fivecore::http_async::INetTask::on_read
        size_t on_read(char* ptr, size_t size, size_t nmemb) override;
        /// @copydoc fivecore::http_async::INetTask::on_finish
        void on_finish(CURLcode curl_code, long http_code) override;

    private:

        std::string url_;                    ///< 请求 URL
        std::vector<uint8_t> resp_body_;     ///< 响应体累积缓冲
        HttpFinishCallback finish_cb_;       ///< 完成回调

    };
}


#endif //LIBFIVECORE_HTTPGETTASK_H
