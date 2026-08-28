#ifndef ASSEMPAGE_H
#define ASSEMPAGE_H

#include <QWidget>
#include "db/sqldbm.h"        // DBM 为值成员，需要完整类型
#include <memory>
#include <QSystemTrayIcon>   // trayIcon 成员类型 + onTrayIconActivated 槽签名

QT_BEGIN_NAMESPACE
class QMenu;
class QSqlQuery;
class QPushButton;
QT_END_NAMESPACE

// 项目内类型仅以指针形式使用，使用前向声明替代 include
class Widget;
class subPage;
class chartPage;
class petwindow;
class RecordService;   // 前向声明：unique_ptr 成员，不需要完整类型
class TimerService;    // 前向声明：unique_ptr 成员，不需要完整类型
class ConfigService;   // 前向声明：unique_ptr 成员，不需要完整类型
class NotificationService; // 前向声明：unique_ptr 成员

#include "dto/time_record.h"  // TimerEndSnapshot 作为槽参数类型，需要完整定义

QT_BEGIN_NAMESPACE
namespace Ui {
class assemPage;
}
QT_END_NAMESPACE

// assemPage 类，继承自 QWidget
class assemPage : public QWidget
{
    Q_OBJECT

public:
    assemPage(QWidget *parent = nullptr);
    ~assemPage();

    void createTrayIconMenu();

private:
    Widget* aHomePage;
    subPage* aSubPage;
    chartPage* aChartPage;

    sqlDBM DBM;

    // 记录业务服务层（负责数据库读写与统计）
    std::unique_ptr<RecordService> m_recordService;
    // 计时业务服务层（负责计时状态与信号）
    std::unique_ptr<TimerService> m_timerService;
    // 配置服务（负责常量与配置管理）
    std::unique_ptr<ConfigService> m_configService;
    // 通知服务（负责托盘提示封装）
    std::unique_ptr<NotificationService> m_notificationService;

    bool assMoving;
    QPoint assLastPose;

    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;

    petwindow *pet;

private slots:
    void pageChangeToHome();
    void pageChangeToSub();
    void pageChangeToChart();

    void SetScreenState();
    void exitWindow();

    // 计时结束落库（由 TimerService::sig_timerEnded 触发，桥接到 RecordService）
    // 注：卡片 CRUD 交互已下沉至 subPage，assempage 不再处理
    void assAddRecord(const TimerEndSnapshot &snap);

    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    // 构造流程分解（B-14b）：构造函数只保留装配顺序，实现细节在各方法中
    void initServices();     // 服务实例化与相互注入（顺序敏感：RecordService → TimerService → ConfigService）
    void initPages();        // 页面/桌宠实例化与服务注入 + stackedWidget 组装
    void initTrayIcon();     // 托盘图标 + 右键菜单 + NotificationService
    void connectSignals();   // 全部信号槽连接（组装层桥接的唯一入口）

    Ui::assemPage *ui;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
};
#endif // ASSEMPAGE_H
