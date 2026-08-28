#include "app/assempage.h"
#include "ui_assempage.h"
#include "pages/homePage.h"
#include "pages/subpage.h"
#include "pages/chartpage.h"
#include "pages/infodialog.h"
#include "app/petwindow.h"
#include "widgets/customcalendarwidget.h"
#include "widgets/mybutton.h"
#include "service/record_service.h"
#include "service/timer_service.h"
#include "service/notification_service.h"
#include "config/config_service.h"
#include <QSystemTrayIcon>
#include <QMenu>
#include <QApplication>
#include <QPushButton>
#include <QMouseEvent>

// assemPage 类的构造函数（B-14b：只保留装配顺序，实现细节分解到各 init/connect 方法）
assemPage::assemPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::assemPage)
{
    ui->setupUi(this);

    initServices();
    initPages();
    initTrayIcon();
    connectSignals();

    // 首次绘制图表（页面数据初始化）
    qDebug() << "Exiting achart draw";
    aChartPage->drawChart();
    qDebug() << "Exiting assemPage constructor";
}

// 服务实例化与相互注入
// 顺序敏感：DBM 为值成员进入构造函数体时已完全构造 → RecordService → TimerService → ConfigService
void assemPage::initServices()
{
    // 1. 创建 RecordService，注入 dbm 指针
    m_recordService = std::make_unique<RecordService>(&DBM);
    // 2. 创建 TimerService（内部持有 TimerModel，不依赖 dbm）
    m_timerService = std::make_unique<TimerService>();
    // 3. 创建 ConfigService（集中管理常量与配置）
    m_configService = std::make_unique<ConfigService>();
    // 4. 将 ConfigService 注入 TimerService（转发给 TimerModel 读取计时常量）
    m_timerService->setConfigService(m_configService.get());
}

// 页面/桌宠实例化与服务注入 + stackedWidget 组装
void assemPage::initPages()
{
    // 创建一个 petwindow 实例，用于显示桌宠
    pet = new petwindow(this);
    qDebug() << "Entering assemPage constructor";

    // 创建主页面实例，同时注入两个服务：TimerService（计时）+ RecordService（记录）
    aHomePage = new Widget(this);
    aHomePage->setTimerService(m_timerService.get());
    aHomePage->setRecordService(m_recordService.get());

    // 创建子页面实例
    aSubPage = new subPage(this);
    qDebug() << "Entering achartpage constructor";
    // 创建图表页面实例
    aChartPage = new chartPage(this);

    // 子页面/图表页面只注入 RecordService（查询/记录操作）
    aSubPage->setRecordService(m_recordService.get());
    aSubPage->setConfigService(m_configService.get());
    aSubPage->calendar->setRecordService(m_recordService.get());
    aChartPage->setRecordService(m_recordService.get());

    // 桌宠注入 ConfigService（菜单文本从配置读取）
    pet->setConfigService(m_configService.get());
    qDebug() << "Exiting achartpage constructor";

    // 将页面添加到堆叠窗口，默认显示主页
    ui->stackedWidget->addWidget(aHomePage);
    ui->stackedWidget->addWidget(aSubPage);
    ui->stackedWidget->addWidget(aChartPage);
    ui->stackedWidget->setCurrentWidget(aHomePage);
}

// 托盘图标 + 右键菜单 + NotificationService（封装托盘通知逻辑）
void assemPage::initTrayIcon()
{
    // 创建系统托盘图标实例
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/icon/res/icon/window_icon.png"));
    trayIcon->setToolTip(m_configService->appTrayTooltip());
    createTrayIconMenu();
    trayIcon->show();
    trayIcon->setContextMenu(trayMenu);

    // 创建 NotificationService（依赖已创建的 trayIcon 与 ConfigService）
    m_notificationService = std::make_unique<NotificationService>(trayIcon, this);
    m_notificationService->setConfigService(m_configService.get());
}

// 全部信号槽连接（组装层桥接的唯一入口）
void assemPage::connectSignals()
{
    // 托盘激活信号转发给 assempage（处理显示/隐藏主窗口逻辑）
    connect(m_notificationService.get(), &NotificationService::trayActivated,
            this, &assemPage::onTrayIconActivated);

    // 页面切换（主页 → 子页/图表页）
    connect(aHomePage->subButton, &QPushButton::clicked, this, &assemPage::pageChangeToSub);
    connect(aHomePage->chartButton, &QPushButton::clicked, this, &assemPage::pageChangeToChart);

    // 信号桥接：TimerService 计时结束 → assAddRecord（携带快照 → 对话框 → RecordService 保存）
    bool res = connect(m_timerService.get(), &TimerService::sig_timerEnded, this, &assemPage::assAddRecord);
    qDebug() << "槽函数是否连接" << res;

    // 主页面临时提醒 → NotificationService（语义化通知）
    connect(aHomePage, &Widget::workingNow,
            m_notificationService.get(), &NotificationService::notifyAlreadyWorking);
    connect(aHomePage, &Widget::noWorkingNow,
            m_notificationService.get(), &NotificationService::notifyNotWorking);
    connect(aHomePage, &Widget::pausingNow,
            m_notificationService.get(), &NotificationService::notifyNotWorking);

    // LCD 变化 → 桌宠
    connect(aHomePage, &Widget::lcdChanged, pet, &petwindow::changedLcd);

    // 子页面/图表页面切换
    connect(aSubPage->homeButton, &QPushButton::clicked, this, &assemPage::pageChangeToHome);
    connect(aSubPage->chartButton, &QPushButton::clicked, this, &assemPage::pageChangeToChart);
    connect(aChartPage->homeButton, &QPushButton::clicked, this, &assemPage::pageChangeToHome);
    connect(aChartPage->subButton, &QPushButton::clicked, this, &assemPage::pageChangeToSub);

    // 主页其他按钮
    connect(aHomePage->fullScreenButton, &QPushButton::clicked, this, &assemPage::SetScreenState);
    connect(aHomePage->exitButton, &QPushButton::clicked, this, &assemPage::exitWindow);

    // 桌宠 → 主页/TimerService（统一经过计时链路）
    connect(pet, &petwindow::emitStart, aHomePage, &Widget::onStartButtonClicked);
    connect(pet, &petwindow::emitPause, aHomePage, &Widget::onPauseButtonClicked);
    connect(pet, &petwindow::emitCounter, aHomePage, &Widget::onCounterButtonClicked);
    connect(pet, &petwindow::emitEnd, aHomePage, &Widget::onEndButtonClicked);
    connect(pet, &petwindow::emitEnd, m_timerService.get(), &TimerService::timerEnd);
}

