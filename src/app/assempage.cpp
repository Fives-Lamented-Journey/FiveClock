#include "app/assempage.h"
#include "ui_assempage.h"
#include "pages/homePage.h"
#include "pages/subpage.h"
#include "pages/chartpage.h"
#include "pages/card.h"
#include "pages/infodialog.h"
#include "app/petwindow.h"
#include "widgets/customcalendarwidget.h"
#include "widgets/mybutton.h"
#include <QSystemTrayIcon>
#include <QMenu>
#include <QApplication>
#include <QPushButton>
#include <QMouseEvent>
#include <QDate>

// assemPage 类的构造函数，用于初始化页面和相关组件
assemPage::assemPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::assemPage)
{
    ui->setupUi(this);
    // dbm 为成员变量，进入构造函数体时已完全构造，此时才可注入给服务层
    // 创建记录业务服务层，注入 dbm 指针（UI → RecordService → sqlDBM）
    m_recordService = std::make_unique<RecordService>(&DBM);
    // 创建一个 petwindow 实例，用于显示桌宠
    pet = new petwindow(this);
    qDebug() << "Entering assemPage constructor";
    // 创建主页面实例
    aHomePage = new Widget(this);
    // 主页面注入记录业务服务（计时逻辑委托给服务层）
    aHomePage->setRecordService(m_recordService.get());
    // 创建子页面实例
    aSubPage = new subPage(this);
    qDebug() << "Entering achartpage constructor";
    // 创建图表页面实例
    aChartPage = new chartPage(this);
    // 页面创建完成后统一注入记录业务服务：
    // 各视图通过服务层查询记录并订阅 dataChanged 信号，不再感知 dbm
    // （注意：subPage 构造期会执行一次 showCard，此时服务尚未注入，保持原有的空记录展示行为）
    aSubPage->setRecordService(m_recordService.get());
    aSubPage->calendar->setRecordService(m_recordService.get());
    aChartPage->setRecordService(m_recordService.get());
    qDebug() << "Exiting achartpage constructor";
    // 创建系统托盘图标实例
    trayIcon = new QSystemTrayIcon(this);
    // 设置托盘图标
    trayIcon->setIcon(QIcon(":/icon/res/icon/window_icon.png"));
    // 设置托盘图标提示信息
    trayIcon->setToolTip("Five's clock");

    // 创建托盘图标菜单
    createTrayIconMenu();
    // 显示托盘图标
    trayIcon->show();
    // 设置托盘图标上下文菜单
    trayIcon->setContextMenu(trayMenu);

    // 将主页面、子页面和图表页面添加到堆叠窗口中
    ui->stackedWidget->addWidget(aHomePage);
    ui->stackedWidget->addWidget(aSubPage);
    ui->stackedWidget->addWidget(aChartPage);
    // 设置当前显示的页面为主页面
    ui->stackedWidget->setCurrentWidget(aHomePage);

    // 连接主页面的子页面按钮点击信号到页面切换槽函数
    connect(aHomePage->subButton, &QPushButton::clicked, this, &assemPage::pageChangeToSub);
    // 连接主页面的图表页面按钮点击信号到页面切换槽函数
    connect(aHomePage->chartButton, &QPushButton::clicked, this, &assemPage::pageChangeToChart);

    // 连接 RecordService 的计时结束信号到添加记录槽函数（携带快照数据）
    bool res = connect(m_recordService.get(), &RecordService::sig_timerEnded, this, &assemPage::assAddRecord);
    qDebug() << "槽函数是否连接" << res;
    // 连接主页面的开始工作信号到提示工作中槽函数
    connect(aHomePage, &Widget::workingNow, this, &assemPage::tipWorkingNow);
    // 连接主页面的未开始工作信号到提示未工作槽函数
    connect(aHomePage, &Widget::noWorkingNow, this, &assemPage::tipNoWorkingNow);
    // 连接主页面的暂停工作信号到提示暂停槽函数
    connect(aHomePage, &Widget::pausingNow, this, &assemPage::tipPausingNow);

    // 连接主页面的 LCD 显示变化信号到桌宠的更新 LCD 显示槽函数
    connect(aHomePage, &Widget::lcdChanged, pet, &petwindow::changedLcd);

    // 连接子页面的主页面按钮点击信号到页面切换槽函数
    connect(aSubPage->homeButton, &QPushButton::clicked, this, &assemPage::pageChangeToHome);
    // 连接子页面的图表页面按钮点击信号到页面切换槽函数
    connect(aSubPage->chartButton, &QPushButton::clicked, this, &assemPage::pageChangeToChart);

    // 连接图表页面的主页面按钮点击信号到页面切换槽函数
    connect(aChartPage->homeButton, &QPushButton::clicked, this, &assemPage::pageChangeToHome);
    // 连接图表页面的子页面按钮点击信号到页面切换槽函数
    connect(aChartPage->subButton, &QPushButton::clicked, this, &assemPage::pageChangeToSub);

    // 连接子页面的更新记录信号到更新记录槽函数
    connect(aSubPage, &subPage::updateRecord, this, &assemPage::assUpdateRecord);
    // 连接子页面的删除记录信号到删除记录槽函数
    connect(aSubPage, &subPage::deleteRecord, this, &assemPage::assDeleteRecord);

    // 连接子页面的添加空记录按钮点击信号到添加空记录槽函数
    connect(aSubPage->subAddRecordButton, &QPushButton::clicked, this, &assemPage::assAddEmptyRecord);
    // connect(aHomePage,&Widget::homeAddRecord,this,&assemPage::assAddRecord);
    // 连接主页面的全屏按钮点击信号到设置屏幕状态槽函数
    connect(aHomePage->fullScreenButton, &QPushButton::clicked, this, &assemPage::SetScreenState);
    // 连接主页面的退出按钮点击信号到退出窗口槽函数
    connect(aHomePage->exitButton, &QPushButton::clicked, this, &assemPage::exitWindow);

    // 连接托盘图标激活信号到托盘图标激活处理槽函数
    connect(trayIcon, &QSystemTrayIcon::activated, this, &assemPage::onTrayIconActivated);

    // 连接桌宠的开始信号到主页面的开始按钮点击槽函数
    connect(pet, &petwindow::emitStart, aHomePage, &Widget::onStartButtonClicked);
    // 连接桌宠的暂停信号到主页面的暂停按钮点击槽函数
    connect(pet, &petwindow::emitPause, aHomePage, &Widget::onPauseButtonClicked);
    // 连接桌宠的计数器信号到主页面的计数器按钮点击槽函数
    connect(pet, &petwindow::emitCounter, aHomePage, &Widget::onCounterButtonClicked);
    // 连接桌宠的结束信号到主页面的结束按钮点击槽函数
    connect(pet, &petwindow::emitEnd, aHomePage, &Widget::onEndButtonClicked);
    // 连接桌宠的结束信号到 RecordService::timerEnd（统一经过计时链路，再触发 sig_timerEnded → assAddRecord）
    connect(pet, &petwindow::emitEnd, m_recordService.get(), &RecordService::timerEnd);

    qDebug() << "Exiting achart draw";
    // 绘制图表
    aChartPage->drawChart();
    qDebug() << "Exiting assemPage constructor";
}

