#include "pages/homePage.h"
#include "ui_homePage.h"
#include "widgets/mybutton.h"
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPainter>

// 构造函数，初始化 Widget 类的对象
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    // 设置 UI 界面
    ui->setupUi(this);

    // 初始化所有 mybutton（统一以 this 为 parent，确保 Qt 自动管理生命周期）
    homeButton = new mybutton(":/icon/res/icon/home_pressed.png",
                              ":/icon/res/icon/home_hover.png",
                              ":/icon/res/icon/home_pressed.png",
                              this);
    subButton = new mybutton(":/icon/res/icon/card_ini.png",
                             ":/icon/res/icon/card_hover.png",
                             ":/icon/res/icon/card_pressed.png",
                             this);
    chartButton = new mybutton(":/icon/res/icon/chartButton_ini.png",
                               ":/icon/res/icon/chartButton_hover.png",
                               ":/icon/res/icon/chartButton_pressed.png",
                               this);
    startbutton = new mybutton(":/icon/res/icon/start_pressed_button.png",
                               ":/icon/res/icon/start_hover_button.png",
                               ":/icon/res/icon/start_pressed_button.png",
                               this);
    pausebutton = new mybutton(":/icon/res/icon/pause_pressed_button.png",
                               ":/icon/res/icon/pause_hover_button.png",
                               ":/icon/res/icon/pause_pressed_button.png",
                               this);
    endbutton = new mybutton(":/icon/res/icon/end_pressed_button.png",
                             ":/icon/res/icon/end_hover_button.png",
                             ":/icon/res/icon/end_pressed_button.png",
                             this);
    counterButton = new mybutton(":/icon/res/icon/count_pressed_button.png",
                                 ":/icon/res/icon/count_hover_button.png",
                                 ":/icon/res/icon/count_pressed_button.png",
                                 this);
    fullScreenButton = new mybutton(":/icon/res/icon/fullScreen_ini.png",
                                    ":/icon/res/icon/fullScreen_hover.png",
                                    ":/icon/res/icon/fullScreen_pressed.png",
                                    this);
    normalScreenButton = new mybutton(":/icon/res/icon/nomalScreen_ini.png",
                                      ":/icon/res/icon/normalScreen_hover.png",
                                      ":/icon/res/icon/nomalScreen_pressed.png",
                                      this);
    // 原实现中该按钮无 parent 且从未加入任何布局，是不可见的顶层窗口；
    // 现补上 parent 后需显式隐藏，保持原有可见行为不变
    normalScreenButton->hide();
    exitButton = new mybutton(":/icon/res/icon/exit_ini.png",
                              ":/icon/res/icon/exit_hover.png",
                              ":/icon/res/icon/exit_pressed.png",
                              this);

    // 创建计时定时器，用于刷新计时时间
    start_timer = new QTimer(this);
    // 设置定时器间隔为 10 毫秒
    start_timer->setInterval(10);
    // 设置定时器为精确模式
    start_timer->setTimerType(Qt::PreciseTimer);
    // 停止定时器
    start_timer->stop();

    // 创建时钟定时器，用于刷新当前时间
    now_timer = new QTimer(this);
    // 设置定时器间隔为 10 毫秒
    now_timer->setInterval(10);
    // 设置定时器为精确模式
    now_timer->setTimerType(Qt::PreciseTimer);
    // 启动定时器
    now_timer->start();

    // 启用开始按钮
    startbutton->setEnabled(true);
    // 禁用暂停按钮
    pausebutton->setEnabled(false);
    // 禁用结束按钮
    endbutton->setEnabled(false);
    // 禁用计数按钮
    counterButton->setEnabled(false);
    // 设置按钮透明度
    setButtonOpacity();

    // 创建垂直布局
    QVBoxLayout* vBox = new QVBoxLayout();
    // 添加拉伸因子
    vBox->addStretch(1);
    // 添加主页按钮
    vBox->addWidget(homeButton);
    // 添加拉伸因子
    vBox->addStretch(1);
    // 添加子页面按钮
    vBox->addWidget(subButton);
    // 添加拉伸因子
    vBox->addStretch(1);
    // 添加图表按钮
    vBox->addWidget(chartButton);
    // 添加拉伸因子
    vBox->addStretch(1);
    // 添加全屏按钮
    vBox->addWidget(fullScreenButton);
    // 添加拉伸因子
    vBox->addStretch(1);
    // 添加退出按钮
    vBox->addWidget(exitButton);
    // 添加拉伸因子
    vBox->addStretch(1);
    // 设置 homePageFrame 的布局
    ui->homePageFrame->setLayout(vBox);

    // 创建水平布局
    QHBoxLayout *hBox = new QHBoxLayout();
    // 添加拉伸因子
    hBox->addStretch(1);
    // 添加开始按钮
    hBox->addWidget(startbutton);
    // 添加拉伸因子
    hBox->addStretch(1);
    // 添加暂停按钮
    hBox->addWidget(pausebutton);
    // 添加拉伸因子
    hBox->addStretch(1);
    // 添加计数按钮
    hBox->addWidget(counterButton);
    // 添加拉伸因子
    hBox->addStretch(1);
    // 添加结束按钮
    hBox->addWidget(endbutton);
    // 添加拉伸因子
    hBox->addStretch(1);

    // 设置 button_frame 的布局
    ui->button_frame->setLayout(hBox);
    // 在 startLcd 上显示初始时间
    ui->startLcd->display("00:00:00");
    // 设置 startLcd 的样式
    ui->startLcd->setStyleSheet("border:none;color:white;");
    // 创建透明度效果对象
    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(this);
    // 设置透明度为 0.7
    opacityEffect->setOpacity(0.7);
    // 应用透明度效果到 startLcd
    ui->startLcd->setGraphicsEffect(opacityEffect);

    // 在 nowLcd 上显示初始时间
    ui->nowLcd->display("00:00:00");
    // 设置 nowLcd 的样式
    ui->nowLcd->setStyleSheet("border:none;color:white");

    // 连接 start_timer 的 timeout 信号到 do_start_timer_timeout 槽函数
    connect(start_timer, &QTimer::timeout, this, &Widget::do_start_timer_timeout);
    // 连接 now_timer 的 timeout 信号到 do_now_timer_timeout 槽函数
    connect(now_timer, &QTimer::timeout, this, &Widget::do_now_timer_timeout);

    // 连接 startbutton 的 clicked 信号到 onStartButtonClicked 槽函数
    connect(startbutton, &QPushButton::clicked, this, &Widget::onStartButtonClicked);
    // 连接 endbutton 的 clicked 信号到 onEndButtonClicked 槽函数
    connect(endbutton, &QPushButton::clicked, this, &Widget::onEndButtonClicked);
    // 连接 pausebutton 的 clicked 信号到 onPauseButtonClicked 槽函数
    connect(pausebutton, &QPushButton::clicked, this, &Widget::onPauseButtonClicked);
}