assemPage::~assemPage()
{
    delete ui;
}

void assemPage::createTrayIconMenu()
{
    trayMenu = new QMenu(this);
    // 菜单文本统一从 ConfigService 读取（B-14c：消除硬编码）
    QAction *showMainAction = new QAction(m_configService->trayMenuShowMain(), this);
    QAction *showPetAction = new QAction(m_configService->trayMenuShowPet(), this);
    QAction *quitAction = new QAction(m_configService->trayMenuQuit(), this);
    connect(showMainAction, &QAction::triggered, this, &Widget::show);
    connect(showMainAction, &QAction::triggered, pet, &petwindow::hide);
    connect(showPetAction, &QAction::triggered, pet, &petwindow::show);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    trayMenu->addAction(showMainAction);
    trayMenu->addAction(showPetAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);
}

void assemPage::pageChangeToHome() { ui->stackedWidget->setCurrentWidget(aHomePage); repaint(); }
void assemPage::pageChangeToSub()   { ui->stackedWidget->setCurrentWidget(aSubPage); repaint(); }
void assemPage::pageChangeToChart() { ui->stackedWidget->setCurrentWidget(aChartPage); repaint(); }

void assemPage::SetScreenState()
{
    if (!isFullScreen()) setWindowState(Qt::WindowFullScreen);
    else setWindowState(Qt::WindowNoState);
    update();
}

void assemPage::exitWindow() { window()->close(); }

// 计时结束落库：由 TimerService::sig_timerEnded 触发
// 桥接：TimerEndSnapshot → infoDialog 补充日期/描述/分类 → RecordService::saveFromSnapshot
// 注：卡片 CRUD 已下沉至 subPage，此处只处理跨服务桥接
void assemPage::assAddRecord(const TimerEndSnapshot &snap)
{
    const QString &startTime = snap.startTime;
    const QString &endTime = snap.endTime;
    const int consumTime = snap.consumMin;
    const int counter = snap.counter;

    infoDialog assInfoDialog(this, startTime, endTime, consumTime, counter);

    if (assInfoDialog.exec() == QDialog::Accepted)
    {
        // 直接通过 getter 收集用户输入，不再读写 dialog 公共成员
        QString date = assInfoDialog.getTodayTime();
        QString detail = assInfoDialog.getdetailText();
        QString classfy = assInfoDialog.getclassBox();

        // 桥接：快照 + 对话框补充 → RecordService::saveFromSnapshot
        bool ok = m_recordService->saveFromSnapshot(snap, date, detail, classfy);
        if (ok) {
            // 落库成功：通知 TimerService 清 isWorking
            m_timerService->timerMarkSaved();
        }
    }
}

void assemPage::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        if (isVisible()) { hide(); pet->show(); }
        else { show(); raise(); pet->hide(); }
        break;
    case QSystemTrayIcon::DoubleClick:
        show(); pet->hide();
        break;
    default: break;
    }
}

void assemPage::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        assMoving = true;
        assLastPose = event->globalPosition().toPoint() - this->pos();
    }
    return QWidget::mousePressEvent(event);
}

void assemPage::mouseReleaseEvent(QMouseEvent *event)
{
    assMoving = false;
    event->accept();
}

void assemPage::mouseMoveEvent(QMouseEvent *event)
{
    QPoint eventPos = event->globalPosition().toPoint();
    if (!(windowState() & Qt::WindowMaximized) && !(windowState() & Qt::WindowFullScreen) && assMoving && (event->buttons() & Qt::LeftButton)
        && (eventPos - assLastPose).manhattanLength() > QApplication::startDragDistance())
    {
        move(eventPos - assLastPose);
        assLastPose = eventPos - this->pos();
    }
    return QWidget::mouseMoveEvent(event);
}

void assemPage::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible()) {
        hide();
        pet->show();
        event->ignore();
    }
}

void assemPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange && isMinimized()) {
        event->ignore();
        hide();
        pet->show();
    } else {
        QWidget::changeEvent(event);
    }
}