// assemPage 类的析构函数，用于释放资源
assemPage::~assemPage()
{
    delete ui;
}

// 创建托盘图标菜单
void assemPage::createTrayIconMenu()
{
    // 创建托盘菜单实例
    trayMenu = new QMenu(this);
    // 创建显示主窗口动作
    QAction *showMainAction = new QAction("显示主窗口", this);
    // 创建显示桌宠动作
    QAction *showPetAction = new QAction("显示鹤鹤", this);
    // 创建退出程序动作
    QAction *quitAction = new QAction("退出程序", this);
    // 连接显示主窗口动作的触发信号到显示主窗口槽函数
    connect(showMainAction, &QAction::triggered, this,
            &Widget::show);
    // 连接显示主窗口动作的触发信号到隐藏桌宠槽函数
    connect(showMainAction, &QAction::triggered, pet,
            &petwindow::hide);
    // 连接显示桌宠动作的触发信号到显示桌宠槽函数
    connect(showPetAction, &QAction::triggered, pet,
            &petwindow::show);
    // 连接退出程序动作的触发信号到退出应用程序槽函数
    connect(quitAction, &QAction::triggered, qApp,
            &QApplication::quit);
    // 将显示主窗口动作添加到托盘菜单
    trayMenu->addAction(showMainAction);
    // 将显示桌宠动作添加到托盘菜单
    trayMenu->addAction(showPetAction);
    // 添加分隔线
    trayMenu->addSeparator();
    // 将退出程序动作添加到托盘菜单
    trayMenu->addAction(quitAction);
}

// 切换到主页面
void assemPage::pageChangeToHome()
{
    // 设置当前显示的页面为主页面
    ui->stackedWidget->setCurrentWidget(aHomePage);
    // 重绘页面
    repaint();
}

