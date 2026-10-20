#include "fivecore/http_async.h"
#include "fivecore/HttpPutStreamTask.h"
#include "fivecore/http.h"
#include "fivecore/CurlMultiContext.h"
#include <curl/curl.h>

#include "fivecore/HttpGetStreamTask.h"

namespace fivecore::http {

    /// @copydoc fivecore::http::put_file_stream_async
    std::shared_ptr<fivecore::http_async::HttpPutStreamTask> put_file_stream_async(
        const std::string& url,
        const std::string& file_path,
        PutStreamFinishCb finish_cb)
    {
        // lambda适配器：接收Task底层裸回调，转换成上层HttpResponse回调
        auto task_callback_wrapper = [finish_cb](CURLcode code, long http_code, std::string resp_body)
        {
            http::HttpResponse resp{};
            resp.status_code = http_code;
            resp.body = std::move(resp_body);
            resp.ok = (code == CURLE_OK);
            if (!resp.ok)
            {
                resp.error_msg = curl_easy_strerror(code);
            }

            if (finish_cb)
            {
                finish_cb(resp.ok, resp);
            }
        };

        // 直接构造你原来的Task，传入包装后的回调，Task代码完全不用修改
        auto task = std::make_shared<fivecore::http_async::HttpPutStreamTask>(url, file_path, task_callback_wrapper);
        auto ctx = fivecore::http_async::get_multi_context();
        if(ctx != nullptr)
        {
            ctx->submit_task(task);
        }
        return task;
    }

    /// @copydoc fivecore::http::get_file_stream_async
    std::shared_ptr<fivecore::http_async::HttpGetStreamTask> get_file_stream_async(
        const std::string& url,
        const std::string& save_file_path,
        GetStreamFinishCb finish_cb)
    {
        auto task_callback_wrapper = [finish_cb](CURLcode code, long http_code, std::string resp_body)
        {
            http::HttpResponse resp{};
            resp.status_code = http_code;
            resp.body = std::move(resp_body);
            resp.ok = (code == CURLE_OK);
            if (!resp.ok)
            {
                resp.error_msg = curl_easy_strerror(code);
            }
            if (finish_cb)
            {
                finish_cb(resp.ok, resp);
            }
        };

        auto task = std::make_shared<fivecore::http_async::HttpGetStreamTask>(url, save_file_path, task_callback_wrapper);
        auto ctx = fivecore::http_async::get_multi_context();
        if(ctx != nullptr)
        {
            ctx->submit_task(task);
        }
        return task;
    }

    // ================================================================
    // res_* 系列异步接口（仿照同步 res_get/res_put_file_stream 迭代新增）
    // 原有 put_file_stream_async / get_file_stream_async 保持不变
    // ================================================================

    /// 将 libcurl 执行结果转换为 Result<HttpResponse>，
    /// 判定逻辑与同步 res_* 系列（http.cpp）完全一致：
    ///   1. curl 传输失败 → HttpTransferError
    ///   2. HTTP 状态非 200 → HttpBadStatus
    ///   3. 成功 → Ok(HttpResponse)
    static Result<HttpResponse> to_result(CURLcode code, long http_code, std::string resp_body)
    {
        HttpResponse resp{};
        resp.status_code = http_code;
        resp.body = std::move(resp_body);

        if (code != CURLE_OK)
        {
            resp.error_msg = curl_easy_strerror(code);
            return Result<HttpResponse>::Err(
                ErrorInfo::make(ErrorCode::HttpTransferError, resp.error_msg));
        }

        if (http_code != 200)
        {
            resp.error_msg = "http status not 200";
            return Result<HttpResponse>::Err(
                ErrorInfo::make(ErrorCode::HttpBadStatus, resp.error_msg));
        }

        resp.ok = true;
        return Result<HttpResponse>::Ok(std::move(resp));
    }

    /// @copydoc fivecore::http::res_put_file_stream_async
    std::shared_ptr<fivecore::http_async::HttpPutStreamTask> res_put_file_stream_async(
        const std::string& url,
        const std::string& file_path,
        ResultFinishCb finish_cb)
    {
        // lambda适配器：Task底层 (CURLcode, http_code, body) → Result<HttpResponse>
        auto task_callback_wrapper = [finish_cb](CURLcode code, long http_code, std::string resp_body)
        {
            Result<HttpResponse> result = to_result(code, http_code, std::move(resp_body));
            if (finish_cb)
            {
                finish_cb(result);
            }
        };

        auto task = std::make_shared<fivecore::http_async::HttpPutStreamTask>(url, file_path, task_callback_wrapper);
        auto ctx = fivecore::http_async::get_multi_context();
        if (ctx != nullptr)
        {
            ctx->submit_task(task);
        }
        return task;
    }

    /// @copydoc fivecore::http::res_get_file_stream_async
    std::shared_ptr<fivecore::http_async::HttpGetStreamTask> res_get_file_stream_async(
        const std::string& url,
        const std::string& save_file_path,
        ResultFinishCb finish_cb)
    {
        auto task_callback_wrapper = [finish_cb](CURLcode code, long http_code, std::string resp_body)
        {
            Result<HttpResponse> result = to_result(code, http_code, std::move(resp_body));
            if (finish_cb)
            {
                finish_cb(result);
            }
        };

        auto task = std::make_shared<fivecore::http_async::HttpGetStreamTask>(url, save_file_path, task_callback_wrapper);
        auto ctx = fivecore::http_async::get_multi_context();
        if (ctx != nullptr)
        {
            ctx->submit_task(task);
        }
        return task;
    }


} // namespace fivecore::http
