#ifndef NOTIFICATION_SERVICE_H
#define NOTIFICATION_SERVICE_H

#include <QObject>
#include <QString>
#include <QSystemTrayIcon>   // 信号签名使用 QSystemTrayIcon::ActivationReason 嵌套枚举

class ConfigService;

// NotificationService：通知服务
// 职责：封装 QSystemTrayIcon 的 showMessage 接口，提供语义化通知方法
// 约束：不操作UI控件、不操作数据库、通过 ConfigService 读取通知文本
//       依赖方向：assempage → NotificationService → QSystemTrayIcon（Qt原生）+ ConfigService
class NotificationService : public QObject
{
    Q_OBJECT
public:
    explicit NotificationService(QSystemTrayIcon *trayIcon, QObject *parent = nullptr);
    ~NotificationService() override;

    // 注入配置服务（读取通知文案和参数）
    void setConfigService(ConfigService *configService);

    // ================================================================
    // 语义化通知接口
    // ================================================================
    // 已在工作中：重复 start 时的提醒
    void notifyAlreadyWorking();
    // 未开始/暂停：pause/end 时的提醒
    void notifyNotWorking();

    // 通用通知接口（供外部自定义文案调用，如倒计时完成等）
    void showInfoMessage(const QString &title, const QString &message, int durationMs = -1);

signals:
    // 托盘图标激活信号（转发自 QSystemTrayIcon，供 assempage 订阅）
    void trayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    QSystemTrayIcon *m_trayIcon = nullptr;      // 外部注入的托盘图标（不负责释放）
    ConfigService *m_configService = nullptr;   // 注入的配置服务（不负责释放）
};

#endif // NOTIFICATION_SERVICE_H