// 切换到子页面
void assemPage::pageChangeToSub()
{
    // 设置当前显示的页面为子页面
    ui->stackedWidget->setCurrentWidget(aSubPage);
    // 重绘页面
    repaint();
}

// 切换到图表页面
void assemPage::pageChangeToChart()
{
    // 设置当前显示的页面为图表页面
    ui->stackedWidget->setCurrentWidget(aChartPage);
    // 重绘页面
    repaint();
}

// 设置屏幕状态（全屏/正常）
void assemPage::SetScreenState()
{
    if (!isFullScreen())
    {
        // 设置窗口为全屏状态
        setWindowState(Qt::WindowFullScreen);
        // 更新窗口
        update();
    }
    else
    {
        // 设置窗口为正常状态
        setWindowState(Qt::WindowNoState);
        // 更新窗口
        update();
    }
}

// 退出窗口
void assemPage::exitWindow()
{
    // 关闭窗口
    window()->close();
}

// 添加记录（计时结束落库：由 RecordService::sig_timerEnded 携带快照数据触发）
void assemPage::assAddRecord(const TimerEndSnapshot &snap)
{
    // 从快照中提取计时数据（替代原 aHomePage 成员变量读取）
    const QString &startTime = snap.startTime;
    const QString &endTime = snap.endTime;
    const int consumTime = snap.consumMin;
    const int counter = snap.counter;

    // 创建信息对话框实例（栈对象：exec 为模态阻塞，返回后自动析构，无需手动 delete）
    infoDialog assInfoDialog(this, startTime, endTime, consumTime, counter);

    if (assInfoDialog.exec() == QDialog::Accepted)
    {
        qDebug("exec01");
        // 获取当前日期
        assInfoDialog.today = assInfoDialog.getTodayTime();
        // 获取详细信息
        assInfoDialog.detail = assInfoDialog.getdetailText();
        // 获取分类信息
        assInfoDialog.classfy = assInfoDialog.getclassBox();
        qDebug("exec02");
        // 计时结束落库：改走业务服务层（UI → RecordService → sqlDBM），SQL 逻辑复用底层实现
        // 写入成功后由 RecordService::dataChanged 通知日历、图表、卡片列表自动刷新
        bool ok = m_recordService->saveTimeRecord(assInfoDialog.today,
                                                  assInfoDialog.startTime, assInfoDialog.endTime,
                                                  assInfoDialog.consumTime, assInfoDialog.counter,
                                                  assInfoDialog.detail, assInfoDialog.classfy);
        if (ok) {
            // 落库成功：确认计时记录已保存，清 TimerModel 的 isWorking 状态
            m_recordService->timerMarkSaved();
        }
    }
    else
    {
        // 用户取消保存：计时记录未落库，isWorking 保持 true，允许重试
        qDebug("exec02");
    }
}

// 更新记录
void assemPage::assUpdateRecord(card *oneCard)
{
    // 更新记录改走业务服务层：从 card 字段构造 TimeRecord，服务层转发至 sqlDBM::updateRecord
    TimeRecord rec;
    rec.eventId = oneCard->cardEverntId;
    rec.begTime = oneCard->cardBegTime;
    rec.endTime = oneCard->cardEndTime;
    rec.consumTime = oneCard->cardConsuTime;
    rec.counter = oneCard->cardcounter;
    rec.detail = oneCard->cardDetail;
    rec.eventClass = oneCard->cardClass;
    // 成功后由 RecordService::dataChanged 通知各视图刷新
    m_recordService->updateRecord(QDate::fromString(oneCard->cardCreateDate, "yyyy-MM-dd"), rec);
}

// 删除记录
void assemPage::assDeleteRecord(card *oneCard)
{
    // 删除记录改走业务服务层（eventID 按日分表自增，需日期+ID 共同定位）
    // （删除成功后由 RecordService::dataChanged 通知各视图自动刷新，无需再判断表是否清空）
    m_recordService->deleteRecord(QDate::fromString(oneCard->cardCreateDate, "yyyy-MM-dd"),
                                  oneCard->cardEverntId);
}

