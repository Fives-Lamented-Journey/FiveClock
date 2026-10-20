#include "service/backup_service.h"
#include "fivecore/hash.h"
#include "fivecore/fileutil.h"
#include "fivecore/http.h"
#include "fivecore/http_async.h"   // 新增异步头
#include "fivecore/HttpPutStreamTask.h"
#include "fivecore/HttpGetstreamTask.h"
#include "fivecore/TaskScheduler.h" // 调度器：限流 + 全局超时 + 取消记账
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;
using namespace fivecore;

BackupService::BackupService(QObject *parent)
    : QObject(parent)
{
    // 创建调度器并挂到全局 CurlMultiContext（提供超时保护/取消/统计能力）。
    // m_running 已保证同时只有一个任务在跑，max_concurrency=2 仅留余量；
    // 创建失败则退化为原有直提模式（行为与集成前一致）
    auto *ctx = fivecore::http_async::get_multi_context();
    if (ctx)
    {
        fivecore::scheduler::SchedulerConfig cfg;
        cfg.max_concurrency = 2;
        auto res = fivecore::scheduler::TaskScheduler::Create(ctx, cfg);
        if (res.is_ok())
        {
            m_scheduler = std::move(res.value());
        }
        else
        {
            spdlog::warn("TaskScheduler 创建失败: {}，退化为直提模式", res.error().message);
        }
    }
}

BackupService::~BackupService()
{
    if (m_scheduler)
    {
        // 排队中任务以 Cancelled 结束，完成 hook 注销；
        // 运行中任务自身回调随后触发时 invokeMethod(this) 因对象销毁被 Qt 丢弃
        m_scheduler->stop();
    }
}

void BackupService::doBackup()
{
    // if (!checkRunning())
    // {
    //     return;
    // }
    //
    // QString localFilePath = "coreTime.db";
    // std::string path = localFilePath.toStdString();
    //
    // if (!fileutil::exists(path))
    // {
    //     spdlog::error("文件不存在");
    //     return;
    // }
    //
    // uint64_t fsize = fileutil::get_size(path);
    //
    // // 1 计算SHA256
    // std::string sha256;
    // if (!calcSha256(path, sha256))
    // {
    //     spdlog::error("hash256失败");
    //     return;
    // }
    //
    //
    // // 2 获取预签名上传URL
    // std::string uploadUrl;
    // std::string metaId;
    // if (!fetchUploadPresignedUrl(path, fsize, sha256, uploadUrl, metaId))
    // {
    //     spdlog::error("预签名获取失败");
    //     return;
    // }
    //
    // // 3 PUT流式上传到MinIO
    // if (!uploadFile(uploadUrl, path))
    // {
    //     spdlog::error("PUT上传失败");
    //     return;
    // }
    //
    // // 4 调用上传确认接口
    // if (!confirm(metaId,sha256))
    // {
    //     spdlog::error("上传确认失败");
    //     return;
    // }

    if (!checkRunning())
    {
        emit finished(false, "任务正在运行，拒绝新任务");
        return;
    }

    QString localFilePath = "coreTime.db";
    std::string path = localFilePath.toStdString();
    emit log(QString("开始备份，文件：%1").arg(localFilePath));

    if (!fileutil::exists(path))
    {
        spdlog::error("文件不存在");
        emit log("❌文件不存在");
        emit finished(false,"文件不存在");
        m_running = false;
        return;
    }

    uint64_t fsize = fileutil::get_size(path);
    emit log(QString("文件大小：%1 bytes").arg(fsize));
    emit progress(10);

    // 1 计算SHA256
    std::string sha256;
    if (!calcSha256(path, sha256))
    {
        spdlog::error("hash256失败");
        emit log("❌SHA256哈希计算失败");
        emit finished(false,"哈希计算失败");
        m_running = false;
        return;
    }
    emit log(QString("文件SHA256：%1").arg(QString::fromStdString(sha256)));
    emit progress(25);

    // 2 获取预签名上传URL
    std::string uploadUrl;
    std::string metaId;
    if (!fetchUploadPresignedUrl(path, fsize, sha256, uploadUrl, metaId))
    {
        spdlog::error("预签名获取失败");
        emit log("❌获取上传预签名URL失败");
        emit finished(false,"获取预签名失败");
        m_running = false;
        return;
    }
    emit log("获取预签名地址成功，开始上传文件");
    emit progress(40);

    // 3 PUT流式上传到MinIO
    if (!uploadFile(uploadUrl, path))
    {
        spdlog::error("PUT上传失败");
        emit log("❌MinIO文件上传失败");
        emit finished(false,"PUT上传失败");
        m_running = false;
        return;
    }
    emit progress(80);

    // 4 调用上传确认接口
    if (!confirm(metaId,sha256))
    {
        spdlog::error("上传确认失败");
        emit log("❌上传确认接口调用失败");
        emit finished(false,"上传确认失败");
        m_running = false;
        return;
    }

    emit progress(100);
    emit log("✅备份任务全部完成");
    emit finished(true,"");
    m_running = false;
}

