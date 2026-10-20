#pragma once
#include <functional>
#include <memory>
#include <string>

#include "fivecore/result.h"   // Result<T> / ErrorInfo / ErrorCode（res_* 系列依赖）

// 前向声明，Task本体仍在 fivecore::http_async，原有代码完全不动
namespace fivecore::http_async {
    class HttpPutStreamTask;
    class HttpGetStreamTask;
}

namespace fivecore::http {

    struct HttpResponse;

    // ================================================================
    // 第一代异步接口：回调携带 (bool ok, HttpResponse)
    // 与同步 get()/put_file_stream() 等原始版本对应，保留不动
    // ================================================================

    /// 异步PUT完成回调，对外业务层统一使用HttpResponse
    using PutStreamFinishCb = std::function<void(bool ok, const HttpResponse& resp)>;

    /**
     * @brief 异步流式上传文件（MinIO）
     * @param url 预签名上传地址
     * @param file_path 本地文件路径
     * @param finish_cb 任务结束回调（成功/失败，带HttpResponse）
     * @return task句柄，可用于cancel取消任务
     */
    std::shared_ptr<fivecore::http_async::HttpPutStreamTask> put_file_stream_async(
        const std::string& url,
        const std::string& file_path,
        PutStreamFinishCb finish_cb
    );


    using GetStreamFinishCb = std::function<void(bool ok, const HttpResponse& resp)>;
    std::shared_ptr<fivecore::http_async::HttpGetStreamTask> get_file_stream_async(
        const std::string& url,
        const std::string& save_file_path,
        GetStreamFinishCb finish_cb
    );

    // ================================================================
    // 第二代异步接口（res_* 系列，仿照同步 res_get/res_put_file_stream 迭代新增）
    // 回调携带 Result<HttpResponse>，错误通过 ErrorCode 结构化表达，
    // 语义与同步 res_* 系列完全对齐：
    //   - curl 传输失败 → ErrorCode::HttpTransferError
    //   - HTTP 状态非 200 → ErrorCode::HttpBadStatus
    //   - 成功 → Ok(HttpResponse)
    // 原有 put_file_stream_async / get_file_stream_async 保持不变（只增不删）
    // ================================================================

    /// res_* 系列统一回调：携带 Result<HttpResponse>
    using ResultFinishCb = std::function<void(const Result<HttpResponse>& result)>;

    /**
     * @brief 异步流式上传文件（MinIO），结果以 Result<HttpResponse> 回调
     * @param url 预签名上传地址
     * @param file_path 本地文件路径
     * @param finish_cb 任务结束回调（Result<HttpResponse>）
     * @return task句柄，可用于cancel取消任务
     */
    std::shared_ptr<fivecore::http_async::HttpPutStreamTask> res_put_file_stream_async(
        const std::string& url,
        const std::string& file_path,
        ResultFinishCb finish_cb
    );

    /**
     * @brief 异步流式下载文件，结果以 Result<HttpResponse> 回调
     * @param url 下载地址
     * @param save_file_path 本地保存路径
     * @param finish_cb 任务结束回调（Result<HttpResponse>）
     * @return task句柄，可用于cancel取消任务
     */
    std::shared_ptr<fivecore::http_async::HttpGetStreamTask> res_get_file_stream_async(
        const std::string& url,
        const std::string& save_file_path,
        ResultFinishCb finish_cb
    );


} // namespace fivecore::http
