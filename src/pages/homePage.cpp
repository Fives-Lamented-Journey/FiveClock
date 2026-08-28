#include "pages/homePage.h"
#include "ui_homePage.h"
#include "widgets/mybutton.h"
#include "service/record_service.h"
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPainter>

// 构造函数：初始化 UI 按钮与 now_timer（纯显示用，计时逻辑委托给 RecordService）
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 初始化所有 mybutton（统一以 this 为 parent）
    homeButton = new mybutton(":/icon/res/icon/home_pressed.png",
                              ":/icon/res/icon/home_hover.png",
                              ":/icon/res/icon/home_pressed.png", this);
    subButton = new mybutton(":/icon/res/icon/card_ini.png",
                             ":/icon/res/icon/card_hover.png",
                             ":/icon/res/icon/card_pressed.png", this);
    chartButton = new mybutton(":/icon/res/icon/chartButton_ini.png",
                               ":/icon/res/icon/chartButton_hover.png",
                               ":/icon/res/icon/chartButton_pressed.png", this);
    startbutton = new mybutton(":/icon/res/icon/start_pressed_button.png",
                               ":/icon/res/icon/start_hover_button.png",
                               ":/icon/res/icon/start_pressed_button.png", this);
    pausebutton = new mybutton(":/icon/res/icon/pause_pressed_button.png",
                               ":/icon/res/icon/pause_hover_button.png",
                               ":/icon/res/icon/pause_pressed_button.png", this);
    endbutton = new mybutton(":/icon/res/icon/end_pressed_button.png",
                             ":/icon/res/icon/end_hover_button.png",
                             ":/icon/res/icon/end_pressed_button.png", this);
    counterButton = new mybutton(":/icon/res/icon/count_pressed_button.png",
                                 ":/icon/res/icon/count_hover_button.png",
                                 ":/icon/res/icon/count_pressed_button.png", this);
    fullScreenButton = new mybutton(":/icon/res/icon/fullScreen_ini.png",
                                    ":/icon/res/icon/fullScreen_hover.png",
                                    ":/icon/res/icon/fullScreen_pressed.png", this);
    normalScreenButton = new mybutton(":/icon/res/icon/nomalScreen_ini.png",
                                      ":/icon/res/icon/normalScreen_hover.png",
                                      ":/icon/res/icon/normalScreen_pressed.png", this);
    normalScreenButton->hide();
    exitButton = new mybutton(":/icon/res/icon/exit_ini.png",
                              ":/icon/res/icon/exit_hover.png",
                              ":/icon/res/icon/exit_pressed.png", this);

    // now_timer：仅刷新当前时间 LCD（纯 UI 逻辑，不涉及计时）
    now_timer = new QTimer(this);
    now_timer->setInterval(10);
    now_timer->setTimerType(Qt::PreciseTimer);
    now_timer->start();

    // 初始按钮状态（未注入 service 时的安全默认值）
    startbutton->setEnabled(true);
    pausebutton->setEnabled(false);
    endbutton->setEnabled(false);
    counterButton->setEnabled(false);
    setButtonOpacity();

    // 布局：左侧导航按钮
    QVBoxLayout *vBox = new QVBoxLayout();
    vBox->addStretch(1);
    vBox->addWidget(homeButton);
    vBox->addStretch(1);
    vBox->addWidget(subButton);
    vBox->addStretch(1);
    vBox->addWidget(chartButton);
    vBox->addStretch(1);
    vBox->addWidget(fullScreenButton);
    vBox->addStretch(1);
    vBox->addWidget(exitButton);
    vBox->addStretch(1);
    ui->homePageFrame->setLayout(vBox);

    // 布局：底部计时控制按钮
    QHBoxLayout *hBox = new QHBoxLayout();
    hBox->addStretch(1);
    hBox->addWidget(startbutton);
    hBox->addStretch(1);
    hBox->addWidget(pausebutton);
    hBox->addStretch(1);
    hBox->addWidget(counterButton);
    hBox->addStretch(1);
    hBox->addWidget(endbutton);
    hBox->addStretch(1);
    ui->button_frame->setLayout(hBox);

    // LCD 初始化
    ui->startLcd->display("00:00:00");
    ui->startLcd->setStyleSheet("border:none;color:white;");
    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(this);
    opacityEffect->setOpacity(0.7);
    ui->startLcd->setGraphicsEffect(opacityEffect);

    ui->nowLcd->display("00:00:00");
    ui->nowLcd->setStyleSheet("border:none;color:white");

    // 信号连接：now_timer → 当前时间 LCD 刷新
    connect(now_timer, &QTimer::timeout, this, &Widget::do_now_timer_timeout);

    // 信号连接：按钮点击 → 转发到 RecordService
    connect(startbutton, &QPushButton::clicked, this, &Widget::onStartButtonClicked);
    connect(endbutton, &QPushButton::clicked, this, &Widget::onEndButtonClicked);
    connect(pausebutton, &QPushButton::clicked, this, &Widget::onPauseButtonClicked);
}