void BackupService::doRestore()
{
    // std::string file = "coreTime.db";
    // std::string presign_url;
    // bool ok = fetchDownloadPresignedUrl(file,presign_url);
    // if (!ok)
    // {
    //     spdlog::error("预签名获取失败");
    //     return;
    // }
    // std::string outfilePath = "coreTime.db";
    // ok = downloadFile(presign_url,outfilePath);
    // if (!ok)
    // {
    //     spdlog::error("文件下载失败");
    //     return;
    // }
    // if (!checkRunning())
    // {
    //     return;
    // }
    //


    // std::string file = "coreTime.db";
    // std::string presign_url;
    // std::string expectSha;
    // bool ok = fetchDownloadPresignedUrl(file,presign_url, expectSha);
    // if (!ok)
    // {
    //     spdlog::error("预签名获取失败");
    //     return;
    // }
    // std::string outfilePath = "coreTime.db";
    // ok = downloadFile(presign_url,outfilePath);
    // if (!ok)
    // {
    //     spdlog::error("文件下载失败");
    //     return;
    // }
    // // 新增：校验哈希
    // std::string realSha;
    // if(!calcSha256(outfilePath, realSha))
    // {
    //     spdlog::error("恢复后文件哈希计算失败");
    //     return;
    // }
    // if(realSha != expectSha)
    // {
    //     spdlog::error("❌哈希校验不匹配！expect:{} real:{}", expectSha, realSha);
    //     return;
    // }
    // spdlog::info("✅恢复完成，哈希校验通过");


    if (!checkRunning())
    {
        emit finished(false, "任务正在运行，拒绝新任务");
        return;
    }

    std::string file = "coreTime.db";
    emit log(QString("开始恢复，目标文件：%1").arg(QString::fromStdString(file)));

    std::string presign_url;
    std::string expectSha;
    bool ok = fetchDownloadPresignedUrl(file,presign_url, expectSha);
    if (!ok)
    {
        spdlog::error("预签名获取失败");
        emit log("❌获取下载预签名URL失败");
        emit finished(false,"获取下载预签名失败");
        m_running = false;
        return;
    }

    emit log(QString("云端记录期望SHA256：%1").arg(QString::fromStdString(expectSha)));
    emit progress(20);

    std::string outfilePath = "coreTime.db";
    ok = downloadFile(presign_url,outfilePath);
    if (!ok)
    {
        spdlog::error("文件下载失败");
        emit log("❌文件流式下载失败");
        emit finished(false,"下载文件失败");
        m_running = false;
        return;
    }
    emit progress(70);
    emit log("下载完成，正在校验文件SHA256完整性……");

    // 新增：校验哈希
    std::string realSha;
    if(!calcSha256(outfilePath, realSha))
    {
        spdlog::error("恢复后文件哈希计算失败");
        emit log("❌恢复后本地文件哈希计算失败");
        emit finished(false,"本地哈希计算失败");
        m_running = false;
        return;
    }
    if(realSha != expectSha)
    {
        spdlog::error("❌哈希校验不匹配！expect:{} real:{}", expectSha, realSha);
        emit log(QString("❌哈希校验不匹配！期望:%1 实际:%2")
                     .arg(QString::fromStdString(expectSha))
                     .arg(QString::fromStdString(realSha)));
        emit finished(false,"文件哈希校验不匹配，文件已损坏");
        m_running = false;
        return;
    }

    emit progress(100);
    emit log("✅恢复完成，SHA256完整性校验通过！");
    emit finished(true,"");
    m_running = false;
}

