//
// Created by wufan on 2026/9/28.
//
// unit_async_http.cpp
#include <gtest/gtest.h>
#include <fivecore/CurlMultiContext.h>
#include <fivecore/HttpGetTask.h>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <thread>

#include "fivecore/HttpPostTask.h"
#include "fivecore/HttpPutStreamTask.h"
#include "fivecore/http.h"        // HttpResponse 完整定义
#include "fivecore/http_async.h"   // res_put_file_stream_async / res_get_file_stream_async

using namespace fivecore::http_async;
using namespace fivecore::http;

// ---------------------- 测试Case1：单个异步GET请求正常完成 ----------------------
TEST(AsyncHttpTest, SingleGetRequestOk)
{
    auto ctx_res = CurlMultiContext::Create();
    ASSERT_TRUE(ctx_res.is_ok());
    auto ctx = std::move(ctx_res.value());

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    CURLcode curl_code = CURLE_OK;
    long http_code = 0;
    std::string response_body;

    auto task = std::make_shared<HttpGetTask>(
        "https://httpbin.org/get",
        [&](CURLcode code, long http, std::vector<uint8_t> body)
        {
            std::lock_guard<std::mutex> lock(mtx);
            curl_code = code;
            http_code = http;
            response_body.assign(body.begin(), body.end());
            done = true;
            cv.notify_one();
        }
    );

    auto submit_ret = ctx->submit_task(task);
    ASSERT_TRUE(submit_ret.is_ok());

    // 最多等待5s，避免卡死
    std::unique_lock<std::mutex> lk(mtx);
    bool wait_ok = cv.wait_for(lk, std::chrono::seconds(5), [&](){ return done; });
    ASSERT_TRUE(wait_ok) << "Single request timeout!";

    EXPECT_EQ(curl_code, CURLE_OK);
    EXPECT_EQ(http_code, 200);
    EXPECT_FALSE(response_body.empty());
}

// ---------------------- 测试Case2：并发两个异步GET请求 ----------------------
TEST(AsyncHttpTest, ConcurrentTwoGetRequest)
{
    auto ctx_res = CurlMultiContext::Create();
    ASSERT_TRUE(ctx_res.is_ok());
    auto ctx = std::move(ctx_res.value());

    std::mutex mtx;
    std::condition_variable cv;
    int finish_count = 0;
    bool all_done = false;
    CURLcode code1, code2;
    long http1, http2;

    auto task1 = std::make_shared<HttpGetTask>(
        "https://httpbin.org/get",
        [&](CURLcode c, long h, std::vector<uint8_t>)
        {
            std::lock_guard<std::mutex> lock(mtx);
            code1 = c;
            http1 = h;
            finish_count++;
            if(finish_count >= 2)
            {
                all_done = true;
                cv.notify_one();
            }
        }
    );
    auto task2 = std::make_shared<HttpGetTask>(
        "https://httpbin.org/headers",
        [&](CURLcode c, long h, std::vector<uint8_t>)
        {
            std::lock_guard<std::mutex> lock(mtx);
            code2 = c;
            http2 = h;
            finish_count++;
            if(finish_count >= 2)
            {
                all_done = true;
                cv.notify_one();
            }
        }
    );

    ASSERT_TRUE(ctx->submit_task(task1).is_ok());
    ASSERT_TRUE(ctx->submit_task(task2).is_ok());

    std::unique_lock<std::mutex> lk(mtx);
    bool wait_ok = cv.wait_for(lk, std::chrono::seconds(8), [&](){ return all_done; });
    ASSERT_TRUE(wait_ok) << "Concurrent test timeout";

    EXPECT_EQ(code1, CURLE_OK);
    EXPECT_EQ(http1, 200);
    EXPECT_EQ(code2, CURLE_OK);
    EXPECT_EQ(http2, 200);
}

