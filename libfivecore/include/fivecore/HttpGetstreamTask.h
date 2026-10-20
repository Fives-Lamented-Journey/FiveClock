#ifndef LIBFIVECORE_HTTPGETSTREAMTASK_H
#define LIBFIVECORE_HTTPGETSTREAMTASK_H

#include <functional>
#include <fstream>
#include <string>

#include "fivecore/INetTask.h"

/**
 * @file HttpGetstreamTask.h
 * @brief 流式 GET 下载任务（响应体直接写入本地文件，不经过内存缓冲）
 */
namespace fivecore::http_async {

    /// 任务完成回调：curl 结果码 + HTTP 状态码 + 响应体字符串
    using HttpStreamFinishCallback = std::function<void(CURLcode code, long http_code, std::string body)>;

    /**
     * @brief 流式 GET 下载任务
     *
     * 在 on_write 中将收到的数据直接写入本地 ofstream，适合大文件下载。
     */
    class HttpGetStreamTask : public INetTask
    {
    public:
        /**
         * @brief 构造下载任务
         * @param url 下载 URL
         * @param save_path 本地保存路径
         * @param cb 完成回调
         */
        HttpGetStreamTask(std::string url, std::string save_path, HttpStreamFinishCallback cb);
        /// 析构：关闭输出文件流
        ~HttpGetStreamTask() override;

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
        std::string url_;                  ///< 下载 URL
        std::string save_path_;            ///< 本地保存路径
        std::ofstream out_file_;           ///< 输出文件流（二进制）
        HttpStreamFinishCallback finish_cb_; ///< 完成回调
    };

} // namespace fivecore::http_async

#endif //LIBFIVECORE_HTTPGETSTREAMTASK_H
