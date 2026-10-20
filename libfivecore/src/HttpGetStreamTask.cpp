//
// Created by wufan on 2026/10/1.
//
#include "fivecore/HttpGetStreamTask.h"
#include <curl/curl.h>

namespace fivecore::http_async {

/// @copydoc fivecore::http_async::HttpGetStreamTask::HttpGetStreamTask
HttpGetStreamTask::HttpGetStreamTask(std::string url, std::string save_path, HttpStreamFinishCallback cb)
    : url_(std::move(url)), save_path_(std::move(save_path)), finish_cb_(std::move(cb))
{
    // 构造阶段先不打开文件，放到setup_easy里打开（在IO线程执行）
}

/// @copydoc fivecore::http_async::HttpGetStreamTask::~HttpGetStreamTask
HttpGetStreamTask::~HttpGetStreamTask()
{
    if(out_file_.is_open())
    {
        out_file_.close();
    }
    if(easy_)
    {
        curl_easy_cleanup(easy_);
        easy_ = nullptr;
    }
}

/// @copydoc fivecore::http_async::HttpGetStreamTask::setup_easy
bool HttpGetStreamTask::setup_easy()
{
    if(is_cancelled_.load())
        return false;

    if (!easy_)
    {
        easy_ = curl_easy_init();
        if (!easy_)
            return false;
    }

    // 打开输出文件，binary模式
    out_file_.open(save_path_, std::ios::binary | std::ios::trunc);
    if (!out_file_.is_open())
    {
        curl_easy_cleanup(easy_);
        easy_ = nullptr;
        return false;
    }

    curl_easy_setopt(easy_, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(easy_, CURLOPT_PRIVATE, this);
    curl_easy_setopt(easy_, CURLOPT_WRITEFUNCTION, INetTask::write_callback);
    curl_easy_setopt(easy_, CURLOPT_WRITEDATA, this);

    curl_easy_setopt(easy_, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(easy_, CURLOPT_FOLLOWLOCATION, 1L);
    return true;
}

/// @copydoc fivecore::http_async::HttpGetStreamTask::on_write
size_t HttpGetStreamTask::on_write(char* ptr, size_t size, size_t nmemb)
{
    if (is_cancelled_.load())
    {
        return 0; // 返回0终止传输
    }
    size_t total = size * nmemb;
    out_file_.write(ptr, static_cast<std::streamsize>(total));
    if (!out_file_.good())
    {
        return 0; // 文件写入失败，中断下载
    }
    return total;
}

/// @copydoc fivecore::http_async::HttpGetStreamTask::on_read
size_t HttpGetStreamTask::on_read(char* ptr, size_t size, size_t nmemb)
{
    // GET不需要上传，不会调用
    return 0;
}

/// @copydoc fivecore::http_async::HttpGetStreamTask::on_finish
void HttpGetStreamTask::on_finish(CURLcode curl_code, long http_code)
{
    if(out_file_.is_open())
    {
        out_file_.close();
    }
    // 第三个参数留空字符串，和put回调对齐
    finish_cb_(curl_code, http_code, "");

    if(easy_)
    {
        curl_easy_cleanup(easy_);
        easy_ = nullptr;
    }
}

} // namespace fivecore::http_async
