#ifndef BACKUP_SERVICE_H
#define BACKUP_SERVICE_H

#pragma once
#include <QObject>
#include <QString>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <memory>

// 前向声明，避免把libfivecore头文件带入MOC
namespace fivecore::http {
    struct HttpResponse;
}

namespace fivecore::http_async {
    class HttpPutStreamTask;
    class HttpGetStreamTask;
}

namespace fivecore::scheduler {
    class TaskScheduler;
}

class BackupService : public QObject
{
    Q_OBJECT
public:
    explicit BackupService(QObject *parent = nullptr);
    /// 析构：停止调度器（排队任务以Cancelled结束，完成hook自动注销）
    ~BackupService() override;

public slots:
    /// 子线程槽：启动备份任务
    void doBackup();
    void doRestore();

    /// ========== 新增：异步版本 ==========
    void doBackupAsync();
    void doRestoreAsync();
    /// 取消正在进行的异步任务
    void cancelTask();
signals:
    void log(QString text);          // 日志输出给UI
    void progress(int percent);      // 进度0‑100
    void finished(bool ok, QString errorInfo);

private:
    std::atomic<bool> m_running{false};

    bool checkRunning();
    bool calcSha256(const std::string& path, std::string& outSha);

    bool fetchUploadPresignedUrl(const std::string& filePath, uint64_t fileSize, const std::string& sha256,
                           std::string& outUploadUrl, std::string& outMetaId);

    bool uploadFile(const std::string& presignedUrl, const std::string& filePath);

    bool fetchDownloadPresignedUrl(const std::string& file,std::string& presign, std::string& expectSha);
    bool downloadFile(const std::string& presignedUrl, const std::string& outfilePath);

    bool confirm(const std::string& metaId, const std::string& sha256);

    /// 上传完成后的收尾逻辑（在 service 线程执行，避免阻塞 libcurl IO 线程）
    void finalizeBackup(bool ok, const QString& errorMsg,
                        const std::string& metaId, const std::string& sha256);
    /// 下载完成后的收尾逻辑（在 service 线程执行）
    void finalizeRestore(bool ok, const QString& errorMsg,
                         const std::string& expectSha, const std::string& outfilePath);

private:
    // 保护 m_uploadTask / m_downloadTask 的互斥锁
    // （回调线程与 cancelTask/析构可能并发访问任务句柄）
    std::mutex m_taskMutex;

    // 异步任务句柄，持有shared_ptr保证生命周期
    std::shared_ptr<fivecore::http_async::HttpPutStreamTask> m_uploadTask;
    std::shared_ptr<fivecore::http_async::HttpGetStreamTask> m_downloadTask;

    // ========== TaskScheduler 集成（限流 + 全局超时 + 取消记账） ==========
    // 调度器挂在全局 CurlMultiContext 的完成 hook 上（一个 ctx 至多一个调度器）；
    // 完成回调仅记账/打日志，业务收尾仍由任务自身回调 → finalize* 负责
    std::unique_ptr<fivecore::scheduler::TaskScheduler> m_scheduler;

    // 调度器任务 ID（0 = 无效/未走调度器；受 m_taskMutex 保护）
    // 类型为 fivecore::scheduler::TaskId（即 uint64_t），此处用基础类型避免头文件依赖
    std::uint64_t m_uploadSchedId = 0;
    std::uint64_t m_downloadSchedId = 0;
};

#endif // BACKUP_SERVICE_H
