
/**
 * @file http.h
 * @brief 同步 HTTP 客户端接口（基于 libcurl easy 接口）
 *
 * 提供两套对等 API：
 *  - 原始版：get / post_json / put_file_stream / get_file_stream，返回 HttpResponse
 *  - Result 版：res_get / res_post_json / res_put_file_stream / res_get_file_stream，
 *    返回 Result<HttpResponse>，错误通过 ErrorCode 结构化表达
 *
 * 使用前需在进程入口构造一次 CurlGlobalGuard 完成 libcurl 全局初始化。
 *
 * @todo 改进:可以对原始curl句柄做层薄RAII封装
 */

#ifndef LIBFIVECORE_HTTP_H
#define LIBFIVECORE_HTTP_H
#pragma once
#include <string>
#include <fstream>
#include  <vector>

#include "result.h"

namespace fivecore::http {

    /**
     * @brief HTTP 响应结构体
     *
     * 同时被同步 / 异步接口复用。原始版接口通过 ok 字段判断成败；
     * Result 版接口返回时 ok 恒为 true（失败由 Result 外层表达）。
     */
    struct HttpResponse
    {
        bool ok{false};              ///< 请求是否成功（curl 成功且 HTTP 200）
        long status_code{0};         ///< HTTP 状态码
        std::string body;            ///< 响应体（流式下载接口中为空）
        std::string error_msg;       ///< 错误描述（失败时填充）
    };


    /**
     * @brief 同步 GET 请求，响应体全部读入内存
     * @param url 请求地址
     * @param queries 查询参数键值对（内部自动 URL 编码）
     * @param timeout_ms 超时时间（毫秒），默认 10s
     * @return HttpResponse，ok 表示 curl 成功且状态码 200
     */
    HttpResponse get(const std::string& url, const std::vector<std::pair<std::string,std::string>>& queries , long timeout_ms = 10000L);

    /**
     * @brief 同步 POST JSON 请求
     * @param url 请求地址
     * @param json_str 请求体 JSON 字符串
     * @return HttpResponse，ok 表示 curl 成功且状态码 200
     *
     * 注：暂时post没有带查询参数的需求
     */
    HttpResponse post_json(const std::string& url, const std::string& json_str);

    /**
     * @brief 同步流式 PUT 上传文件（用于 MinIO 预签名上传）
     * @param url 预签名上传地址
     * @param file_path 本地待上传文件路径
     * @return HttpResponse，ok 表示上传成功且状态码 200
     *
     * 注：S3签名由后端生成，客户端直接使用
     */
    HttpResponse put_file_stream(const std::string& url, const std::string& file_path);

    /**
     * @brief 同步流式 GET 下载，响应体直接写入本地文件
     * @param url 下载地址
     * @param outputFilePath 本地保存路径
     * @return HttpResponse，ok 表示下载成功且状态码 200
     *
     * 注：S3签名由后端生成，客户端直接使用
     */
    HttpResponse get_file_stream(const std::string& url, const std::string& outputFilePath);


    /**
     * @brief Result 版同步 GET
     * @copydetails get
     * @return Ok(HttpResponse) 成功；Err(HttpTransferError/HttpBadStatus) 失败
     */
    Result<HttpResponse> res_get(const std::string& url, const std::vector<std::pair<std::string,std::string>>& queries , long timeout_ms = 10000L);

    /**
     * @brief Result 版同步 POST JSON
     * @copydetails post_json
     * @return Ok(HttpResponse) 成功；Err(HttpTransferError/HttpBadStatus) 失败
     */
    Result<HttpResponse> res_post_json(const std::string& url, const std::string& json_str);

    /**
     * @brief Result 版同步流式 PUT 上传
     * @copydetails put_file_stream
     * @return Ok(HttpResponse) 成功；Err(FileOpenFailed/HttpTransferError/HttpBadStatus) 失败
     */
    Result<HttpResponse> res_put_file_stream(const std::string& url, const std::string& file_path);

    /**
     * @brief Result 版同步流式 GET 下载
     * @copydetails get_file_stream
     * @return Ok(HttpResponse) 成功；Err(FileOpenFailed/HttpTransferError/HttpBadStatus) 失败
     */
    Result<HttpResponse> res_get_file_stream(const std::string& url, const std::string& outputFilePath);

    /**
     * @brief libcurl 全局初始化 / 清理的 RAII 守卫
     *
     * 在 main() 中构造一次即可，析构时自动调用 curl_global_cleanup。
     * 多线程 / HTTPS 场景下必须显式初始化，否则行为未定义。
     */
    class CurlGlobalGuard
    {
    public:
        /// 构造：调用 curl_global_init(CURL_GLOBAL_ALL)
        CurlGlobalGuard();
        /// 析构：调用 curl_global_cleanup()
        ~CurlGlobalGuard();

        /// 禁止拷贝
        CurlGlobalGuard(const CurlGlobalGuard&) = delete;
        /// 禁止拷贝赋值
        CurlGlobalGuard& operator=(const CurlGlobalGuard&) = delete;
    };

}

#endif //LIBFIVECORE_HTTP_H
