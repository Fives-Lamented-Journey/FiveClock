//
// Created by wufan on 2026/10/1.
//
// 追加到同一份测试文件，放在namespace fivecore::http::test里面
#include <gtest/gtest.h>
#include "fivecore/http.h"
#include "fivecore/http_async.h"
#include "fivecore/CurlMultiContext.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include "fivecore/fileutil.h"
#include "fivecore/hash.h"
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "fivecore/HttpPutStreamTask.h"

#include <memory>

// ===================== 全局测试环境：libcurl 生命周期管理 =====================
// 背景：不显式 curl_global_init/cleanup 时，libcurl 8.x 懒加载注册的 atexit
// 与 OpenSSL 3 的清理顺序不确定，且全局 CurlMultiContext 单例的静态析构更晚，
// Valgrind 下表现为 SSL_CTX/RAND/连接缓存的退出期 "definitely lost"。
//
// gtest 全局 Environment 的 TearDown 在 RUN_ALL_TESTS() 内、所有 atexit/静态
// 析构之前执行，因此在这里保证严格的销毁顺序：
//   curl_global_init → 测试运行 → shutdown_multi_context → curl_global_cleanup
namespace {
class CurlGlobalTestEnvironment : public ::testing::Environment
{
public:
    void SetUp() override
    {
        guard_ = std::make_unique<fivecore::http::CurlGlobalGuard>();
    }

    void TearDown() override
    {
        // 先销毁 multi 单例（join IO 线程 + curl_multi_cleanup），
        // 再做全局 cleanup，顺序不可颠倒
        fivecore::http_async::shutdown_multi_context();
        guard_.reset();
    }

private:
    std::unique_ptr<fivecore::http::CurlGlobalGuard> guard_;
};

// gtest 允许在 RUN_ALL_TESTS 之前（含静态初始化阶段）注册全局环境
static ::testing::Environment* const g_curl_global_env [[maybe_unused]] =
    ::testing::AddGlobalTestEnvironment(new CurlGlobalTestEnvironment);
} // namespace

