#include "fivecore/HttpPutStreamTask.h"
#include <curl/curl.h>

namespace fivecore::http_async {

/// @copydoc fivecore::http_async::HttpPutStreamTask::HttpPutStreamTask
HttpPutStreamTask::HttpPutStreamTask(std::string url, std::string file_path, FinishCallback finish_cb)
    : url_(std::move(url))
    , file_path_(std::move(file_path))
    , finish_cb_(std::move(finish_cb))
    , file_size_(0)
    , file_read_error_(false)
{}

/// @copydoc fivecore::http_async::HttpPutStreamTask::~HttpPutStreamTask
HttpPutStreamTask::~HttpPutStreamTask()
{
    if (file_stream_.is_open())
    {
        file_stream_.close();
    }

    // 析构兜底：正常/取消/stop 路径都会先走 on_finish 释放 easy_，
    // 此处仅防御 setup_easy 未被调度等极端场景
    if (easy_)
    {
        curl_easy_cleanup(easy_);
        easy_ = nullptr;
    }
}

/// @copydoc fivecore::http_async::HttpPutStreamTask::setup_easy
bool HttpPutStreamTask::setup_easy()
{
    // 防重复初始化
    if (easy_ != nullptr)
    {
        return true;
    }

    easy_ = curl_easy_init();
    if (!easy_)
    {
        return false;
    }

    // 1. 二进制方式打开本地文件
    file_stream_.open(file_path_, std::ios::in | std::ios::binary);
    if (!file_stream_.is_open())
    {
        curl_easy_cleanup(easy_);
        easy_ = nullptr;
        return false;
    }

    // 2. 获取文件总大小
    file_stream_.seekg(0, std::ios::end);
    auto total = file_stream_.tellg();
    if (total == -1)
    {
        file_stream_.close();
        curl_easy_cleanup(easy_);
        easy_ = nullptr;
        return false;
    }
    file_size_ = static_cast<uint64_t>(total);
    file_stream_.seekg(0, std::ios::beg);

    // 3. 配置curl选项
    curl_easy_setopt(easy_, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(easy_, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(easy_, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(file_size_));

    // 4. 绑定回调 + userdata（读写分别设置，避免野指针）
    curl_easy_setopt(easy_, CURLOPT_READFUNCTION, INetTask::read_callback);
    curl_easy_setopt(easy_, CURLOPT_READDATA, this);

    curl_easy_setopt(easy_, CURLOPT_WRITEFUNCTION, INetTask::write_callback);
    curl_easy_setopt(easy_, CURLOPT_WRITEDATA, this);

    curl_easy_setopt(easy_, CURLOPT_PRIVATE, this);

    return true;
}

/// @copydoc fivecore::http_async::HttpPutStreamTask::cancel
VoidResult HttpPutStreamTask::cancel()
{
    // 先调用基类cancel，置位is_cancelled_
    INetTask::cancel();

    // 派生类额外清理：主动关闭文件流，终止读取
    if (file_stream_.is_open())
    {
        file_stream_.close();
    }
    return OkVoid();
}

/// @copydoc fivecore::http_async::HttpPutStreamTask::on_read
size_t HttpPutStreamTask::on_read(char* ptr, size_t size, size_t nmemb)
{
    // 复用基类的取消标记
    if (is_cancelled_.load())
    {
        if (file_stream_.is_open())
        {
            file_stream_.close();
        }
        return 0;
    }

    const size_t max_read = size * nmemb;
    file_stream_.read(ptr, max_read);

    if (!file_stream_)
    {
        // 区分正常EOF和真实IO错误
        if (!file_stream_.eof())
        {
            file_read_error_.store(true);
            if (file_stream_.is_open())
            {
                file_stream_.close();
            }
            return 0;
        }
    }

    return static_cast<size_t>(file_stream_.gcount());
}

/// @copydoc fivecore::http_async::HttpPutStreamTask::on_write
size_t HttpPutStreamTask::on_write(char* ptr, size_t size, size_t nmemb)
{
    const size_t total = size * nmemb;
    response_buf_.append(ptr, total);
    return total;
}

/// @copydoc fivecore::http_async::HttpPutStreamTask::on_finish
void HttpPutStreamTask::on_finish(CURLcode code, long http_code)
{
    // 请求结束，主动关闭文件释放资源
    if (file_stream_.is_open())
    {
        file_stream_.close();
    }

    // 任务已被 CurlMultiContext 从 multi 摘除，easy 句柄由任务自身释放；
    // 放在所有回调分支之前，保证提前 return 路径也不会遗漏
    if (easy_)
    {
        curl_easy_cleanup(easy_);
        easy_ = nullptr;
    }

    // 本地文件读错误优先级最高，覆盖libcurl返回码
    if (file_read_error_.load())
    {
        finish_cb_(CURLE_READ_ERROR, http_code, "local file read failed during upload");
        return;
    }

    finish_cb_(code, http_code, std::move(response_buf_));
}

} // namespace fivecore::http_async