// 析构函数，释放资源
Widget::~Widget()
{
    // 释放 UI 界面指针
    delete ui;
    // 释放 start_timer 指针
    delete start_timer;
}

// 显示流逝时间的函数
void Widget::showStartTimer()
{
    // 在 startLcd 上显示时间
    ui->startLcd->display(getConsuShowTime());
    // 发出 lcdChanged 信号
    emit lcdChanged(getConsuShowTime());
}

// 显示当前时间的函数
void Widget::showNowTime()
{
    // 获取当前时间
    QString timeNow = getCurrentTime();
    // 在 nowLcd 上显示当前时间
    ui->nowLcd->display(timeNow);
}

// 获取当前时间的函数
QString Widget::getCurrentTime()
{
    // 获取当前日期和时间并格式化为字符串
    return QDateTime::currentDateTime().toString("HH:mm:ss");
}

// 获取消耗时间（分钟）的函数
int Widget::getConsumTime()
{
    // 将毫秒数转换为分钟数
    return mseconds / 60000;
}

QString Widget::getConsuShowTime()
{
    // 获取流逝时间（单位：毫秒）
    mseconds = counterTimer.elapsed() + pausetimer;
    // 计算小时数
    elpasedHour = mseconds / (1000 * 60 * 60);
    // 计算秒数
    elpasedseconds = (mseconds % (1000 * 60)) / 1000;
    // 计算分钟数
    elpasedMin = (mseconds % (1000 * 60 * 60)) / (1000 * 60);

    // 格式化时间字符串
    QString timeStr = QString("%1:%2:%3")
                          .arg(elpasedHour, 2, 10, QChar('0'))
                          .arg(elpasedMin, 2, 10, QChar('0'))
                          .arg(elpasedseconds, 2, 10, QChar('0'));
    return timeStr;
}

// 设置按钮透明度的函数
void Widget::setButtonOpacity()
{
    // 定义启用状态的样式
    QString enabledStyle = "background: transparent; border: none; opacity: 1;";
    // 定义禁用状态的样式
    QString disabledStyle = "background: transparent; border: none; opacity: 0;";

    // 根据开始按钮的状态设置样式
    startbutton->setStyleSheet(startbutton->isEnabled() ? enabledStyle : disabledStyle);
    // 根据暂停按钮的状态设置样式
    pausebutton->setStyleSheet(pausebutton->isEnabled() ? enabledStyle : disabledStyle);
    // 根据结束按钮的状态设置样式
    endbutton->setStyleSheet(endbutton->isEnabled() ? enabledStyle : disabledStyle);
    // 根据计数按钮的状态设置样式
    counterButton->setStyleSheet(counterButton->isEnabled() ? enabledStyle : disabledStyle);
}

