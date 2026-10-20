//
// Created by wufan on 2026/10/1.
//
#include "fivecore/HttpPostTask.h"
#include <curl/curl.h>

namespace fivecore::http_async {

    HttpPostTask::~HttpPostTask()
    {
        // 兜底：析构时 easy_ 仍存在说明 on_finish 未被调用
        if (easy_)
        {
            curl_easy_cleanup(easy_);
            easy_ = nullptr;
        }
        // 同理 headers_ 未被 on_finish 释放
        if (headers_)
        {
            curl_slist_free_all(headers_);
            headers_ = nullptr;
        }
    }

    /// @copydoc fivecore::http_async::HttpPostTask::HttpPostTask
    HttpPostTask::HttpPostTask(std::string url, std::string json_body, HttpFinishCallback cb)
        : url_(std::move(url)), json_body_(std::move(json_body)), finish_cb_(std::move(cb))
    {}

    /// @copydoc fivecore::http_async::HttpPostTask::setup_easy
    bool HttpPostTask::setup_easy()
    {
        // 保护：已经初始化过easy句柄，直接返回，防止重复setup
        if (easy_ != nullptr)
        {
            return true;
        }

        easy_ = curl_easy_init();
        if (!easy_)
            return false;

        curl_easy_setopt(easy_, CURLOPT_URL, url_.c_str());

        // POST 提交
        curl_easy_setopt(easy_, CURLOPT_POST, 1L);
        // 修改为 .data() 语义对齐二进制buffer
        curl_easy_setopt(easy_, CURLOPT_POSTFIELDS, json_body_.data());
        curl_easy_setopt(easy_, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(json_body_.size()));

        // 构建请求头链表，使用成员变量headers_
        headers_ = curl_slist_append(nullptr, "Content-Type: application/json");
        if (!headers_)
        {
            curl_easy_cleanup(easy_);
            easy_ = nullptr;
            return false;
        }
        curl_easy_setopt(easy_, CURLOPT_HTTPHEADER, headers_);

        // 回调绑定，和HttpGetTask保持一致
        curl_easy_setopt(easy_, CURLOPT_WRITEFUNCTION, INetTask::write_callback);
        curl_easy_setopt(easy_, CURLOPT_WRITEDATA, this); // 新增：write回调的userdata
        curl_easy_setopt(easy_, CURLOPT_PRIVATE, this);

        return true;
    }

    /// @copydoc fivecore::http_async::HttpPostTask::on_write
    size_t HttpPostTask::on_write(char* ptr, size_t size, size_t nmemb)
    {
        size_t total = size * nmemb;
        response_buf_.append(ptr, total);
        return total;
    }

    /// @copydoc fivecore::http_async::HttpPostTask::on_read
    size_t HttpPostTask::on_read(char* ptr, size_t size, size_t nmemb)
    {
        return 0;
    }

    /// @copydoc fivecore::http_async::HttpPostTask::on_finish
    void HttpPostTask::on_finish(CURLcode code, long http_code)
    {
        // 请求已经结束，libcurl不再使用header链表，可以安全释放
        if (headers_)
        {
            curl_slist_free_all(headers_);
            headers_ = nullptr;
        }

        if (finish_cb_)
        {
            finish_cb_(code, http_code, std::move(response_buf_));
        }

        // 释放 easy 句柄（任务从 multi 摘除后由任务自身负责清理，
        // 与 HttpGetTask::on_finish 的处理保持一致）
        if (easy_)
        {
            curl_easy_cleanup(easy_);
            easy_ = nullptr;
        }
    }

}