bool BackupService::checkRunning()
{
    if(m_running)
    {
        spdlog::warn("任务正在运行，拒绝新任务");
        return false;
    }
    m_running = true;
    return true;
}

bool BackupService::calcSha256(const std::string& path, std::string& outSha)
{
    outSha = hash::sha256_file(path);
    return !outSha.empty();
}

bool BackupService::fetchUploadPresignedUrl(const std::string& filePath, uint64_t fileSize, const std::string& sha256,
                                     std::string& outUploadUrl, std::string& outMetaId)
{
    json reqBody = {
        {"object_key", filePath},
        {"file_size", fileSize},
        {"sha256_hash", sha256}
    };
    auto resp = http::post_json("http://127.0.0.1:8000/api/presign/upload", reqBody.dump());
    if (!resp.ok)
        return false;

    auto j = json::parse(resp.body);
    outUploadUrl = j["upload_url"];
    outMetaId = j["object_key"];
    return true;
}

bool BackupService::uploadFile(const std::string& presignedUrl, const std::string& filePath)
{
    auto resp = http::put_file_stream(presignedUrl, filePath);
    return resp.ok;
}

bool BackupService::fetchDownloadPresignedUrl(const std::string& file,std::string& presign, std::string& expectSha)
{
    // std::string path = file;
    // std::string url = "http://127.0.0.1:8000/api/presign/download";
    // std::vector<std::pair<std::string,std::string>> query = {{"object_key",path}};
    // auto resp = http::get(url,query);
    // presign = json::parse(resp.body)["download_url"].get<std::string>();
    // return resp.ok;
    std::string url = "http://127.0.0.1:8000/api/presign/download";
    std::vector<std::pair<std::string,std::string>> query = {{"object_key",file}};
    auto resp = http::get(url,query);
    if(!resp.ok) return false;
    auto j = json::parse(resp.body);
    presign = j["download_url"].get<std::string>();
    expectSha = j["sha256_hash"].get<std::string>();
    return true;
}

bool BackupService::downloadFile(const std::string& presignedUrl, const std::string& outfilePath)
{
    auto resp_download = http::get_file_stream(presignedUrl,outfilePath);
    return resp_download.ok;
}

bool BackupService::confirm(const std::string& metaId, const std::string& sha256)
{
    json reqBody = {
        {"object_key", metaId},
        {"sha256_hash", sha256}
    };
    auto resp = http::post_json("http://127.0.0.1:8000/api/upload/confirm", reqBody.dump());
    return resp.ok;
}