// 析构函数
Widget::~Widget()
{
    delete ui;
}

// 注入 RecordService：连接其信号以刷新 LCD / 按钮状态 / 转发提醒
void Widget::setRecordService(RecordService *service)
{
    m_recordService = service;
    // 计时 LCD 刷新（每 10ms）
    connect(service, &RecordService::sig_timeUpdated,
            this, &Widget::onServiceTimeUpdated);
    // 计时结束：重置 LCD、更新按钮状态
    connect(service, &RecordService::sig_timerEnded,
            this, &Widget::onServiceTimerEnded);
    // 计时状态变更（start/pause/end）：刷新按钮启用/禁用
    connect(service, &RecordService::sig_stateChanged,
            this, &Widget::applyButtonState);
    // 提醒信号：转发给 assemPage（托盘提示）
    connect(service, &RecordService::sig_alreadyWorking,
            this, &Widget::onServiceAlreadyWorking);
    connect(service, &RecordService::sig_notWorking,
            this, &Widget::onServiceNotWorking);

    // 根据 service 当前状态初始化按钮
    applyButtonState();
}

// ---------- 时间显示（纯 UI） ----------

void Widget::showNowTime()
{
    ui->nowLcd->display(getCurrentTime());
}

QString Widget::getCurrentTime()
{
    return QDateTime::currentDateTime().toString("HH:mm:ss");
}

void Widget::do_now_timer_timeout()
{
    showNowTime();
}

// ---------- 按钮状态 ----------

void Widget::setButtonOpacity()
{
    const QString enabledStyle = "background: transparent; border: none; opacity: 1;";
    const QString disabledStyle = "background: transparent; border: none; opacity: 0;";

    startbutton->setStyleSheet(startbutton->isEnabled() ? enabledStyle : disabledStyle);
    pausebutton->setStyleSheet(pausebutton->isEnabled() ? enabledStyle : disabledStyle);
    endbutton->setStyleSheet(endbutton->isEnabled() ? enabledStyle : disabledStyle);
    counterButton->setStyleSheet(counterButton->isEnabled() ? enabledStyle : disabledStyle);
}

// 根据当前计时状态设置按钮启用/禁用
// 迁移自原 onStart/onPause/onEnd 中的按钮状态逻辑
void Widget::applyButtonState()
{
    if (!m_recordService) {
        // 未注入 service 时的安全默认
        startbutton->setEnabled(true);
        pausebutton->setEnabled(false);
        endbutton->setEnabled(false);
        counterButton->setEnabled(false);
    } else {
        const bool working = m_recordService->isTimerWorking();
        const bool running = m_recordService->isTimerRunning();

        // 未工作：仅 start 可用
        startbutton->setEnabled(!working);
        // 正在工作时：end 始终可用；pause/counter 取决于是否在运行
        endbutton->setEnabled(working);
        pausebutton->setEnabled(working);
        counterButton->setEnabled(working && running);
    }
    setButtonOpacity();
}

// ---------- 按钮点击槽（转发到 RecordService） ----------

void Widget::onStartButtonClicked()
{
    if (m_recordService) {
        m_recordService->timerStart();
    }
}

void Widget::onEndButtonClicked()
{
    if (m_recordService) {
        m_recordService->timerEnd();
    }
}

void Widget::onPauseButtonClicked()
{
    if (m_recordService) {
        m_recordService->timerPause();
    }
}

void Widget::onCounterButtonClicked()
{
    if (m_recordService) {
        m_recordService->timerIncrementCounter();
    }
}

// ---------- RecordService 信号处理 ----------

void Widget::onServiceTimeUpdated(qint64 ms)
{
    // 将毫秒数格式化为 hh:mm:ss 并更新 LCD
    int h = int(ms / (1000 * 60 * 60));
    int m = int((ms % (1000 * 60 * 60)) / (1000 * 60));
    int s = int((ms % (1000 * 60)) / 1000);
    QString timeStr = QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
    ui->startLcd->display(timeStr);
    emit lcdChanged(timeStr);
}

void Widget::onServiceTimerEnded()
{
    // 计时结束：重置 LCD（按钮状态由 sig_stateChanged 统一驱动）
    ui->startLcd->display("00:00:00");
    emit lcdChanged("00:00:00");
}

void Widget::onServiceAlreadyWorking()
{
    emit workingNow();
}

void Widget::onServiceNotWorking()
{
    // pause() / end() 在未工作时均会触发此信号
    // 原逻辑：pause → pausingNow，end → noWorkingNow
    // 由于无法区分，这里统一发 noWorkingNow（托盘提示文案略有差异但不影响功能）
    emit noWorkingNow();
}

// ---------- 绘制 ----------

void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QPixmap pic(":/pic/res/pic/bgi05.png");
    painter.drawPixmap(0, 0, width(), height(), pic);
    QWidget::paintEvent(event);
}

void Widget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}
