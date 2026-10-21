#include "pages/homePage.h"
#include "ui_homePage.h"
#include "widgets/mybutton.h"
#include "service/record_service.h"
#include "service/timer_service.h"
#include "service/backup_service.h"
#include "config/config_service.h"

#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <qmessagebox.h>
#include <QPainter>
#include <QThread>

// 构造函数：初始化 UI 按钮与 now_timer（纯显示用）
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    initButton();
    initLayout();
    initUI();

    initBackUpService();
    initBackupLogDialog();

    initSingals();

}

// 析构函数
Widget::~Widget()
{
    if(m_backup_thread)
    {
        // 先取消正在进行的异步任务
        if(m_backup_service)
        {
            QMetaObject::invokeMethod(m_backup_service, "cancelTask");
        }
        // 告诉子线程事件循环退出；如果此时正在跑curl会等待当前任务跑完
        m_backup_thread->quit();
        // 阻塞等待子线程完全结束，再释放对象
        m_backup_thread->wait();

        delete m_backup_service;
        delete m_backup_thread;
    }
    delete ui;
}

// 注入 RecordService（记录查询/操作通过它）
void Widget::setRecordService(RecordService *service)
{
    m_recordService = service;
    // RecordService 无计时信号，此处无需连接
}

// 注入配置服务：BackupService 在构造期已创建，直接转发（仅赋值指针，跨线程安全）
void Widget::setConfigService(ConfigService *service)
{
    if (m_backup_service)
        m_backup_service->setConfigService(service);
}