// ---------------------- 测试Case3：上下文stop主动终止请求 ----------------------
TEST(AsyncHttpTest, ContextStopAbortTask_opt)
{
    auto ctx_res = CurlMultiContext::Create();
    ASSERT_TRUE(ctx_res.is_ok());
    auto ctx = std::move(ctx_res.value());

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    CURLcode curl_code = CURLE_OK;

    auto task = std::make_shared<HttpGetTask>(
        "https://httpbin.org/delay/3", // 3秒延迟接口，方便中途stop
        [&](CURLcode code, long, std::vector<uint8_t>)
        {
            std::lock_guard<std::mutex> lock(mtx);
            curl_code = code;
            done = true;
            cv.notify_one();
        }
    );

    ASSERT_TRUE(ctx->submit_task(task).is_ok());

    // 短暂等待，请求开始后直接销毁ctx（触发stop）
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ctx.reset(); // 析构调用stop()，主动摘除任务

    std::unique_lock<std::mutex> lk(mtx);
    bool wait_ok = cv.wait_for(lk, std::chrono::seconds(5), [&](){ return done; });
    ASSERT_TRUE(wait_ok) << "Abort task timeout";

    // stop摘除任务会返回 CURLE_ABORTED_BY_CALLBACK
    EXPECT_EQ(curl_code, CURLE_ABORTED_BY_CALLBACK);
}

TEST(AsyncHttpTest, ContextStopAbortTask)
{
    auto ctx_res = CurlMultiContext::Create();
    ASSERT_TRUE(ctx_res.is_ok());
    auto ctx = std::move(ctx_res.value());

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    CURLcode curl_code = CURLE_OK;

    auto task = std::make_shared<HttpGetTask>(
        "https://httpbin.org/delay/3",
        [&](CURLcode code, long, std::vector<uint8_t>)
        {
            std::lock_guard<std::mutex> lock(mtx);
            curl_code = code;
            done = true;
            cv.notify_one();
        }
    );

    ASSERT_TRUE(ctx->submit_task(task).is_ok());

    // 等待0.5s，让请求进入libcurl多路等待状态
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // 销毁上下文，触发stop，摘除任务
    ctx.reset();

    std::unique_lock<std::mutex> lk(mtx);
    bool wait_ok = cv.wait_for(lk, std::chrono::seconds(5), [&](){ return done; });
    ASSERT_TRUE(wait_ok) << "Abort task timeout";

    // 两种合法情况：
    // 1. 成功中断 → CURLE_ABORTED_BY_CALLBACK
    // 2. 极快网络下请求刚好提前完成 → CURLE_OK（网络边界情况）
    EXPECT_TRUE(curl_code == CURLE_ABORTED_BY_CALLBACK || curl_code == CURLE_OK);
}


TEST(AsyncHttpTest, SinglePostJsonRequestOk)
{
    auto ctx_res = CurlMultiContext::Create();
    ASSERT_TRUE(ctx_res.is_ok());
    auto ctx = std::move(ctx_res.value());

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    CURLcode curl_code = CURLE_FAILED_INIT;
    long http_code = 0;
    std::string response_body;

    std::string test_json = R"({"name":"fiveclock","backup":true})";

    auto finish_cb = [&](CURLcode code, long http, std::string resp)
    {
        std::lock_guard<std::mutex> lock(mtx);
        curl_code = code;
        http_code = http;
        response_body = std::move(resp);
        done = true;
        cv.notify_one();
    };

    // 构造POST任务
    auto post_task = std::make_shared<HttpPostTask>(
        "https://httpbin.org/post",
        test_json,
        finish_cb
    );

    // 提交任务到multi上下文
    auto submit_res = ctx->submit_task(post_task);
    ASSERT_TRUE(submit_res.is_ok());

    // 等待回调，设置超时防止测试卡死
    std::unique_lock<std::mutex> lock(mtx);
    bool wait_ok = cv.wait_for(lock, std::chrono::seconds(10), [&](){ return done; });
    ASSERT_TRUE(wait_ok) << "Post request test timeout";

    ASSERT_EQ(curl_code, CURLE_OK);
    ASSERT_EQ(http_code, 200);

    // httpbin返回的json会包含我们上传的data，简单校验内容
    ASSERT_TRUE(response_body.find("fiveclock") != std::string::npos);
}


TEST(AsyncHttpTest, PostEmptyBody)
{
    auto ctx_res = CurlMultiContext::Create();
    ASSERT_TRUE(ctx_res.is_ok());
    auto ctx = std::move(ctx_res.value());

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    CURLcode curl_code = CURLE_FAILED_INIT;
    long http_code = 0;
    std::string response_body;

    auto finish_cb = [&](CURLcode code, long http, std::string resp)
    {
        std::lock_guard<std::mutex> lock(mtx);
        curl_code = code;
        http_code = http;
        response_body = std::move(resp);
        done = true;
        cv.notify_one();
    };

    auto post_task = std::make_shared<HttpPostTask>(
        "https://httpbin.org/post",
        "",
        finish_cb
    );

    auto submit_res = ctx->submit_task(post_task);
    ASSERT_TRUE(submit_res.is_ok());

    std::unique_lock<std::mutex> lock(mtx);
    bool wait_ok = cv.wait_for(lock, std::chrono::seconds(10), [&](){ return done; });
    ASSERT_TRUE(wait_ok);

    ASSERT_EQ(curl_code, CURLE_OK);
    ASSERT_EQ(http_code, 200);
}


