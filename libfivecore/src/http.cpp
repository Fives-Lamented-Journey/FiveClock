#include "fivecore/http.h"
#include <curl/curl.h>
#include <vector>
#include "fivecore/fileutil.h"

/**
 * @file http.cpp
 * @brief 同步 HTTP 客户端实现（libcurl easy 接口）
 */
namespace fivecore::http {

// 接收响应数据的write回调（静态C风格）
static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    size_t total = size * nmemb;
    auto* resp_body = static_cast<std::string*>(userdata);
    resp_body->append(ptr, total);
    return total;
}

// PUT上传读取文件的read回调（静态C风格）
static size_t read_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* fs = static_cast<std::ifstream*>(userdata);
    const size_t max_bytes = size * nmemb;
    fs->read(ptr, static_cast<std::streamsize>(max_bytes));
    return static_cast<size_t>(fs->gcount());
}

    // 新增：下载写文件回调
static size_t download_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    std::ofstream* fs = static_cast<std::ofstream*>(userdata);
    size_t total = size * nmemb;
    fs->write(ptr, static_cast<std::streamsize>(total));
    return total;
}


static std::string build_url(const std::string& base_url, const std::vector<std::pair<std::string, std::string>>& params)
{
    CURLU *url = curl_url();
    CURLUcode rc = curl_url_set(url, CURLUPART_URL, base_url.c_str(), 0);
    if(rc != CURLUE_OK)
    {
        curl_url_cleanup(url);
        return base_url;
    }

    for(const auto& [k, v] : params)
    {
        // 拼接 key=value
        std::string kv = k + "=" + v;
        // APPENDQUERY：追加，自动插入&
        // URLENCODE：对整串做url编码，但保留=符号不编码
        curl_url_set(url, CURLUPART_QUERY, kv.c_str(),
                     CURLU_APPENDQUERY | CURLU_URLENCODE);
    }

    char* out_url = nullptr;
    curl_url_get(url, CURLUPART_URL, &out_url, 0);
    std::string result(out_url);

    curl_free(out_url);
    curl_url_cleanup(url);
    return result;
}


/// @copydoc fivecore::http::CurlGlobalGuard::CurlGlobalGuard
CurlGlobalGuard::CurlGlobalGuard()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

/// @copydoc fivecore::http::CurlGlobalGuard::~CurlGlobalGuard
CurlGlobalGuard::~CurlGlobalGuard()
{
    curl_global_cleanup();
}

    // http.cpp

    // 静态 write_callback 你代码里已经存在，无需重复写
    // static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) { ... }

/// @copydoc fivecore::http::get
HttpResponse get(const std::string& url, const std::vector<std::pair<std::string,std::string>>& queries,long timeout_ms)
{
    HttpResponse res;
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        res.error_msg = "curl_easy_init failed";
        return res;
    }

    std::string fullurl = build_url(url,queries);

    std::string response_body;
    curl_easy_setopt(curl, CURLOPT_URL, fullurl.c_str());
    // GET 是libcurl默认，不需要额外设置CURLOPT_HTTPGET；如需显式：
    // curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    // 响应接收回调
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

    // 本地开发关闭ssl校验
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);

    CURLcode curl_ret = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status_code);

    if (curl_ret == CURLE_OK)
    {
        res.body = std::move(response_body);
        res.ok = (res.status_code == 200);
    }
    else
    {
        res.error_msg = curl_easy_strerror(curl_ret);
    }

    curl_easy_cleanup(curl);
    return res;
}

/// @copydoc fivecore::http::post_json
HttpResponse post_json(const std::string& url, const std::string& json_str)
{
    HttpResponse res;
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        res.error_msg = "curl_easy_init failed";
        return res;
    }

    // 1. 初始化头链表，必须初始化为NULL
    struct curl_slist* headers = nullptr;

    // 添加 application 头，常用：application/json
    headers = curl_slist_append(headers, "Content-Type: application/json");
    // 可选：Accept: application/json 告诉服务器希望返回json
    headers = curl_slist_append(headers, "Accept: application/json");

    // 绑定头列表到curl句柄
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    std::string response_body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_str.size()));

    // 响应回调
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

    // 本地开发关闭SSL校验，生产务必开启
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // 总超时 10s
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000L);

    CURLcode curl_ret = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status_code);

    // 释放请求头链表（curl_easy_cleanup 不会替我们释放 slist）
    curl_slist_free_all(headers);

    if (curl_ret == CURLE_OK)
    {
        res.body = std::move(response_body);
        res.ok = (res.status_code == 200);
    }
    else
    {
        res.error_msg = curl_easy_strerror(curl_ret);
    }

    curl_easy_cleanup(curl);
    return res;
}

/// @copydoc fivecore::http::put_file_stream
HttpResponse put_file_stream(const std::string& url, const std::string& file_path)
{
    HttpResponse res;
    std::ifstream fs(file_path, std::ios::binary);

    if (!fs.is_open())
    {
        res.error_msg = "cannot open file: " + file_path;
        return res;
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        res.error_msg = "curl_easy_init failed";
        return res;
    }


    curl_off_t file_size = fileutil::get_size(file_path);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, file_size);


    std::string response_body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);



    // 文件读回调
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &fs);

    // 接收服务端返回
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000L);

    CURLcode curl_ret = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status_code);

    if (curl_ret == CURLE_OK)
    {
        res.body = std::move(response_body);
        res.ok = (res.status_code == 200);
    }
    else
    {
        res.error_msg = curl_easy_strerror(curl_ret);
    }

    curl_easy_cleanup(curl);
    return res;
}

