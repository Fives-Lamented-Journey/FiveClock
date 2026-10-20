#include "fivecore/HttpGetTask.h"
namespace fivecore::http_async {

    /// @copydoc fivecore::http_async::HttpGetTask::HttpGetTask
    HttpGetTask::HttpGetTask(std::string url, HttpFinishCallback cb)
        :url_(std::move(url)), finish_cb_(std::move(cb))
    {
    }

    HttpGetTask::~HttpGetTask()
    {
        // 兜底：析构时 easy_ 仍存在说明 on_finish 未被调用（极端异常路径）
        if (easy_)
        {
            curl_easy_cleanup(easy_);
            easy_ = nullptr;
        }
    }

    /// @copydoc fivecore::http_async::HttpGetTask::setup_easy
    bool HttpGetTask::setup_easy()
    {
        if(is_cancelled_.load())
            return false;

        if (!easy_)
        {
            easy_ = curl_easy_init();
            if (!easy_)
                return false;
        }

        // 设置url
        curl_easy_setopt(easy_, CURLOPT_URL, url_.c_str());
        curl_easy_setopt(easy_, CURLOPT_PRIVATE, this);
        
        // 绑定基类静态write回调
        curl_easy_setopt(easy_, CURLOPT_WRITEFUNCTION, INetTask::write_callback);

        // write_callback的userdata，传入this
        curl_easy_setopt(easy_, CURLOPT_WRITEDATA, this);

        // 可选：关闭信号、重定向等
        curl_easy_setopt(easy_, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(easy_, CURLOPT_FOLLOWLOCATION, 1L);
        return true;
    }

    /// @copydoc fivecore::http_async::HttpGetTask::on_write
    size_t HttpGetTask::on_write(char* ptr, size_t size, size_t nmemb)
    {
        if (is_cancelled_.load())
            return 0; // 返回0，libcurl终止传输
        size_t total = size * nmemb;
        resp_body_.insert(resp_body_.end(), reinterpret_cast<uint8_t*>(ptr), reinterpret_cast<uint8_t*>(ptr) + total);
        return total;
    }

    /// @copydoc fivecore::http_async::HttpGetTask::on_read
    size_t HttpGetTask::on_read(char* ptr, size_t size, size_t nmemb)
    {
        // GET请求不上传数据，该回调永远不会触发
        return 0;
    }

    /// @copydoc fivecore::http_async::HttpGetTask::on_finish
    void HttpGetTask::on_finish(CURLcode curl_code, long http_code)
    {
        finish_cb_(curl_code, http_code, std::move(resp_body_));

        // 释放easy资源
        if(easy_)
        {
            curl_easy_cleanup(easy_);
            easy_ = nullptr;
        }
    }

}
