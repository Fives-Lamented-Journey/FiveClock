#ifndef LIBFIVECORE_HTTPPUTSTREAMTASK_H
#define LIBFIVECORE_HTTPPUTSTREAMTASK_H

#include "fivecore/INetTask.h"
#include <fstream>
#include <string>
#include <functional>
#include <atomic>

/**
 * @file HttpPutStreamTask.h
 * @brief 流式 PUT 上传任务（用于大文件 / MinIO 备份上传，不加载整个文件到内存）
 */
namespace fivecore::http_async {

    /**
     * @brief 流式 PUT 上传任务
     *
     * 通过 ifstream 分块读取文件，在 on_read 中按需填充 curl 缓冲区，
     * 避免大文件全量入内存。完成时回调携带 (CURLcode, http_code, resp_body)。
     */
    class HttpPutStreamTask : public INetTask
    {
    public:
        /// 任务完成回调：curl 结果码 + HTTP 状态码 + 响应体
        using FinishCallback = std::function<void(CURLcode code, long http_code, std::string resp)>;

        /**
         * @brief 构造上传任务
         * @param url 上传目标 URL（如 MinIO 预签名地址）
         * @param file_path 本地待上传文件路径
         * @param finish_cb 完成回调
         */
        HttpPutStreamTask(std::string url, std::string file_path, FinishCallback finish_cb);
        /// 析构：关闭文件流
        ~HttpPutStreamTask() override;

        /// 禁止拷贝
        HttpPutStreamTask(const HttpPutStreamTask&) = delete;
        /// 禁止拷贝赋值
        HttpPutStreamTask& operator=(const HttpPutStreamTask&) = delete;

        /// @copydoc fivecore::http_async::INetTask::setup_easy
        bool setup_easy() override;
        /// @copydoc fivecore::http_async::INetTask::cancel
        VoidResult cancel() override;

    protected:
        /// @copydoc fivecore::http_async::INetTask::on_read
        size_t on_read(char* ptr, size_t size, size_t nmemb) override;
        /// @copydoc fivecore::http_async::INetTask::on_write
        size_t on_write(char* ptr, size_t size, size_t nmemb) override;
        /// @copydoc fivecore::http_async::INetTask::on_finish
        void on_finish(CURLcode code, long http_code) override;

    private:
        std::string url_;            ///< 上传目标 URL
        std::string file_path_;      ///< 本地文件路径
        FinishCallback finish_cb_;   ///< 完成回调

        std::ifstream file_stream_;  ///< 文件输入流（二进制）
        uint64_t file_size_{0};      ///< 文件总大小（用于 CURLOPT_INFILESIZE_LARGE）
        std::atomic<bool> file_read_error_{false}; ///< 文件读取出错标记
        std::string response_buf_;   ///< 响应体缓冲
    };

} // namespace fivecore::http_async

#endif // LIBFIVECORE_HTTPPUTSTREAMTASK_H