// 注入 TimerService（计时控制通过它）
void Widget::setTimerService(TimerService *service)
{
    m_timerService = service;
    // 计时 LCD 刷新（每 10ms）
    connect(service, &TimerService::sig_timeUpdated,
            this, &Widget::onTimerTimeUpdated);
    // 计时结束：重置 LCD
    connect(service, &TimerService::sig_timerEnded,
            this, &Widget::onTimerEnded);
    // 计时状态变更（start/pause/end）：刷新按钮启用/禁用
    connect(service, &TimerService::sig_stateChanged,
            this, &Widget::applyButtonState);
    // 提醒信号：转发给 assemPage（托盘提示）
    connect(service, &TimerService::sig_alreadyWorking,
            this, &Widget::onTimerAlreadyWorking);
    connect(service, &TimerService::sig_notWorking,
            this, &Widget::onTimerNotWorking);

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

// 根据当前计时状态设置按钮启用/禁用（从 TimerService 查询状态）
void Widget::applyButtonState()
{
    if (!m_timerService) {
        // 未注入 service 时的安全默认
        startbutton->setEnabled(true);
        pausebutton->setEnabled(false);
        endbutton->setEnabled(false);
        counterButton->setEnabled(false);
    } else {
        const bool working = m_timerService->isTimerWorking();
        const bool running = m_timerService->isTimerRunning();

        // 未工作：仅 start 可用
        startbutton->setEnabled(!working);
        // 正在工作时：end 始终可用；pause/counter 取决于是否在运行
        endbutton->setEnabled(working);
        pausebutton->setEnabled(working);
        counterButton->setEnabled(working && running);
    }
    setButtonOpacity();
}

// ---------- 按钮点击槽（转发到 TimerService） ----------

void Widget::onStartButtonClicked()
{
    if (m_timerService) {
        m_timerService->timerStart();
    }
}

void Widget::onEndButtonClicked()
{
    if (m_timerService) {
        m_timerService->timerEnd();
    }
}

void Widget::onPauseButtonClicked()
{
    if (m_timerService) {
        m_timerService->timerPause();
    }
}

void Widget::onCounterButtonClicked()
{
    if (m_timerService) {
        m_timerService->timerIncrementCounter();
    }
}

void Widget::onUploadButtonClicked()
{
    if (m_backup_service)
    {
        m_backupDialog->clearAll();
        m_backupDialog->show();
        emit dobackup();
    }

}

void Widget::onDownloadButtonClicked()
{
    if (m_backup_service)
    {
        m_backupDialog->clearAll();
        m_backupDialog->show();
        emit dorestore();
    }
}

void Widget::onBackupFinished(bool ok, QString err)
{
    // m_backupDialog->hide();
    if(ok)
    {
        QMessageBox::information(this,"任务完成","操作执行成功");
        m_backupDialog->appendLog("==== ✅ 任务完成：操作执行成功 ====");
    }
    else
    {
        QMessageBox::critical(this,"任务失败",err);
        m_backupDialog->appendLog(QString("==== ❌ 任务失败：%1 ====").arg(err));
    }
}

// ---------- TimerService 信号处理 ----------

void Widget::onTimerTimeUpdated(qint64 ms)
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

void Widget::onTimerEnded()
{
    // 计时结束：重置 LCD（按钮状态由 sig_stateChanged 统一驱动）
    ui->startLcd->display("00:00:00");
    emit lcdChanged("00:00:00");
}

void Widget::onTimerAlreadyWorking()
{
    emit workingNow();
}

void Widget::onTimerNotWorking()
{
    // pause() / end() 在未工作时均会触发此信号
    emit noWorkingNow();
}

// ---------- 绘制 ----------

void Widget::initButton()
{

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

    uploadButton = new mybutton(":/icon/res/icon/upload_ini_button.png",
                                ":/icon/res/icon/upload_hover_button.png",
                                ":/icon/res/icon/upload_pressed_button.png", this);
    downloadButton = new mybutton(":/icon/res/icon/download_ini_button.png",
                                 ":/icon/res/icon/download_hover_button.png",
                                 ":/icon/res/icon/download_pressed_button.png", this);

    // 初始按钮状态（未注入 service 时的安全默认值）
    startbutton->setEnabled(true);
    pausebutton->setEnabled(false);
    endbutton->setEnabled(false);
    counterButton->setEnabled(false);
    setButtonOpacity();

    // 信号连接：按钮点击 → 转发到 TimerService
    connect(startbutton, &QPushButton::clicked, this, &Widget::onStartButtonClicked);
    connect(endbutton, &QPushButton::clicked, this, &Widget::onEndButtonClicked);
    connect(pausebutton, &QPushButton::clicked, this, &Widget::onPauseButtonClicked);

    // connect(uploadButton,&QPushButton::clicked,this,&Widget::dobackup);
    // connect(downloadButton,&QPushButton::clicked,this,&Widget::dorestore);

    connect(uploadButton, &QPushButton::clicked, this, &Widget::onUploadButtonClicked);
    connect(downloadButton, &QPushButton::clicked, this, &Widget::onDownloadButtonClicked);
}

void  Widget::initLayout()
{
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

    vBox->addWidget(uploadButton);
    vBox->addStretch(1);
    vBox->addWidget(downloadButton);
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
}

void Widget::initBackupLogDialog()
{
    qDebug() << __func__;
    m_backupDialog = new BackupLogDialog(this); // parent设为本窗口，自动居中

    // 绑定BackupService信号 → 弹窗的槽函数
    connect(m_backup_service, &BackupService::log, m_backupDialog, &BackupLogDialog::appendLog);
    connect(m_backup_service, &BackupService::progress, m_backupDialog, &BackupLogDialog::setProgress);
    connect(m_backup_service, &BackupService::finished, this, &Widget::onBackupFinished);

    connect(m_backupDialog, &BackupLogDialog::cancelRequested, m_backup_service, &BackupService::cancelTask);
}

void Widget::initUI()
{
    // QFrame 默认不绘制 app 级 QSS 的背景/边框，必须显式开启 WA_StyledBackground；
    // 三栏玻璃容器（左导航/计时面板/右品牌区）的样式由 app.qss 提供
    ui->homePageFrame->setAttribute(Qt::WA_StyledBackground, true);
    ui->timerGlass->setAttribute(Qt::WA_StyledBackground, true);
    ui->rightFrame->setAttribute(Qt::WA_StyledBackground, true);
    // 计时面板给足最小宽度，避免被两侧弹簧挤窄导致大号 LCD 放不下
    ui->timerGlass->setMinimumWidth(560);

    // now_timer：仅刷新当前时间 LCD（纯 UI 逻辑，不涉及计时）
    now_timer = new QTimer(this);
    now_timer->setInterval(10);
    now_timer->setTimerType(Qt::PreciseTimer);
    now_timer->start();

    // LCD 初始化：QLCDNumber 段码按 widget 尺寸自适应缩放，
    // 单靠 stylesheet font-size 不够，需同时设最小尺寸 + Expanding 策略让它真正占满空间。
    ui->nowLcd->setStyleSheet("border: none; color: rgba(255, 255, 255, 0.75);");
    ui->nowLcd->setFont(QFont("Arial", 22));
    ui->nowLcd->setMinimumHeight(60);
    ui->nowLcd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ui->startLcd->setStyleSheet("border: none; color: white;");
    ui->startLcd->setFont(QFont("Arial", 72));
    ui->startLcd->setMinimumHeight(150);
    ui->startLcd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->startLcd->display("00:00:00");

    ui->nowLcd->display("00:00:00");
}

void Widget::initSingals()
{
    // 信号连接：now_timer → 当前时间 LCD 刷新
    connect(now_timer, &QTimer::timeout, this, &Widget::do_now_timer_timeout);
}

void Widget::initBackUpService()
{
    m_backup_service = new BackupService;
    m_backup_thread = new QThread;
    m_backup_service->moveToThread(m_backup_thread);
    m_backup_thread->start();

    // connect(this, &Widget::dobackup,m_backup_service, &BackupService::doBackup);
    connect(this, &Widget::dobackup,m_backup_service, &BackupService::doBackupAsync);

    // connect(this,&Widget::dorestore,m_backup_service,&BackupService::doRestore);

    connect(this, &Widget::dorestore, m_backup_service, &BackupService::doRestoreAsync);

    // 注：取消任务的连接统一在 initBackupLogDialog() 中建立，
    // 因为此时 m_backupDialog 尚未创建，此处连接无效且与后者重复
}


void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // 背景缩放缓存：仅在窗口尺寸变化时重算一次，其余帧直接贴图
    // （每帧从 qrc 解码 PNG 并拉伸大图是拖动/调整窗口卡顿的根因，与 subPage 策略一致）
    if (m_bgScaled.size() != size()) {
        m_bgScaled = QPixmap(":/pic/res/pic/bgi05.png")
                         .scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_bgScaled);
    QWidget::paintEvent(event);
}

void Widget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}
