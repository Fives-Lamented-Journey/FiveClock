/**
 * @file backup_service.h
 * @brief 备份服务提供者，业务与网络之间的粘合层，翻译官
 *
 * 基于TaskScheduler(持有)实现网络传输服务功能的粘合层
 * 同时提供降级服务（TaskScheduler创建失败时，会与CurlMutiContext直接交互，进行任务投递）
 * 通过ConfigService实现配置注入（IP/URL等）
 *
 * @todo 备份文件名称同样不应该硬编码，应当置于配置文件中
 * @todo 恢复文件名称不应该于源文件名称重复，应当先通过添加后缀或其它方式实现恢复文件名称与源文件的差异，恢复成功后，再删除源文件，同时更改恢复文件名称，避免恢复失败时，造成源文件损坏
 * @todo 清理遗留代码
 * @todo
 *
 */
#ifndef BACKUP_SERVICE_H
#define BACKUP_SERVICE_H

#pragma once
#include <QObject>
#include <QString>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <memory>
#include <string>

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

class ConfigService;

class BackupService : public QObject
{
    Q_OBJECT
public:
    explicit BackupService(QObject *parent = nullptr);
    /// 析构：停止调度器（排队任务以Cancelled结束，完成hook自动注销）
    ~BackupService() override;

    /// 注入配置服务（备份端点 URL 来源；仅读取，不持有所有权）
    /// 直接方法调用即可（仅存储指针，与所在线程无关）
    void setConfigService(const ConfigService *service);

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

    // 配置服务（端点 URL 来源；外部注入，非拥有指针，仅只读访问）
    const ConfigService *m_configService = nullptr;

    bool checkRunning();
    bool calcSha256(const std::string& path, std::string& outSha);

    // 备份端点 URL（已注入 ConfigService 则读取配置，否则回退内置默认地址）
    std::string presignUploadUrl() const;
    std::string presignDownloadUrl() const;
    std::string uploadConfirmUrl() const;

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