namespace fivecore::http::test {
using json = nlohmann::json;

// ===================== 集成测试：后端获取预签名URL + 异步上传到MinIO，和同步upload用同一套后端逻辑 =====================
#if HTTP_INTEGRATION_TEST
TEST(HttpAsyncIntegrationTest, AsyncUploadWithPresign)
{
    CurlGlobalGuard guard;
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    bool upload_ok = false;
    HttpResponse upload_resp;

    const std::string path = "intergration_async.txt";
    // 准备测试文件
    {
        std::ofstream f(path, std::ios::binary);
        f.write("hello fivecore async upload", 27);
    }

    // 1. 请求后端获取预签名上传URL（复用你原有逻辑）
    json req;
    req["object_key"] = path;
    req["file_size"] = fileutil::get_size(path);
    req["sha256_hash"] = hash::sha256_file(path);
    std::cout << "object_key: " << req["object_key"] << "\n";
    std::cout << "file_size: " << req["file_size"] << "\n";
    std::cout << "sha256_hash: " << req["sha256_hash"] << "\n";

    std::string url = "http://127.0.0.1:8000/api/presign/upload";
    auto resp = post_json(url, req.dump());
    ASSERT_TRUE(resp.ok);
    ASSERT_EQ(resp.status_code,200);
    ASSERT_FALSE(resp.body.empty());

    json response = json::parse(resp.body);
    std::cout << response.dump(4) <<"\n";
    std::string presigned_url = response["upload_url"].get<std::string>();

    // 2. 调用异步上传接口put_file_stream_async
    auto task = put_file_stream_async(
        presigned_url,
        path,
        [&](bool ok, const HttpResponse& resp)
        {
            std::lock_guard<std::mutex> lk(mtx);
            done = true;
            upload_ok = ok;
            upload_resp = resp;
            cv.notify_one();
        }
    );

    // 等待异步任务完成，超时30s
    std::unique_lock<std::mutex> lk(mtx);
    auto wait_ok = cv.wait_for(lk, std::chrono::seconds(30), [&](){return done;});
    ASSERT_TRUE(wait_ok) << "Async upload timeout!";

    ASSERT_TRUE(upload_ok) << "Async upload failed. err=" << upload_resp.error_msg;
    ASSERT_EQ(upload_resp.status_code,200);

    std::filesystem::remove(path);
}

// 可选：异步任务取消测试（集成环境）
TEST(HttpAsyncIntegrationTest, AsyncUploadCancel)
{
    CurlGlobalGuard guard;
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    bool upload_ok = true;
    HttpResponse upload_resp;

    const std::string path = "intergration_cancel.bin";
    // 创建稍大一点文件，方便中途取消
    {
        std::ofstream f(path, std::ios::binary);
        std::string buf(1024*1024*2, 'A');
        f.write(buf.data(), buf.size());
    }

    json req;
    req["object_key"] = path;
    req["file_size"] = fileutil::get_size(path);
    req["sha256_hash"] = hash::sha256_file(path);
    std::string url = "http://127.0.0.1:8000/api/presign/upload";
    auto resp = post_json(url, req.dump());
    ASSERT_TRUE(resp.ok);
    json response = json::parse(resp.body);
    std::string presigned_url = response["upload_url"].get<std::string>();

    auto task = put_file_stream_async(
        presigned_url,
        path,
        [&](bool ok, const HttpResponse& resp)
        {
            std::lock_guard<std::mutex> lk(mtx);
            done = true;
            upload_ok = ok;
            upload_resp = resp;
            cv.notify_one();
        }
    );
    // 立刻取消任务
    task->cancel();

    std::unique_lock<std::mutex> lk(mtx);
    cv.wait_for(lk, std::chrono::seconds(10), [&](){return done;});
    ASSERT_FALSE(upload_ok);

    std::filesystem::remove(path);
}

TEST(HttpAsyncIntegrationTest, AsyncDownloadWithPresign) {
    CurlGlobalGuard guard;
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    bool download_ok = false;
    HttpResponse download_resp;

    const std::string object_key = "intergration_async.txt";
    const std::string upload_path = object_key;
    const std::string save_path = "download_async.txt";

    // 自建上传源文件（不依赖前序用例：AsyncUploadWithPresign 结束时会
    // remove 同名文件，gtest 按定义顺序执行，本用例不能假设文件仍存在）
    {
        std::ofstream f(upload_path, std::ios::binary);
        f.write("hello fivecore async download", 29);
    }

    json req;
    req["object_key"] = object_key;
    req["file_size"] = fileutil::get_size(object_key);
    req["sha256_hash"] = hash::sha256_file(object_key);

    std::string presign_url = "http://127.0.0.1:8000/api/presign/upload";
    auto presign_resp = post_json(presign_url,req.dump());
    ASSERT_TRUE(presign_resp.ok);
    ASSERT_EQ(presign_resp.status_code,200);
    ASSERT_FALSE(presign_resp.body.empty());

    auto body_resp = json::parse(presign_resp.body);
    auto upload_url = body_resp["upload_url"].get<std::string>();

    bool upload_ok = false;
    HttpResponse upload_resp;
    {
        auto task = put_file_stream_async(
            upload_url,
            upload_path,
            [&](bool ok, const HttpResponse& resp)
            {
                std::lock_guard<std::mutex> lk(mtx);
                done = true;
                upload_ok = ok;
                upload_resp = resp;
                cv.notify_one();
            }
        );
        std::unique_lock<std::mutex> lk(mtx);
        auto wait_ok = cv.wait_for(lk, std::chrono::seconds(30), [&](){return done;});
        ASSERT_TRUE(wait_ok) << "Async upload timeout!";
        ASSERT_TRUE(upload_ok) << "Async upload failed. err=" << upload_resp.error_msg;
        ASSERT_EQ(upload_resp.status_code,200);
    }

    // ========== 关键：重置done，给下载阶段用 ==========
    {
        std::lock_guard<std::mutex> lk(mtx);
        done = false;
    }

    // 1. 请求后端获取下载预签名URL（复用你原有download逻辑）
    std::string url = "http://127.0.0.1:8000/api/presign/download";
    std::vector<std::pair<std::string,std::string>> query = {{"object_key", object_key}};
    auto resp = get(url, query);

    ASSERT_TRUE(resp.ok);
    ASSERT_EQ(resp.status_code,200);
    json response = json::parse(resp.body);
    std::string download_url = response["download_url"].get<std::string>();

    // 2. 异步下载文件
    auto task = get_file_stream_async(
        download_url,
        save_path,
        [&](bool ok, const HttpResponse& resp)
        {
            std::lock_guard<std::mutex> lk(mtx);
            done = true;
            download_ok = ok;
            download_resp = resp;
            cv.notify_one();
        }
    );

    // 等待下载完成，超时30s
    std::unique_lock<std::mutex> lk(mtx);
    auto wait_ok = cv.wait_for(lk, std::chrono::seconds(30), [&](){return done;});
    ASSERT_TRUE(wait_ok) << "Async download timeout";

    ASSERT_TRUE(download_ok) << "Async download failed. err=" << download_resp.error_msg;
    ASSERT_EQ(download_resp.status_code,200);

    // 校验哈希：对比原上传文件 和 下载后的文件hash一致
    std::string original_hash = hash::sha256_file(object_key);
    std::string downloaded_hash = hash::sha256_file(save_path);
    ASSERT_EQ(original_hash, downloaded_hash);

    std::filesystem::remove(save_path);
    std::filesystem::remove(upload_path);
}



#endif

} // namespace fivecore::http::test
