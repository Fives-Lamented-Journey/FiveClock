#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include <qsqldatabase.h>
#include <qdatawidgetmapper>
#include <QDate>
#include <qelapsedtimer>
#include <qtimer.h>
#include <QTime>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <QPainter>
#include <mybutton.h>
#include <QGraphicsOpacityEffect>
#include <infodialog.h>
#include <QSystemTrayIcon>
QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

// Widget 类继承自 QWidget，用于实现主界面的功能
class Widget : public QWidget
{
    Q_OBJECT
private:
    // infoDialog 指针，用于显示信息对话框
    infoDialog *m_infoDialog;

    // start 定时器，用于刷新记录流逝时间的 LCD
    QTimer* start_timer;
    // now 定时器，用于刷新记录当前时间的 LCD
    QTimer* now_timer;
    // 计数器，用于记录流逝的时间
    QElapsedTimer counterTimer;

    int elpasedHour;
    int elpasedMin;
    int elpasedseconds;

    // 存储当前日期和时间的指针
    QDateTime * m_time;
    // 记录流逝的毫秒数
    qint64 mseconds;
    // 记录暂停时已经流逝的时间
    qint64 pausetimer = 0;




public:
    // 构造函数，parent 为父窗口指针，默认为 nullptr
    Widget(QWidget *parent = nullptr);
    // 析构函数
    ~Widget();

    // 显示流逝时间的函数
    void showStartTimer();
    // 显示当前时间的函数
    void showNowTime();

    // 获取当前时间的函数
    QString getCurrentTime();

    // 获取消耗时间（分钟）的函数
    int  getConsumTime();

    //获取用于显示的消耗时间的函数（时间格式"hh:mm,ss"）
    QString getConsuShowTime();

    // 设置定时器的函数
    void setTimer(QTimer *newTimer);

    // 设置按钮透明度的函数
    void setButtonOpacity();

    // 标记是否正在工作
    bool isWorking = false;
    // 标记是否正在计时
    bool isRunning = false;

    // 记录开始时间的字符串
    QString startTime;
    // 记录结束时间的字符串
    QString endTime;
    // 记录消耗的时间
    int consumTime;
    // 计数器
    int counter;
    // 主页按钮
    mybutton *homeButton = new mybutton(":/icon/res/icon/home_pressed.png",
                                        ":/icon/res/icon/home_hover.png",
                                        ":/icon/res/icon/home_pressed.png",
                                        this);
    // 子页面按钮
    mybutton *subButton = new mybutton(":/icon/res/icon/card_ini.png",
                                       ":/icon/res/icon/card_hover.png",
                                       ":/icon/res/icon/card_pressed.png",
                                       this);
    // 图表按钮
    mybutton *chartButton = new mybutton(":/icon/res/icon/chartButton_ini.png",
                                         ":/icon/res/icon/chartButton_hover.png",
                                         ":/icon/res/icon/chartButton_pressed.png");
    // 开始按钮
    mybutton *startbutton = new mybutton(":/icon/res/icon/start_pressed_button.png",
                                         ":/icon/res/icon/start_hover_button.png",
                                         ":/icon/res/icon/start_pressed_button.png",
                                         this);
    // 暂停按钮
    mybutton *pausebutton = new mybutton(":/icon/res/icon/pause_pressed_button.png",
                                         ":/icon/res/icon/pause_hover_button.png",
                                         ":/icon/res/icon/pause_pressed_button.png",
                                         this);
    // 结束按钮
    mybutton *endbutton = new mybutton(":/icon/res/icon/end_pressed_button.png",
                                       ":/icon/res/icon/end_hover_button.png",
                                       ":/icon/res/icon/end_pressed_button.png",
                                       this);
    // 计数按钮
    mybutton *counterButton = new mybutton(":/icon/res/icon/count_pressed_button.png",
                                           ":/icon/res/icon/count_hover_button.png",
                                           ":/icon/res/icon/count_pressed_button.png");
    // 设置屏幕状态的按钮
    mybutton *setScreenStateButton;
    // 全屏按钮
    mybutton *fullScreenButton = new mybutton(":/icon/res/icon/fullScreen_ini.png",
                                              ":/icon/res/icon/fullScreen_hover.png",
                                              ":/icon/res/icon/fullScreen_pressed.png");
    // 正常屏幕按钮
    mybutton *normalScreenButton = new mybutton(":/icon/res/icon/nomalScreen_ini.png",
                                                ":/icon/res/icon/normalScreen_hover.png",
                                                ":/icon/res/icon/nomalScreen_pressed.png");
    // 退出按钮
    mybutton *exitButton = new mybutton(":/icon/res/icon/exit_ini.png",
                                        ":/icon/res/icon/exit_hover.png",
                                        ":/icon/res/icon/exit_pressed.png");

signals:
    // 当 LCD 显示的时间改变时发出的信号
    void lcdChanged(QString time);
    // 正在工作时发出的信号
    void workingNow();
    // 未工作时发出的信号
    void noWorkingNow();
    // 暂停时发出的信号
    void pausingNow();

public slots:
    // 开始按钮点击的槽函数
    void onStartButtonClicked();
    // 结束按钮点击的槽函数
    void onEndButtonClicked();
    // 暂停按钮点击的槽函数
    void onPauseButtonClicked();
    // 计数按钮点击的槽函数
    void onCounterButtonClicked();
    // start 定时器超时的槽函数
    void do_start_timer_timeout();
    // now 定时器超时的槽函数
    void do_now_timer_timeout();

private:
    // UI 界面指针
    Ui::Widget *ui;

    // QWidget 界面重写的事件处理函数
protected:
    // 绘制事件处理函数
    void paintEvent(QPaintEvent *event) override;
    // 大小改变事件处理函数
    void resizeEvent(QResizeEvent *event) override;
};
#endif // HOMEPAGE_H
