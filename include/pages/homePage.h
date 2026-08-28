#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include <QDateTime>       // m_time 为值成员，需要完整类型

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

// 项目内类型仅以指针形式使用
class mybutton;
class RecordService;

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

// Widget 类继承自 QWidget，纯视图层：不再持有计时业务状态
// 计时逻辑全部委托给 RecordService，本类仅负责 UI 显示与用户操作转发
class Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

    // 注入记录业务服务（页面通过它操作计时、订阅信号刷新 UI）
    void setRecordService(RecordService *service);

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

signals:
    void lcdChanged(QString time);  // LCD 变化通知（转发给桌宠）
    void workingNow();              // 正在工作提醒（托盘提示）
    void noWorkingNow();            // 未工作提醒
    void pausingNow();              // 暂停提醒

public slots:
    void onStartButtonClicked();
    void onEndButtonClicked();
    void onPauseButtonClicked();
    void onCounterButtonClicked();
    void do_now_timer_timeout();

private slots:
    // RecordService 信号处理：更新 LCD / 按钮状态 / 转发提醒信号
    void onServiceTimeUpdated(qint64 ms);
    void onServiceTimerEnded();
    void onServiceAlreadyWorking();
    void onServiceNotWorking();

private:
    // 构建时根据当前计时状态设置按钮启用/禁用
    void applyButtonState();

    QTimer *now_timer = nullptr;        // 刷新当前时间 LCD（纯 UI）
    QDateTime m_time;

    RecordService *m_recordService = nullptr;

    Ui::Widget *ui;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};
#endif // HOMEPAGE_H