// 添加空记录
void assemPage::assAddEmptyRecord()
{
    // 创建信息对话框实例（栈对象：exec 为模态阻塞，返回后自动析构，无需手动 delete）
    infoDialog assInfoDialog(this, "", "", 0, 0, "", true);

    if (assInfoDialog.exec() == QDialog::Accepted)
    {
        qDebug("exec01");
        // 获取子页面选择的日期
        assInfoDialog.today = aSubPage->selectedToday;

        // 获取开始时间
        assInfoDialog.startTime = assInfoDialog.getstartLine();
        // 获取结束时间
        assInfoDialog.endTime = assInfoDialog.getendLine();
        // 获取消耗时间
        assInfoDialog.consumTime = assInfoDialog.getconsumBox();
        // 获取计数器值
        assInfoDialog.counter = assInfoDialog.getcounterBox();
        // 获取分类信息
        assInfoDialog.classfy = assInfoDialog.getclassBox();
        // 获取详细信息
        assInfoDialog.detail = assInfoDialog.getdetailText();

        qDebug() << assInfoDialog.today;
        qDebug() << assInfoDialog.detail;
        qDebug() << assInfoDialog.classfy;
        // 插入记录改走业务服务层：从 infoDialog 字段构造 TimeRecord
        // 写入成功后由 RecordService::dataChanged 通知卡片列表、日历、图表自动刷新
        TimeRecord rec;
        rec.begTime = assInfoDialog.startTime;
        rec.endTime = assInfoDialog.endTime;
        rec.consumTime = assInfoDialog.consumTime;
        rec.counter = assInfoDialog.counter;
        rec.detail = assInfoDialog.detail;
        rec.eventClass = assInfoDialog.classfy;
        m_recordService->addEmptyRecord(QDate::fromString(assInfoDialog.today, "yyyy-MM-dd"), rec);
    }
    else
    {
        qDebug("exec02");
    }
}

// 处理托盘图标激活事件
void assemPage::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::Trigger:
        if (isVisible())
        {
            // 如果窗口可见，则隐藏窗口并显示桌宠
            hide();
            pet->show();
        }
        else
        {
            // 如果窗口不可见，则显示窗口并隐藏桌宠
            show();
            raise();
            pet->hide();
        }
        break;
    case QSystemTrayIcon::DoubleClick:
        // 双击托盘图标显示主窗口，隐藏桌宠
        show();
        pet->hide();
        break;
    default:
        break;
    }
}

// 提示正在工作
void assemPage::tipWorkingNow()
{
    // 显示系统托盘消息，提示正在工作
    trayIcon->showMessage("小家伙", "已经在计时了哦",
                          QSystemTrayIcon::Information, 2000);
}

// 提示未开始工作
void assemPage::tipNoWorkingNow()
{
    // 显示系统托盘消息，提示未开始工作
    trayIcon->showMessage("小家伙", "还没有开始计时哦",
                          QSystemTrayIcon::Information, 2000);
}

// 提示暂停工作
void assemPage::tipPausingNow()
{
    // 显示系统托盘消息，提示暂停工作
    trayIcon->showMessage("小家伙", "还没有开始计时哦",
                          QSystemTrayIcon::Information, 2000);
}

// 处理鼠标按下事件
void assemPage::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // 标记窗口正在移动
        assMoving = true;
        // 记录鼠标按下时的相对位置
        assLastPose = event->globalPosition().toPoint() - this->pos();
    }
    return QWidget::mousePressEvent(event);
    // QWidget::mousePressEvent(event);
}

// 处理鼠标释放事件
void assemPage::mouseReleaseEvent(QMouseEvent *event)
{
    // 标记窗口停止移动
    assMoving = false;
    event->accept();
}

// 处理鼠标移动事件
void assemPage::mouseMoveEvent(QMouseEvent *event)
{
    QPoint eventPos = event->globalPosition().toPoint();
    if (!(windowState() & Qt::WindowMaximized) && !(windowState() & Qt::WindowFullScreen) && assMoving && (event->buttons() & Qt::LeftButton)
        && (eventPos - assLastPose).manhattanLength() > QApplication::startDragDistance())
    {
        // 如果窗口不是最大化或全屏状态，且正在移动，且鼠标左键按下，且移动距离超过拖动阈值，则移动窗口
        move(eventPos - assLastPose);
        // 更新鼠标相对位置
        assLastPose = eventPos - this->pos();
    }
    return QWidget::mouseMoveEvent(event);
}

// 处理窗口关闭事件
void assemPage::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible())
    {
        // 如果托盘图标可见，则隐藏窗口并显示桌宠，忽略关闭事件
        hide();
        pet->show();
        event->ignore();
    }
}

// 处理窗口状态变化事件
void assemPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange && isMinimized())
    {
        // 如果窗口状态变为最小化，则忽略事件，隐藏窗口并显示桌宠
        event->ignore();
        hide();
        pet->show();
    }
    else
    {
        // 否则调用基类的事件处理函数
        QWidget::changeEvent(event);
    }
}