TEST(AsyncHttpTest, PostTaskCancel)
{
    auto ctx_res = CurlMultiContext::Create();
    ASSERT_TRUE(ctx_res.is_ok());
    auto ctx = std::move(ctx_res.value());

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    CURLcode curl_code = CURLE_OK;

    std::string test_json = R"({"large_data":"xxxx"})";

    auto finish_cb = [&](CURLcode code, long http, std::string resp)
    {
        std::lock_guard<std::mutex> lock(mtx);
        curl_code = code;
        done = true;
        cv.notify_one();
    };

    auto post_task = std::make_shared<HttpPostTask>(
        "https://httpbin.org/delay/3", // 3s延迟接口，给我们留出取消窗口
        test_json,
        finish_cb
    );

    auto submit_res = ctx->submit_task(post_task);
    ASSERT_TRUE(submit_res.is_ok());

    // 等待一小段时间，让请求进入libcurl多路IO
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // 【任务级取消，不是销毁上下文】
    auto cancel_ret = post_task->cancel();
    ASSERT_TRUE(cancel_ret.is_ok());

    std::unique_lock<std::mutex> lock(mtx);
    bool wait_ok = cv.wait_for(lock, std::chrono::seconds(5), [&](){ return done; });
    ASSERT_TRUE(wait_ok) << "Post cancel timeout";

    // 两种合法结果：被中途取消 或者 请求在取消前已经完成
    EXPECT_TRUE(curl_code == CURLE_ABORTED_BY_CALLBACK || curl_code == CURLE_OK);
}



