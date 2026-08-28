#include "config/config_service.h"

ConfigService::ConfigService(QObject *parent)
    : QObject(parent)
{
    // 托盘/通知提示文本
    m_trayTitle         = QStringLiteral("小家伙");
    m_trayAlreadyWorking = QStringLiteral("已经在计时了哦");
    m_trayNotWorking     = QStringLiteral("还没有开始计时哦");
    m_trayDurationMs     = 2000;

    // 页面提示
    m_emptyRecordTip     = QStringLiteral("记录为空,主人要记得坚持记录哦");

    // 桌宠菜单
    m_petMenuStart    = QStringLiteral("开始计时");
    m_petMenuPause    = QStringLiteral("暂停/继续计时");
    m_petMenuCounter  = QStringLiteral("分神次数+1");
    m_petMenuEnd      = QStringLiteral("结束计时");
    m_petMenuHide     = QStringLiteral("隐藏");
    m_petMenuQuit     = QStringLiteral("退出");

    // 托盘菜单
    m_trayMenuShowMain = QStringLiteral("显示主窗口");
    m_trayMenuShowPet  = QStringLiteral("显示鹤鹤");
    m_trayMenuQuit     = QStringLiteral("退出程序");

    // 应用元信息
    m_appName         = QStringLiteral("FiveClock");
    m_appTrayTooltip  = QStringLiteral("Five's clock");
}

ConfigService::~ConfigService() = default;

// ---------- 事件分类（委托 dto inline 函数） ----------

QStringList ConfigService::eventClassNames() const
{
    return eventClassDisplayNameList();
}

QString ConfigService::eventClassName(EventClass ec) const
{
    return eventClassDisplayName(ec);
}

EventClass ConfigService::eventClassFromName(const QString &name) const
{
    return eventClassFromDisplayName(name);
}

QString ConfigService::eventClassDbKey(EventClass ec) const
{
    // 使用全局作用域限定符调用 dto 中的 inline 函数，避免与成员函数同名冲突
    return ::eventClassDbKey(ec);
}

// ---------- 托盘/通知 ----------

QString ConfigService::trayTipTitle() const { return m_trayTitle; }
QString ConfigService::trayTipAlreadyWorking() const { return m_trayAlreadyWorking; }
QString ConfigService::trayTipNotWorking() const { return m_trayNotWorking; }
int ConfigService::trayTipDurationMs() const { return m_trayDurationMs; }

// ---------- 页面提示 ----------

QString ConfigService::emptyRecordTip() const { return m_emptyRecordTip; }

// ---------- 桌宠菜单 ----------

QString ConfigService::petMenuStart() const { return m_petMenuStart; }
QString ConfigService::petMenuPause() const { return m_petMenuPause; }
QString ConfigService::petMenuCounter() const { return m_petMenuCounter; }
QString ConfigService::petMenuEnd() const { return m_petMenuEnd; }
QString ConfigService::petMenuHide() const { return m_petMenuHide; }
QString ConfigService::petMenuQuit() const { return m_petMenuQuit; }

// ---------- 托盘菜单 ----------

QString ConfigService::trayMenuShowMain() const { return m_trayMenuShowMain; }
QString ConfigService::trayMenuShowPet() const { return m_trayMenuShowPet; }
QString ConfigService::trayMenuQuit() const { return m_trayMenuQuit; }

// ---------- 应用元信息 ----------

QString ConfigService::appName() const { return m_appName; }
QString ConfigService::appTrayTooltip() const { return m_appTrayTooltip; }

// ---------- 计时常量 ----------

int ConfigService::timerTickIntervalMs() const { return m_timerTickIntervalMs; }
int ConfigService::msPerMinute() const { return m_msPerMinute; }
int ConfigService::msPerHour() const { return m_msPerHour; }
int ConfigService::msPerSecond() const { return m_msPerSecond; }
QString ConfigService::timeFormatHms() const { return m_timeFormatHms; }

// ---------- 预留 JSON 持久化 ----------

bool ConfigService::loadFromJson(const QString &filePath)
{
    Q_UNUSED(filePath);
    // 本迭代不实现，预留接口
    return false;
}

bool ConfigService::saveToJson(const QString &filePath) const
{
    Q_UNUSED(filePath);
    // 本迭代不实现，预留接口
    return false;
}
