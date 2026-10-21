#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include <QPixmap>         // m_bgScaled 背景缩放缓存
#include <QDateTime>       // m_time 为值成员，需要完整类型
#include "include/pages/backuplogdialog.h"

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

// 项目内类型仅以指针形式使用
class mybutton;
class RecordService;
class TimerService;
class ConfigService;
class BackupService;

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

// Widget 类继承自 QWidget，纯视图层
// 计时逻辑委托给 TimerService，记录操作委托给 RecordService
class Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

    // 注入记录业务服务（查询/记录操作通过它）
    void setRecordService(RecordService *service);
    // 注入计时业务服务（计时控制通过它）
    void setTimerService(TimerService *service);
    // 注入配置服务（转发给 BackupService 读取备份端点 URL）
    void setConfigService(ConfigService *service);

    // 显示当前时间的函数
    void showNowTime();
    // 获取当前时间的函数
    QString getCurrentTime();
    // 设置按钮透明度
    void setButtonOpacity();

    // 主页按钮
    mybutton *homeButton = nullptr;
    mybutton *subButton = nullptr;
    mybutton *chartButton = nullptr;
    mybutton *startbutton = nullptr;
    mybutton *pausebutton = nullptr;
    mybutton *endbutton = nullptr;
    mybutton *counterButton = nullptr;
    mybutton *setScreenStateButton = nullptr;
    mybutton *fullScreenButton = nullptr;
    mybutton *normalScreenButton = nullptr;
    mybutton *exitButton = nullptr;

    mybutton *uploadButton = nullptr;
    mybutton *downloadButton = nullptr;

signals:
    void lcdChanged(QString time);  // LCD 变化通知（转发给桌宠）
    void workingNow();              // 正在工作提醒（托盘提示）
    void noWorkingNow();            // 未工作提醒
    void pausingNow();              // 暂停提醒

    void dobackup();
    void dorestore();

public slots:
    void onStartButtonClicked();
    void onEndButtonClicked();
    void onPauseButtonClicked();
    void onCounterButtonClicked();
    void do_now_timer_timeout();

    void onUploadButtonClicked();
    void onDownloadButtonClicked();
    void onBackupFinished(bool ok,QString err);


private slots:
    // TimerService 信号处理：更新 LCD / 按钮状态 / 转发提醒信号
    void onTimerTimeUpdated(qint64 ms);
    void onTimerEnded();
    void onTimerAlreadyWorking();
    void onTimerNotWorking();

private:
    // 构建时根据当前计时状态设置按钮启用/禁用
    void applyButtonState();

    QTimer *now_timer = nullptr;        // 刷新当前时间 LCD（纯 UI）
    QDateTime m_time;
    QPixmap m_bgScaled;                 // 背景缩放缓存（仅尺寸变化时重算，避免每帧拉伸）

    TimerService *m_timerService = nullptr;   // 计时业务服务（计时控制）
    RecordService *m_recordService = nullptr;  // 记录业务服务（查询/记录操作）
    BackupService *m_backup_service = nullptr;
    QThread* m_backup_thread;

    BackupLogDialog* m_backupDialog;

    Ui::Widget *ui;

    void initButton();
    void initLayout();
    void initBackupLogDialog();
    void initUI();
    void initSingals();

    void initBackUpService();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};
#endif // HOMEPAGE_H