void BackupService::doBackupAsync()
{
    if (!checkRunning())
    {
        emit finished(false, "任务正在运行，拒绝新任务");
        return;
    }

    QString localFilePath = "coreTime.db";
    std::string path = localFilePath.toStdString();
    emit log(QString("【异步备份】开始备份，文件：%1").arg(localFilePath));

    if (!fileutil::exists(path))
    {
        spdlog::error("文件不存在");
        emit log("❌文件不存在");
        emit finished(false,"文件不存在");
        m_running = false;
        return;
    }

    uint64_t fsize = fileutil::get_size(path);
    emit log(QString("文件大小：%1 bytes").arg(fsize));
    emit progress(10);

    // 1 计算SHA256（同步，这步很快）
    std::string sha256;
    if (!calcSha256(path, sha256))
    {
        spdlog::error("hash256失败");
        emit log("❌SHA256哈希计算失败");
        emit finished(false,"哈希计算失败");
        m_running = false;
        return;
    }
    emit log(QString("文件SHA256：%1").arg(QString::fromStdString(sha256)));
    emit progress(25);

    // 2 获取预签名上传URL（同步HTTP调用）
    std::string uploadUrl;
    std::string metaId;
    if (!fetchUploadPresignedUrl(path, fsize, sha256, uploadUrl, metaId))
    {
        spdlog::error("预签名获取失败");
        emit log("❌获取上传预签名URL失败");
        emit finished(false,"获取预签名失败");
        m_running = false;
        return;
    }
    emit log("获取预签名地址成功，开始【异步PUT上传】");
    emit progress(40);

    // 3 异步流式上传到MinIO（经 TaskScheduler 调度：限流 + 全局超时 + 取消记账）
    {
        // 任务自身回调：适配层与 http::put_file_stream_async 内部一致，
        // 收尾投递回 service 线程执行，避免阻塞 IO 线程
        auto task_callback_wrapper = [this, metaId, sha256](CURLcode code, long http_code, std::string resp_body)
        {
            http::HttpResponse resp{};
            resp.status_code = http_code;
            resp.body = std::move(resp_body);
            resp.ok = (code == CURLE_OK);
            if (!resp.ok)
            {
                resp.error_msg = curl_easy_strerror(code);
            }

            const bool ok = resp.ok;
            const QString errMsg = ok ? QString()
                                      : QString::fromStdString(resp.error_msg);
            QMetaObject::invokeMethod(this, [this, ok, errMsg, metaId, sha256]() {
                finalizeBackup(ok, errMsg, metaId, sha256);
            }, Qt::QueuedConnection);
        };

        auto task = std::make_shared<fivecore::http_async::HttpPutStreamTask>(
            uploadUrl, path, std::move(task_callback_wrapper));

        std::lock_guard<std::mutex> lk(m_taskMutex);
        m_uploadTask = task;
        m_uploadSchedId = 0;
        if (m_scheduler)
        {
            // 调度器完成回调仅记账/日志（业务收尾仍由任务自身回调负责）
            auto comp_cb = [this](const fivecore::scheduler::TaskCompletion& tc)
            {
                using FR = fivecore::scheduler::FinishReason;
                const char* why = "上传成功";
                switch (tc.reason)
                {
                case FR::Failed:    why = "上传失败";           break;
                case FR::Cancelled: why = "上传已取消";         break;
                case FR::TimedOut:  why = "上传超时，已强制取消"; break;
                default: break;
                }
                spdlog::info("[调度器] 备份任务#{} {}（排队{}ms，执行{}ms）", tc.id, why,
                             std::chrono::duration_cast<std::chrono::milliseconds>(tc.queue_wait).count(),
                             std::chrono::duration_cast<std::chrono::milliseconds>(tc.run_time).count());
                if (tc.reason == FR::TimedOut || tc.reason == FR::Cancelled)
                {
                    emit log(QString("⚠️%1（任务ID:%2）").arg(QString::fromLatin1(why)).arg(tc.id));
                }
            };
            auto sub_res = m_scheduler->submit(task, std::move(comp_cb));
            if (sub_res.is_ok())
            {
                m_uploadSchedId = sub_res.value();
            }
            else
            {
                // 提交失败（调度器已 stop）：按传输失败收尾，避免 m_running 卡死
                spdlog::error("备份任务提交调度器失败: {}", sub_res.error().message);
                QMetaObject::invokeMethod(this, [this, metaId, sha256]() {
                    finalizeBackup(false, "任务提交失败", metaId, sha256);
                }, Qt::QueuedConnection);
            }
        }
        else
        {
            // 调度器不可用：退化为直提全局上下文（与集成前行为一致）
            auto *ctx = fivecore::http_async::get_multi_context();
            if (ctx)
            {
                ctx->submit_task(task);
            }
        }
    }
}

