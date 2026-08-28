#ifndef CONFIG_SERVICE_H
#define CONFIG_SERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>

#include "dto/time_record.h"   // EventClass 枚举（唯一可信源）

// ConfigService：配置服务
// 职责：集中管理内置常量（事件分类、提示文本、阈值等）
// 约束：不操作UI、不操作数据库、不依赖任何业务服务
//       仅提供只读 getter 接口，禁止外部修改
//       预留 JSON 持久化接口（本迭代不实现）
class ConfigService : public QObject
{
    Q_OBJECT
public:
    explicit ConfigService(QObject *parent = nullptr);
    ~ConfigService() override;

    // ================================================================
    // 事件分类（委托 dto/time_record.h 中 EventClass 枚举）
    // 以下方法为向后兼容包装器，底层实现使用 dto 中的 inline 函数
    // ================================================================
    QStringList eventClassNames() const;
    QString eventClassName(EventClass ec) const;
    EventClass eventClassFromName(const QString &name) const;
    QString eventClassDbKey(EventClass ec) const;

    // ================================================================
    // 托盘/通知提示文本
    // ================================================================
    QString trayTipTitle() const;           // 托盘标题
    QString trayTipAlreadyWorking() const;   // 已在计时
    QString trayTipNotWorking() const;      // 未开始计时
    int trayTipDurationMs() const;          // 提示持续毫秒

    // ================================================================
    // 页面提示文本
    // ================================================================
    QString emptyRecordTip() const;         // 空记录提示

    // ================================================================
    // 桌宠菜单文本
    // ================================================================
    QString petMenuStart() const;
    QString petMenuPause() const;
    QString petMenuCounter() const;
    QString petMenuEnd() const;
    QString petMenuHide() const;
    QString petMenuQuit() const;

    // ================================================================
    // 托盘菜单文本
    // ================================================================
    QString trayMenuShowMain() const;       // 显示主窗口
    QString trayMenuShowPet() const;        // 显示桌宠
    QString trayMenuQuit() const;           // 退出程序

    // ================================================================
    // 应用元信息
    // ================================================================
    QString appName() const;                // 应用名
    QString appTrayTooltip() const;         // 托盘 tooltip

    // ================================================================
    // 计时常量（TimerModel 使用）
    // ================================================================
    int timerTickIntervalMs() const;         // tick 定时器间隔（毫秒）
    int msPerMinute() const;                 // 每分钟毫秒数
    int msPerHour() const;                   // 每小时毫秒数
    int msPerSecond() const;                 // 每秒毫秒数
    QString timeFormatHms() const;           // 时间格式化模板 "hh:mm:ss"

    // ================================================================
    // 预留接口：未来 JSON 持久化
    // ================================================================
    bool loadFromJson(const QString &filePath);   // 预留
    bool saveToJson(const QString &filePath) const; // 预留

private:
    // 通知/提示文本
    QString m_trayTitle;
    QString m_trayAlreadyWorking;
    QString m_trayNotWorking;
    int m_trayDurationMs;

    // 页面提示
    QString m_emptyRecordTip;

    // 桌宠菜单
    QString m_petMenuStart;
    QString m_petMenuPause;
    QString m_petMenuCounter;
    QString m_petMenuEnd;
    QString m_petMenuHide;
    QString m_petMenuQuit;

    // 托盘菜单
    QString m_trayMenuShowMain;
    QString m_trayMenuShowPet;
    QString m_trayMenuQuit;

    // 应用元信息
    QString m_appName;
    QString m_appTrayTooltip;

    // 计时常量
    int m_timerTickIntervalMs = 10;
    int m_msPerMinute = 60000;
    int m_msPerHour = 3600000;
    int m_msPerSecond = 1000;
    QString m_timeFormatHms = QStringLiteral("%1:%2:%3");
};

#endif // CONFIG_SERVICE_H