// 开始按钮点击的槽函数
void Widget::onStartButtonClicked()
{
    // 如果正在工作，发出 workingNow 信号并返回
    if (isWorking == true)
    {
        emit workingNow();
        return;
    }

    // 标记为正在计时
    isRunning = true;
    // 标记为正在工作
    isWorking = true;
    // 重置计数器
    counter = 0;
    // 启动计时定时器
    start_timer->start();

    // 记录开始时间
    startTime = getCurrentTime();

    // 启动计数器
    counterTimer.start();
    // 禁用开始按钮
    startbutton->setEnabled(false);
    // 启用结束按钮
    endbutton->setEnabled(true);
    // 启用暂停按钮
    pausebutton->setEnabled(true);
    // 启用计数按钮
    counterButton->setEnabled(true);
    // 设置按钮透明度
    setButtonOpacity();
}

// 结束按钮点击的槽函数
void Widget::onEndButtonClicked()
{
    // 如果未工作，发出 noWorkingNow 信号并返回
    if (isWorking == false)
    {
        emit noWorkingNow();
        return;
    }

    // 停止计时定时器
    start_timer->stop();
    // 获取消耗时间
    consumTime = getConsumTime();
    // 重置记录的流逝时间
    pausetimer = 0;
    // 记录结束时间
    endTime = getCurrentTime();
    // 输出结束信息
    qDebug("end");
    // 输出结束按钮点击时的消耗时间
    qDebug() << "endbutton_consumTime:" << consumTime;

    // 启用开始按钮
    startbutton->setEnabled(true);
    // 禁用计数按钮
    counterButton->setEnabled(false);
    // 禁用结束按钮
    endbutton->setEnabled(false);
    // 禁用暂停按钮
    pausebutton->setEnabled(false);
    // 设置按钮透明度
    setButtonOpacity();

    // 在 startLcd 上显示初始时间
    ui->startLcd->display("00:00:00");
    // 发出 lcdChanged 信号
    emit lcdChanged("00:00:00");
}

// 暂停按钮点击的槽函数
void Widget::onPauseButtonClicked()
{
    // 如果未工作，发出 pausingNow 信号并返回
    if (isWorking == false)
    {
        emit pausingNow();
        return;
    }

    if (isRunning)  // 当在计时时，记录之前已经流逝的时间，并停止计时，设置当前状态
    {
        // 停止计时定时器
        start_timer->stop();
        // 累加暂停前已经流逝的时间
        pausetimer += counterTimer.restart();

        // 禁用开始按钮
        startbutton->setEnabled(false);
        // 启用结束按钮
        endbutton->setEnabled(true);
        // 禁用计数按钮
        counterButton->setEnabled(false);
        // 设置按钮透明度
        setButtonOpacity();

        // 标记为未计时
        isRunning = false;
        // 计数器加 1
        counter++;
    }
    else    // 当在暂停计时时，继续计时，重置计时器，设置当前状态
    {
        // 启动计时定时器
        start_timer->start();
        // 重置计数器
        counterTimer.restart();

        // 启用计数按钮
        counterButton->setEnabled(true);
        // 设置按钮透明度
        setButtonOpacity();

        // 标记为正在计时
        isRunning = true;
    }
}

// 计数按钮点击的槽函数
void Widget::onCounterButtonClicked()
{
    // 计数器加 1
    counter++;
}

// start 定时器超时的槽函数
void Widget::do_start_timer_timeout()
{
    // 定时刷新显示流逝时间的 LCD
    showStartTimer();
}

// now 定时器超时的槽函数
void Widget::do_now_timer_timeout()
{
    // 定时刷新显示当前时间的 LCD
    showNowTime();
}

// 绘制事件处理函数
void Widget::paintEvent(QPaintEvent *event)
{
    // 忽略事件参数
    Q_UNUSED(event);
    // 创建 QPainter 对象
    QPainter painter(this);
    // 加载背景图片
    QPixmap pic(":/pic/res/pic/bgi05.png");
    // 绘制背景图片
    painter.drawPixmap(0, 0, width(), height(), pic);
    // 调用父类的 paintEvent 函数
    QWidget::paintEvent(event);
}

// 大小改变事件处理函数
void Widget::resizeEvent(QResizeEvent *event)
{
    // 调用父类的 resizeEvent 函数
    QWidget::resizeEvent(event);
    // 更新界面
    update();
}