void BackupService::doRestoreAsync()
{
    if (!checkRunning())
    {
        emit finished(false, "任务正在运行，拒绝新任务");
        return;
    }

    std::string file = "coreTime.db";
    emit log(QString("【异步恢复】开始恢复，目标文件：%1").arg(QString::fromStdString(file)));

    // 1. 获取下载预签名URL + 期望哈希（同步调用，请求量小，阻塞可忽略）
    std::string presign_url;
    std::string expectSha;
    bool ok = fetchDownloadPresignedUrl(file, presign_url, expectSha);
    if (!ok)
    {
        spdlog::error("预签名获取失败");
        emit log("❌获取下载预签名URL失败");
        emit finished(false,"获取下载预签名失败");
        m_running = false;
        return;
    }

    emit log(QString("云端记录期望SHA256：%1").arg(QString::fromStdString(expectSha)));
    emit progress(20);

    std::string outfilePath = "coreTime.db";

    // 2. 异步流式下载文件（经 TaskScheduler 调度：限流 + 全局超时 + 取消记账）
    {
        // 任务自身回调：适配层与 http::get_file_stream_async 内部一致，
        // 收尾投递回 service 线程执行，避免阻塞 IO 线程
        auto task_callback_wrapper = [this, expectSha, outfilePath](CURLcode code, long http_code, std::string resp_body)
        {
            http::HttpResponse resp{};
            resp.status_code = http_code;
            resp.body = std::move(resp_body);
            resp.ok = (code == CURLE_OK);
            if (!resp.ok)
            {
                resp.error_msg = curl_easy_strerror(code);
            }

            const bool ok = resp.ok;
            const QString errMsg = ok ? QString()
                                      : QString::fromStdString(resp.error_msg);
            QMetaObject::invokeMethod(this, [this, ok, errMsg, expectSha, outfilePath]() {
                finalizeRestore(ok, errMsg, expectSha, outfilePath);
            }, Qt::QueuedConnection);
        };

        auto task = std::make_shared<fivecore::http_async::HttpGetStreamTask>(
            presign_url, outfilePath, std::move(task_callback_wrapper));

        std::lock_guard<std::mutex> lk(m_taskMutex);
        m_downloadTask = task;
        m_downloadSchedId = 0;
        if (m_scheduler)
        {
            // 调度器完成回调仅记账/日志（业务收尾仍由任务自身回调负责）
            auto comp_cb = [this](const fivecore::scheduler::TaskCompletion& tc)
            {
                using FR = fivecore::scheduler::FinishReason;
                const char* why = "下载成功";
                switch (tc.reason)
                {
                case FR::Failed:    why = "下载失败";           break;
                case FR::Cancelled: why = "下载已取消";         break;
                case FR::TimedOut:  why = "下载超时，已强制取消"; break;
                default: break;
                }
                spdlog::info("[调度器] 恢复任务#{} {}（排队{}ms，执行{}ms）", tc.id, why,
                             std::chrono::duration_cast<std::chrono::milliseconds>(tc.queue_wait).count(),
                             std::chrono::duration_cast<std::chrono::milliseconds>(tc.run_time).count());
                if (tc.reason == FR::TimedOut || tc.reason == FR::Cancelled)
                {
                    emit log(QString("⚠️%1（任务ID:%2）").arg(QString::fromLatin1(why)).arg(tc.id));
                }
            };
            auto sub_res = m_scheduler->submit(task, std::move(comp_cb));
            if (sub_res.is_ok())
            {
                m_downloadSchedId = sub_res.value();
            }
            else
            {
                // 提交失败（调度器已 stop）：按传输失败收尾，避免 m_running 卡死
                spdlog::error("恢复任务提交调度器失败: {}", sub_res.error().message);
                QMetaObject::invokeMethod(this, [this, expectSha, outfilePath]() {
                    finalizeRestore(false, "任务提交失败", expectSha, outfilePath);
                }, Qt::QueuedConnection);
            }
        }
        else
        {
            // 调度器不可用：退化为直提全局上下文（与集成前行为一致）
            auto *ctx = fivecore::http_async::get_multi_context();
            if (ctx)
            {
                ctx->submit_task(task);
            }
        }
    }
}


