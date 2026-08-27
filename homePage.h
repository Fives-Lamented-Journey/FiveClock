#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include <QElapsedTimer>   // counterTimer 为值成员，需要完整类型
#include <QDateTime>       // m_time 为值成员，需要完整类型

QT_BEGIN_NAMESPACE
// 以下 Qt 类型在头文件中仅以指针/引用形式使用，使用前向声明降低编译依赖
class QTimer;
QT_END_NAMESPACE

// 项目内类型仅以指针形式使用，使用前向声明替代 include
class mybutton;

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
    // start 定时器，用于刷新记录流逝时间的 LCD
    QTimer* start_timer;
    // now 定时器，用于刷新记录当前时间的 LCD
    QTimer* now_timer;
    // 计数器，用于记录流逝的时间
    QElapsedTimer counterTimer;

    int elpasedHour;
    int elpasedMin;
    int elpasedseconds;

    // 存储当前日期和时间（栈对象，无需手动释放）
    QDateTime m_time;
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
    mybutton *homeButton = nullptr;
    // 子页面按钮
    mybutton *subButton = nullptr;
    // 图表按钮
    mybutton *chartButton = nullptr;
    // 开始按钮
    mybutton *startbutton = nullptr;
    // 暂停按钮
    mybutton *pausebutton = nullptr;
    // 结束按钮
    mybutton *endbutton = nullptr;
    // 计数按钮
    mybutton *counterButton = nullptr;
    // 设置屏幕状态的按钮
    mybutton *setScreenStateButton = nullptr;
    // 全屏按钮
    mybutton *fullScreenButton = nullptr;
    // 正常屏幕按钮
    mybutton *normalScreenButton = nullptr;
    // 退出按钮
    mybutton *exitButton = nullptr;

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