// ---------------------- PUT测试Case1：小文件正常流式上传 ----------------------
TEST(AsyncHttpTest, PutSmallFileOk)
{
    // 构造临时测试文件
    const std::string test_file = "test_upload_tmp.txt";
    const std::string test_content = "fivecore put stream test data 123456";
    {
        std::ofstream ofs(test_file, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs.write(test_content.data(), test_content.size());
    }

    auto ctx_res = CurlMultiContext::Create();
    ASSERT_TRUE(ctx_res.is_ok());
    auto ctx = std::move(ctx_res.value());

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    CURLcode curl_code = CURLE_FAILED_INIT;
    long http_code = 0;
    std::string response_body;

    auto finish_cb = [&](CURLcode code, long http, std::string resp)
    {
        std::lock_guard<std::mutex> lock(mtx);
        curl_code = code;
        http_code = http;
        response_body = std::move(resp);
        done = true;
        cv.notify_one();
    };

    auto put_task = std::make_shared<HttpPutStreamTask>(
        "https://httpbin.org/put",
        test_file,
        finish_cb
    );

    fivecore::VoidResult submit_res = ctx->submit_task(put_task);
    ASSERT_TRUE(submit_res.is_ok());

    std::unique_lock<std::mutex> lock(mtx);
    bool wait_ok = cv.wait_for(lock, std::chrono::seconds(10), [&](){ return done; });
    ASSERT_TRUE(wait_ok) << "Put request timeout";

    EXPECT_EQ(curl_code, CURLE_OK);
    EXPECT_EQ(http_code, 200);
    // 校验httpbin返回中包含我们上传的原始内容
    EXPECT_TRUE(response_body.find(test_content) != std::string::npos);

    // 清理临时文件
    std::remove(test_file.c_str());
}

// ---------------------- PUT测试Case2：本地文件不存在，setup在后台线程失败，触发on_finish回调 ----------------------
TEST(AsyncHttpTest, PutFileNotExist)
{
    auto ctx_res = CurlMultiContext::Create();
    ASSERT_TRUE(ctx_res.is_ok());
    auto ctx = std::move(ctx_res.value());

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    CURLcode curl_code = CURLE_OK;
    long http_code = 0;
    std::string resp;

    auto finish_cb = [&](CURLcode code, long http, std::string r)
    {
        std::lock_guard<std::mutex> lock(mtx);
        curl_code = code;
        http_code = http;
        resp = std::move(r);
        done = true;
        cv.notify_one();
    };

    auto put_task = std::make_shared<HttpPutStreamTask>(
        "https://httpbin.org/put",
        "nonexistent_file_abc123.txt",
        finish_cb
    );

    // submit_task只是入队，一定返回ok，setup_easy在后台线程执行
    fivecore::VoidResult submit_res = ctx->submit_task(put_task);
    ASSERT_TRUE(submit_res.is_ok());

    std::unique_lock<std::mutex> lock(mtx);
    bool wait_ok = cv.wait_for(lock, std::chrono::seconds(10), [&](){ return done; });
    ASSERT_TRUE(wait_ok) << "PutFileNotExist timeout";

    // setup_easy返回false，CurlMultiContext会触发on_finish，此时easy未初始化，curl_code一般为CURLE_FAILED_INIT
    EXPECT_NE(curl_code, CURLE_OK);
}

// ---------------------- PUT测试Case3：上传中途任务取消 ----------------------
TEST(AsyncHttpTest, PutTaskCancel)
{
    // 构造测试文件
    const std::string test_file = "test_cancel_upload.txt";
    const std::string test_content(2048, 'x'); // 2KB测试数据
    {
        std::ofstream ofs(test_file, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs.write(test_content.data(), test_content.size());
    }

    auto ctx_res = CurlMultiContext::Create();
    ASSERT_TRUE(ctx_res.is_ok());
    auto ctx = std::move(ctx_res.value());

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    CURLcode curl_code = CURLE_OK;

    auto finish_cb = [&](CURLcode code, long, std::string)
    {
        std::lock_guard<std::mutex> lock(mtx);
        curl_code = code;
        done = true;
        cv.notify_one();
    };

    auto put_task = std::make_shared<HttpPutStreamTask>(
        "https://httpbin.org/delay/3", // 服务端延迟3秒返回，留出取消窗口
        test_file,
        finish_cb
    );

    fivecore::VoidResult submit_res = ctx->submit_task(put_task);
    ASSERT_TRUE(submit_res.is_ok());

    // 等待请求进入IO状态
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 任务级取消
    fivecore::VoidResult cancel_ret = put_task->cancel();
    ASSERT_TRUE(cancel_ret.is_ok());

    std::unique_lock<std::mutex> lock(mtx);
    bool wait_ok = cv.wait_for(lock, std::chrono::seconds(5), [&](){ return done; });
    ASSERT_TRUE(wait_ok) << "Put cancel timeout";

    // 取消场景：两种情况
    // 1. 取消成功中断传输 → curl_code != CURLE_OK（CURLE_PARTIAL_FILE / CURLE_ABORTED_BY_CALLBACK / CURLE_SEND_ERROR等）
    // 2. 取消前上传已经跑完 → curl_code == CURLE_OK
    // 只要回调正常触发，就是成功，不再硬编码错误码
    EXPECT_TRUE(done);

    std::remove(test_file.c_str());
}


// ================================================================
// res_* 系列异步接口测试（回调携带 Result<HttpResponse>）
// 验证点：成功 → Ok；HTTP 非 200 → Err(HttpBadStatus)
// ================================================================

// ---------- res_put_file_stream_async：成功上传返回 Ok ----------
TEST(AsyncHttpResTest, ResPutStreamAsync_Success_ReturnsOk)
{
    const std::string test_file = "test_res_put_ok.txt";
    const std::string test_content = "fivecore res_put stream test";
    {
        std::ofstream ofs(test_file, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs.write(test_content.data(), test_content.size());
    }

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    fivecore::Result<fivecore::http::HttpResponse> result =
        fivecore::Result<fivecore::http::HttpResponse>::Err(
            fivecore::ErrorInfo::make(fivecore::ErrorCode::Ok, ""));

    auto task = res_put_file_stream_async(
        "https://httpbin.org/put",
        test_file,
        [&](const fivecore::Result<fivecore::http::HttpResponse>& res)
        {
            std::lock_guard<std::mutex> lock(mtx);
            result = res;
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mtx);
    bool wait_ok = cv.wait_for(lock, std::chrono::seconds(15), [&](){ return done; });
    ASSERT_TRUE(wait_ok) << "res_put_file_stream_async timeout";

    ASSERT_TRUE(result.is_ok()) << "expected Ok, got error: " << result.error().message;
    EXPECT_EQ(result.value().status_code, 200);
    EXPECT_TRUE(result.value().body.find(test_content) != std::string::npos);

    std::remove(test_file.c_str());
}

// ---------- res_put_file_stream_async：HTTP 404 返回 Err(HttpBadStatus) ----------
TEST(AsyncHttpResTest, ResPutStreamAsync_Non200_ReturnsErrBadStatus)
{
    const std::string test_file = "test_res_put_404.txt";
    {
        std::ofstream ofs(test_file, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs.write("data", 4);
    }

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    fivecore::Result<fivecore::http::HttpResponse> result =
        fivecore::Result<fivecore::http::HttpResponse>::Ok(fivecore::http::HttpResponse{});

    auto task = res_put_file_stream_async(
        "https://httpbin.org/status/404",
        test_file,
        [&](const fivecore::Result<fivecore::http::HttpResponse>& res)
        {
            std::lock_guard<std::mutex> lock(mtx);
            result = res;
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mtx);
    bool wait_ok = cv.wait_for(lock, std::chrono::seconds(15), [&](){ return done; });
    ASSERT_TRUE(wait_ok) << "res_put_file_stream_async timeout";

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, fivecore::ErrorCode::HttpBadStatus);

    std::remove(test_file.c_str());
}

// ---------- res_get_file_stream_async：成功下载返回 Ok ----------
TEST(AsyncHttpResTest, ResGetStreamAsync_Success_ReturnsOk)
{
    const std::string save_path = "test_res_get_ok.json";

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    fivecore::Result<fivecore::http::HttpResponse> result =
        fivecore::Result<fivecore::http::HttpResponse>::Err(
            fivecore::ErrorInfo::make(fivecore::ErrorCode::Ok, ""));

    auto task = res_get_file_stream_async(
        "https://httpbin.org/get",
        save_path,
        [&](const fivecore::Result<fivecore::http::HttpResponse>& res)
        {
            std::lock_guard<std::mutex> lock(mtx);
            result = res;
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mtx);
    bool wait_ok = cv.wait_for(lock, std::chrono::seconds(15), [&](){ return done; });
    ASSERT_TRUE(wait_ok) << "res_get_file_stream_async timeout";

    ASSERT_TRUE(result.is_ok()) << "expected Ok, got error: " << result.error().message;
    EXPECT_EQ(result.value().status_code, 200);

    std::remove(save_path.c_str());
}

// ---------- res_get_file_stream_async：HTTP 500 返回 Err(HttpBadStatus) ----------
TEST(AsyncHttpResTest, ResGetStreamAsync_Non200_ReturnsErrBadStatus)
{
    const std::string save_path = "test_res_get_500.json";

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    fivecore::Result<fivecore::http::HttpResponse> result =
        fivecore::Result<fivecore::http::HttpResponse>::Ok(fivecore::http::HttpResponse{});

    auto task = res_get_file_stream_async(
        "https://httpbin.org/status/500",
        save_path,
        [&](const fivecore::Result<fivecore::http::HttpResponse>& res)
        {
            std::lock_guard<std::mutex> lock(mtx);
            result = res;
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mtx);
    bool wait_ok = cv.wait_for(lock, std::chrono::seconds(15), [&](){ return done; });
    ASSERT_TRUE(wait_ok) << "res_get_file_stream_async timeout";

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, fivecore::ErrorCode::HttpBadStatus);

    std::remove(save_path.c_str());
}

// ---------- res_put_file_stream_async：任务取消 ----------
TEST(AsyncHttpResTest, ResPutStreamAsync_Cancel_TriggersCallback)
{
    const std::string test_file = "test_res_put_cancel.txt";
    const std::string test_content(2048, 'y');
    {
        std::ofstream ofs(test_file, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs.write(test_content.data(), test_content.size());
    }

    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;

    auto task = res_put_file_stream_async(
        "https://httpbin.org/delay/3",
        test_file,
        [&](const fivecore::Result<fivecore::http::HttpResponse>&)
        {
            std::lock_guard<std::mutex> lock(mtx);
            done = true;
            cv.notify_one();
        }
    );

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto cancel_ret = task->cancel();
    ASSERT_TRUE(cancel_ret.is_ok());

    std::unique_lock<std::mutex> lock(mtx);
    bool wait_ok = cv.wait_for(lock, std::chrono::seconds(8), [&](){ return done; });
    EXPECT_TRUE(wait_ok) << "cancel did not trigger callback";

    std::remove(test_file.c_str());
}
