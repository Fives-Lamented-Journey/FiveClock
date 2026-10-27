
// unit_task_scheduler.cpp — TaskScheduler 单元测试
//
// 不依赖外网，使用两类确定性本地端点：
// 1. 黑洞监听器：TCP 三次握手成功但服务端永不响应 → 请求无限挂起（测排队/超时/取消）
// 2. 已关闭端口：绑定取得端口号后立即关闭 → 连接秒拒 CURLE_COULDNT_CONNECT（测快速完成/FIFO）
#include <gtest/gtest.h>
#include "fivecore/TaskScheduler.h"
#include "fivecore/HttpGetTask.h"

using fivecore::http_async::CurlMultiContext;
using fivecore::http_async::HttpGetTask;
using fivecore::scheduler::FinishReason;
using fivecore::scheduler::SchedulerConfig;
using fivecore::scheduler::TaskCompletion;
using fivecore::scheduler::TaskId;
using fivecore::scheduler::TaskScheduler;
using fivecore::scheduler::kInvalidTaskId;

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socket_t = SOCKET;
  static constexpr socket_t kInvalidSocket = INVALID_SOCKET;
  using socklen_arg_t = int;
  static void close_socket(socket_t s) { closesocket(s); }
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  using socket_t = int;
  static constexpr socket_t kInvalidSocket = -1;
  using socklen_arg_t = socklen_t;
  static void close_socket(socket_t s) { ::close(s); }
#endif

namespace
{
/**
 * @brief 本地黑洞探测端点
 *
 * blackhole_url()：TCP 可连上但永不回包，任务挂起直到被取消/超时。
 * refused_url()：绑定取号后立刻关闭的端口，连接被秒拒，任务快速失败。
 */
class LocalProbe
{
public:
    LocalProbe()
    {
#ifdef _WIN32
        WSADATA d;
        WSAStartup(MAKEWORD(2, 2), &d);
#endif
        listener_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listener_ != kInvalidSocket)
        {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.sin_port = 0;
            if (::bind(listener_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0 &&
                ::listen(listener_, 16) == 0)
            {
                socklen_arg_t len = sizeof(addr);
                ::getsockname(listener_, reinterpret_cast<sockaddr*>(&addr), &len);
                port_ = ntohs(addr.sin_port);
                th_ = std::thread([this] { accept_loop(); });
            }
        }
    }