/// @copydoc fivecore::http::get_file_stream
HttpResponse get_file_stream(const std::string& url, const std::string& outputFilePath)
{
    HttpResponse res;
    std::ofstream fs(outputFilePath, std::ios::binary);
    if (!fs.is_open())
    {
        res.error_msg = "cannot open output file: " + outputFilePath;
        return res;
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        res.error_msg = "curl_easy_init failed";
        return res;
    }

    std::string respBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fs);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000L);

    CURLcode curl_ret = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status_code);

    if (curl_ret == CURLE_OK)
    {
        res.ok = (res.status_code == 200);
    }
    else
    {
        res.error_msg = curl_easy_strerror(curl_ret);
    }

    curl_easy_cleanup(curl);
    return res;
}

/// @copydoc fivecore::http::res_get
    Result<HttpResponse> res_get(const std::string& url,
    const std::vector<std::pair<std::string,std::string>>& queries,
    long timeout_ms)
{
    HttpResponse res;
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::CurlCreateHandleFailed, "curl_easy_init failed"));
    }

    std::string fullurl = build_url(url, queries);
    std::string response_body;

    curl_easy_setopt(curl, CURLOPT_URL, fullurl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);

    CURLcode curl_ret = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status_code);

    if (curl_ret != CURLE_OK)
    {
        std::string err_msg = curl_easy_strerror(curl_ret);
        curl_easy_cleanup(curl);
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::HttpTransferError, err_msg));
    }

    res.body = std::move(response_body);
    if (res.status_code != 200)
    {
        curl_easy_cleanup(curl);
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::HttpBadStatus, "http status not 200"));
    }

    res.ok = true;
    curl_easy_cleanup(curl);
    return Result<HttpResponse>::Ok(std::move(res));
}

/// @copydoc fivecore::http::res_post_json
Result<HttpResponse> res_post_json(const std::string& url, const std::string& json_str)
{
    HttpResponse res;
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::CurlCreateHandleFailed, "curl_easy_init failed"));
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    std::string response_body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_str.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000L);

    CURLcode curl_ret = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status_code);

    curl_slist_free_all(headers);

    if (curl_ret != CURLE_OK)
    {
        std::string err_msg = curl_easy_strerror(curl_ret);
        curl_easy_cleanup(curl);
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::HttpTransferError, err_msg));
    }

    res.body = std::move(response_body);
    if (res.status_code != 200)
    {
        curl_easy_cleanup(curl);
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::HttpBadStatus, "http status not 200"));
    }

    res.ok = true;
    curl_easy_cleanup(curl);
    return Result<HttpResponse>::Ok(std::move(res));
}

/// @copydoc fivecore::http::res_put_file_stream
Result<HttpResponse> res_put_file_stream(const std::string& url, const std::string& file_path)
{
    HttpResponse res;
    std::ifstream fs(file_path, std::ios::binary);
    if (!fs.is_open())
    {
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::FileOpenFailed, "cannot open file: " + file_path));
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::CurlCreateHandleFailed, "curl_easy_init failed"));
    }

    curl_off_t file_size = fileutil::get_size(file_path);
    std::string response_body;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, file_size);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &fs);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000L);

    CURLcode curl_ret = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status_code);

    if (curl_ret != CURLE_OK)
    {
        std::string err_msg = curl_easy_strerror(curl_ret);
        curl_easy_cleanup(curl);
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::HttpTransferError, err_msg));
    }

    res.body = std::move(response_body);
    if (res.status_code != 200)
    {
        curl_easy_cleanup(curl);
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::HttpBadStatus, "http status not 200"));
    }

    res.ok = true;
    curl_easy_cleanup(curl);
    return Result<HttpResponse>::Ok(std::move(res));
}

/// @copydoc fivecore::http::res_get_file_stream
Result<HttpResponse> res_get_file_stream(const std::string& url, const std::string& outputFilePath)
{
    HttpResponse res;
    std::ofstream fs(outputFilePath, std::ios::binary);
    if (!fs.is_open())
    {
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::FileOpenFailed, "cannot open output file: " + outputFilePath));
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::CurlCreateHandleFailed, "curl_easy_init failed"));
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fs);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000L);

    CURLcode curl_ret = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status_code);

    if (curl_ret != CURLE_OK)
    {
        std::string err_msg = curl_easy_strerror(curl_ret);
        curl_easy_cleanup(curl);
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::HttpTransferError, err_msg));
    }

    if (res.status_code != 200)
    {
        curl_easy_cleanup(curl);
        return Result<HttpResponse>::Err(
            ErrorInfo::make(ErrorCode::HttpBadStatus, "http status not 200"));
    }

    res.ok = true;
    curl_easy_cleanup(curl);
    return Result<HttpResponse>::Ok(std::move(res));
}

}