void BackupService::cancelTask()
{
    std::lock_guard<std::mutex> lk(m_taskMutex);
    // 优先走调度器取消（同时覆盖"排队中"与"运行中"两种状态）；
    // 调度器路径失败（无调度器/ID失效）时回退为直接任务取消（与集成前行为一致）
    if (m_scheduler && m_uploadSchedId != 0 &&
        m_scheduler->cancel(m_uploadSchedId).is_ok())
    {
        emit log("⚠️已触发备份任务取消");
    }
    else if (m_uploadTask)
    {
        m_uploadTask->cancel();
        emit log("⚠️已触发备份任务取消");
    }

    if (m_scheduler && m_downloadSchedId != 0 &&
        m_scheduler->cancel(m_downloadSchedId).is_ok())
    {
        emit log("⚠️已触发恢复任务取消");
    }
    else if (m_downloadTask)
    {
        m_downloadTask->cancel();
        emit log("⚠️已触发恢复任务取消");
    }
}

// ========== 异步任务收尾（运行在 service 线程，不阻塞 libcurl IO 线程） ==========

void BackupService::finalizeBackup(bool ok, const QString& errorMsg,
                                   const std::string& metaId, const std::string& sha256)
{
    if (!ok)
    {
        spdlog::error("异步PUT上传失败: {}", errorMsg.toStdString());
        emit log("❌MinIO异步文件上传失败");
        emit finished(false, "异步PUT上传失败");
    }
    else
    {
        emit progress(80);
        emit log("异步上传成功，调用后端确认接口");

        // 4 调用上传确认接口（同步 HTTP，在 service 线程执行，不阻塞 IO 线程）
        if (!confirm(metaId, sha256))
        {
            spdlog::error("上传确认失败");
            emit log("❌上传确认接口调用失败");
            emit finished(false, "上传确认失败");
        }
        else
        {
            emit progress(100);
            emit log("✅【异步备份】任务全部完成");
            emit finished(true, "");
        }
    }

    m_running = false;
    {
        std::lock_guard<std::mutex> lk(m_taskMutex);
        m_uploadTask.reset();
        m_uploadSchedId = 0;
    }
}

void BackupService::finalizeRestore(bool ok, const QString& errorMsg,
                                    const std::string& expectSha, const std::string& outfilePath)
{
    if (!ok)
    {
        spdlog::error("异步文件下载失败: {}", errorMsg.toStdString());
        emit log("❌文件流式下载失败");
        emit finished(false, "下载文件失败");
    }
    else
    {
        emit progress(70);
        emit log("下载完成，正在校验文件SHA256完整性……");

        // 3. 本地文件哈希校验（在 service 线程执行）
        std::string realSha;
        if (!calcSha256(outfilePath, realSha))
        {
            spdlog::error("恢复后文件哈希计算失败");
            emit log("❌恢复后本地文件哈希计算失败");
            emit finished(false, "本地哈希计算失败");
        }
        else if (realSha != expectSha)
        {
            spdlog::error("❌哈希校验不匹配！expect:{} real:{}", expectSha, realSha);
            emit log(QString("❌哈希校验不匹配！期望:%1 实际:%2")
                         .arg(QString::fromStdString(expectSha))
                         .arg(QString::fromStdString(realSha)));
            emit finished(false, "文件哈希校验不匹配，文件已损坏");
        }
        else
        {
            emit progress(100);
            emit log("✅【异步恢复】完成，SHA256完整性校验通过！");
            emit finished(true, "");
        }
    }

    m_running = false;
    {
        std::lock_guard<std::mutex> lk(m_taskMutex);
        m_downloadTask.reset();
        m_downloadSchedId = 0;
    }
}