    ~LocalProbe()
    {
        stop_ = true;
        if (listener_ != kInvalidSocket)
        {
            close_socket(listener_);
        }
        if (th_.joinable())
        {
            th_.join();
        }
        {
            std::lock_guard<std::mutex> lk(mtx_);
            for (auto s : held_)
            {
                close_socket(s);
            }
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }

    /// 黑洞地址：连接成功但永不响应
    std::string blackhole_url() const
    {
        return "http://127.0.0.1:" + std::to_string(port_) + "/";
    }

    /// 秒拒地址：绑定取号后立刻关闭，连接几乎必然被拒绝
    std::string refused_url()
    {
        socket_t s = ::socket(AF_INET, SOCK_STREAM, 0);
        if (s == kInvalidSocket)
        {
            return blackhole_url(); // 兜底（正常不会走到）
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        if (::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
        {
            close_socket(s);
            return blackhole_url();
        }
        socklen_arg_t len = sizeof(addr);
        ::getsockname(s, reinterpret_cast<sockaddr*>(&addr), &len);
        const uint16_t p = ntohs(addr.sin_port);
        close_socket(s);
        return "http://127.0.0.1:" + std::to_string(p) + "/";
    }

private:
    /// 接收所有连接并保持（不读取、不响应），制造"请求挂起"效果
    void accept_loop()
    {
        while (!stop_.load())
        {
            socket_t s = ::accept(listener_, nullptr, nullptr);
            if (s == kInvalidSocket)
            {
                break;
            }
            std::lock_guard<std::mutex> lk(mtx_);
            held_.push_back(s); // 收下但一个字节也不回
        }
    }

    socket_t listener_ = kInvalidSocket;
    uint16_t port_ = 0;
    std::thread th_;
    std::atomic<bool> stop_{false};
    std::mutex mtx_;
    std::vector<socket_t> held_;
};

/// 测试公共骨架：创建独立 ctx + 调度器（析构顺序：先调度器后 ctx）
struct SchedFixture
{
    std::unique_ptr<CurlMultiContext> ctx;
    std::unique_ptr<TaskScheduler> sched;

    SchedFixture(SchedulerConfig cfg = {})
    {
        auto ctx_res = CurlMultiContext::Create();
        if (!ctx_res.is_ok())
        {
            return;
        }
        ctx = std::move(ctx_res.value());
        auto sched_res = TaskScheduler::Create(ctx.get(), cfg);
        if (sched_res.is_ok())
        {
            sched = std::move(sched_res.value());
        }
    }

    bool valid() const { return ctx && sched; }
};

/// 构造一个快速失败的 GET 任务（连接秒拒）
std::shared_ptr<HttpGetTask> make_refused_task(LocalProbe& probe)
{
    return std::make_shared<HttpGetTask>(
        probe.refused_url(),
        [](CURLcode, long, std::vector<uint8_t>) {});
}

/// 构造一个黑洞 GET 任务（挂起不回包）
std::shared_ptr<HttpGetTask> make_blackhole_task(LocalProbe& probe)
{
    return std::make_shared<HttpGetTask>(
        probe.blackhole_url(),
        [](CURLcode, long, std::vector<uint8_t>) {});
}
} // namespace

// ---------------------- Case1：并发上限与排队 ----------------------
TEST(TaskSchedulerTest, ConcurrencyLimit)
{
    LocalProbe probe;
    SchedulerConfig cfg;
    cfg.max_concurrency = 2;
    SchedFixture f(cfg);
    ASSERT_TRUE(f.valid());

    std::mutex mtx;
    std::condition_variable cv;
    std::size_t done_count = 0;

    for (int i = 0; i < 5; ++i)
    {
        auto r = f.sched->submit(make_blackhole_task(probe),
                                 [&](const TaskCompletion&)
                                 {
                                     std::lock_guard<std::mutex> lk(mtx);
                                     ++done_count;
                                     cv.notify_all();
                                 });
        ASSERT_TRUE(r.is_ok());
    }

    // 等调度生效：2 个进入网络层，3 个排队
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    {
        const auto s = f.sched->stats();
        EXPECT_EQ(s.running, 2u);
        EXPECT_EQ(s.waiting, 3u);
    }

    // 全部取消排空（1、2 运行中走清扫；3、4、5 排队中同步移除）
    for (TaskId id = 1; id <= 5; ++id)
    {
        EXPECT_TRUE(f.sched->cancel(id).is_ok());
    }
    std::unique_lock<std::mutex> lk(mtx);
    ASSERT_TRUE(cv.wait_for(lk, std::chrono::seconds(3),
                            [&] { return done_count == 5; }))
        << "expect all 5 completions, got " << done_count;
}

// ---------------------- Case2：FIFO 顺序 ----------------------
TEST(TaskSchedulerTest, FifoOrder)
{
    LocalProbe probe;
    SchedulerConfig cfg;
    cfg.max_concurrency = 1;
    SchedFixture f(cfg);
    ASSERT_TRUE(f.valid());

    std::mutex mtx;
    std::condition_variable cv;
    std::vector<TaskId> order;

    for (int i = 0; i < 4; ++i)
    {
        auto r = f.sched->submit(make_refused_task(probe),
                                 [&](const TaskCompletion& tc)
                                 {
                                     std::lock_guard<std::mutex> lk(mtx);
                                     order.push_back(tc.id);
                                     if (order.size() == 4)
                                     {
                                         cv.notify_all();
                                     }
                                 });
        ASSERT_TRUE(r.is_ok());
        EXPECT_EQ(r.value(), static_cast<TaskId>(i + 1)); // ID 自 1 连续分配
    }

    std::unique_lock<std::mutex> lk(mtx);
    ASSERT_TRUE(cv.wait_for(lk, std::chrono::seconds(10),
                            [&] { return order.size() == 4; }));
    EXPECT_EQ(order, (std::vector<TaskId>{1, 2, 3, 4}));

    const auto s = f.sched->stats();
    EXPECT_EQ(s.failed, 4u);   // 连接秒拒 → 传输层失败
    EXPECT_EQ(s.succeeded, 0u);
    EXPECT_EQ(s.running, 0u);
    EXPECT_EQ(s.waiting, 0u);
}

// ---------------------- Case3：全局超时 ----------------------
TEST(TaskSchedulerTest, GlobalTimeout)
{
    LocalProbe probe;
    SchedulerConfig cfg;
    cfg.max_concurrency = 2;
    cfg.default_timeout = std::chrono::milliseconds(300);
    SchedFixture f(cfg);
    ASSERT_TRUE(f.valid());

    std::mutex mtx;
    std::condition_variable cv;
    TaskCompletion captured;

    const auto t0 = std::chrono::steady_clock::now();
    auto r = f.sched->submit(make_blackhole_task(probe),
                             [&](const TaskCompletion& tc)
                             {
                                 std::lock_guard<std::mutex> lk(mtx);
                                 captured = tc;
                                 cv.notify_all();
                             });
    ASSERT_TRUE(r.is_ok());

    std::unique_lock<std::mutex> lk(mtx);
    ASSERT_TRUE(cv.wait_for(lk, std::chrono::seconds(3),
                            [&] { return captured.id != kInvalidTaskId; }));
    const auto elapsed = std::chrono::steady_clock::now() - t0;

    EXPECT_EQ(captured.reason, FinishReason::TimedOut);
    EXPECT_LT(elapsed, std::chrono::seconds(2)); // 300ms 超时 + tick，远小于 2s
    EXPECT_EQ(f.sched->stats().timed_out, 1u);
}

// ---------------------- Case4：取消排队中的任务 ----------------------
TEST(TaskSchedulerTest, CancelQueuedTask)
{
    LocalProbe probe;
    SchedulerConfig cfg;
    cfg.max_concurrency = 1;
    SchedFixture f(cfg);
    ASSERT_TRUE(f.valid());

    std::atomic<bool> queued_cb_called{false};
    TaskId id_running = 0;
    TaskId id_queued = 0;

    // 任务1 占住唯一并发额度（黑洞挂起）
    auto r1 = f.sched->submit(make_blackhole_task(probe),
                              [](const TaskCompletion&) {});
    ASSERT_TRUE(r1.is_ok());
    id_running = r1.value();

    // 任务2 进入等待队列
    auto r2 = f.sched->submit(make_blackhole_task(probe),
                              [&](const TaskCompletion& tc)
                              {
                                  EXPECT_EQ(tc.reason, FinishReason::Cancelled);
                                  queued_cb_called = true;
                              });
    ASSERT_TRUE(r2.is_ok());
    id_queued = r2.value();

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    {
        const auto s = f.sched->stats();
        ASSERT_EQ(s.running, 1u);
        ASSERT_EQ(s.waiting, 1u);
    }

    // 取消排队任务：回调在本线程同步触发，cancel 返回后回调必然已执行
    EXPECT_TRUE(f.sched->cancel(id_queued).is_ok());
    EXPECT_TRUE(queued_cb_called.load());
    EXPECT_EQ(f.sched->stats().waiting, 0u);

    // 取消运行中任务，排空
    EXPECT_TRUE(f.sched->cancel(id_running).is_ok());
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (f.sched->stats().running != 0u && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    EXPECT_EQ(f.sched->stats().running, 0u);
    EXPECT_EQ(f.sched->stats().cancelled, 2u);

    // 对已完成/不存在的 ID 重复取消应返回 Err
    EXPECT_TRUE(f.sched->cancel(id_queued).is_err());
}

// ---------------------- Case5：取消运行中的任务 ----------------------
TEST(TaskSchedulerTest, CancelRunningTask)
{
    LocalProbe probe;
    SchedFixture f; // 默认配置：并发4、超时60s
    ASSERT_TRUE(f.valid());

    std::mutex mtx;
    std::condition_variable cv;
    TaskCompletion captured;

    auto r = f.sched->submit(make_blackhole_task(probe),
                             [&](const TaskCompletion& tc)
                             {
                                 std::lock_guard<std::mutex> lk(mtx);
                                 captured = tc;
                                 cv.notify_all();
                             });
    ASSERT_TRUE(r.is_ok());

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    ASSERT_EQ(f.sched->stats().running, 1u);

    EXPECT_TRUE(f.sched->cancel(r.value()).is_ok());

    std::unique_lock<std::mutex> lk(mtx);
    ASSERT_TRUE(cv.wait_for(lk, std::chrono::seconds(3),
                            [&] { return captured.id != kInvalidTaskId; }));
    EXPECT_EQ(captured.reason, FinishReason::Cancelled);
    EXPECT_EQ(f.sched->stats().cancelled, 1u);
}

// ---------------------- Case6：统计累计 + stop 后拒绝提交 ----------------------
TEST(TaskSchedulerTest, StatsAccumulateAndStopReject)
{
    LocalProbe probe;
    SchedFixture f;
    ASSERT_TRUE(f.valid());

    std::mutex mtx;
    std::condition_variable cv;
    std::size_t done_count = 0;

    for (int i = 0; i < 4; ++i)
    {
        auto r = f.sched->submit(make_refused_task(probe),
                                 [&](const TaskCompletion&)
                                 {
                                     std::lock_guard<std::mutex> lk(mtx);
                                     ++done_count;
                                     cv.notify_all();
                                 });
        ASSERT_TRUE(r.is_ok());
    }

    std::unique_lock<std::mutex> lk(mtx);
    ASSERT_TRUE(cv.wait_for(lk, std::chrono::seconds(10),
                            [&] { return done_count == 4; }));
    lk.unlock();

    const auto s = f.sched->stats();
    EXPECT_EQ(s.submitted, 4u);
    EXPECT_EQ(s.failed, 4u);
    EXPECT_EQ(s.cancelled, 0u);
    EXPECT_EQ(s.timed_out, 0u);
    EXPECT_EQ(s.running, 0u);
    EXPECT_EQ(s.waiting, 0u);

    // stop 之后提交恒返 Err
    EXPECT_TRUE(f.sched->stop().is_ok());
    EXPECT_TRUE(f.sched->stop().is_ok()); // 可重复调用

    auto late = f.sched->submit(make_refused_task(probe));
    EXPECT_TRUE(late.is_err());
}

// ---------------------- Case7：非法参数 ----------------------
TEST(TaskSchedulerTest, CreateRejectsBadInput)
{
    // ctx 为空
    auto r1 = TaskScheduler::Create(nullptr);
    EXPECT_TRUE(r1.is_err());

    // max_concurrency 为 0：需要一个真实 ctx 才能走到该检查
    SchedFixture f;
    ASSERT_TRUE(f.valid());

    SchedulerConfig bad;
    bad.max_concurrency = 0;
    auto r2 = TaskScheduler::Create(f.ctx.get(), bad);
    EXPECT_TRUE(r2.is_err());
}
