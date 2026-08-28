#include "service/notification_service.h"
#include "config/config_service.h"
#include <QSystemTrayIcon>

NotificationService::NotificationService(QSystemTrayIcon *trayIcon, QObject *parent)
    : QObject(parent)
    , m_trayIcon(trayIcon)
{
    if (m_trayIcon) {
        connect(m_trayIcon, &QSystemTrayIcon::activated,
                this, &NotificationService::trayActivated);
    }
}

NotificationService::~NotificationService()
{
    // m_trayIcon 为外部注入，不负责释放
}

void NotificationService::setConfigService(ConfigService *configService)
{
    m_configService = configService;
}

// ---------- 语义化通知接口 ----------

void NotificationService::notifyAlreadyWorking()
{
    if (!m_trayIcon) return;
    QString title = m_configService ? m_configService->trayTipTitle()
                                    : QStringLiteral("小家伙");
    QString msg = m_configService ? m_configService->trayTipAlreadyWorking()
                                  : QStringLiteral("已经在计时了哦");
    int duration = m_configService ? m_configService->trayTipDurationMs() : 2000;
    m_trayIcon->showMessage(title, msg, QSystemTrayIcon::Information, duration);
}

void NotificationService::notifyNotWorking()
{
    if (!m_trayIcon) return;
    QString title = m_configService ? m_configService->trayTipTitle()
                                    : QStringLiteral("小家伙");
    QString msg = m_configService ? m_configService->trayTipNotWorking()
                                  : QStringLiteral("还没有开始计时哦");
    int duration = m_configService ? m_configService->trayTipDurationMs() : 2000;
    m_trayIcon->showMessage(title, msg, QSystemTrayIcon::Information, duration);
}

void NotificationService::showInfoMessage(const QString &title, const QString &message, int durationMs)
{
    if (!m_trayIcon) return;
    int duration = durationMs;
    if (durationMs <= 0 && m_configService) {
        duration = m_configService->trayTipDurationMs();
    } else if (durationMs <= 0) {
        duration = 2000;
    }
    m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, duration);
}
