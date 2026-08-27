#include "assempage.h"
#include "ui_assempage.h"

// assemPage 类的构造函数，用于初始化页面和相关组件
assemPage::assemPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::assemPage)
{
    ui->setupUi(this);
    // 创建一个 petwindow 实例，用于显示桌宠
    pet = new petwindow(this);
    qDebug() << "Entering assemPage constructor";
    // 创建主页面实例
    aHomePage = new Widget(this);
    // 创建子页面实例
    aSubPage = new subPage(this);
    qDebug() << "Entering achartpage constructor";
    // 创建图表页面实例
    aChartPage = new chartPage(this);
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

    // 连接主页面的结束按钮点击信号到添加记录槽函数
    bool res = connect(aHomePage->endbutton, &QPushButton::clicked, this, &assemPage::assAddRecord);
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

    // 连接子页面的排序记录信号到排序记录槽函数
    connect(aSubPage, &subPage::sortedRecord, this, &assemPage::assSortedRecord);
    // 连接子页面的更新记录信号到更新记录槽函数
    connect(aSubPage, &subPage::updateRecord, this, &assemPage::assUpdateRecord);
    // 连接子页面的删除记录信号到删除记录槽函数
    connect(aSubPage, &subPage::deleteRecord, this, &assemPage::assDeleteRecord);
    // 连接子页面的日历按钮样式设置信号到设置日期按钮样式槽函数
    connect(aSubPage->calendar, &CustomCalendarWidget::setButtonStyle, this, &assemPage::setDayButtonStyle);

    // 连接子页面的添加空记录按钮点击信号到添加空记录槽函数
    connect(aSubPage->subAddRecordButton, &QPushButton::clicked, this, &assemPage::assAddEmptyRecord);
    // connect(aHomePage,&Widget::homeAddRecord,this,&assemPage::assAddRecord);
    // 连接主页面的全屏按钮点击信号到设置屏幕状态槽函数
    connect(aHomePage->fullScreenButton, &QPushButton::clicked, this, &assemPage::SetScreenState);
    // 连接主页面的退出按钮点击信号到退出窗口槽函数
    connect(aHomePage->exitButton, &QPushButton::clicked, this, &assemPage::exitWindow);

    // 连接图表页面的获取数据信号到返回记录槽函数
    connect(aChartPage, &chartPage::getData, this, &assemPage::assReturnRecord);
    // 连接图表页面的获取周数据信号到返回周记录槽函数
    connect(aChartPage, &chartPage::getWeekData, this, &assemPage::assReturnWeekRecord);

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
    // 连接桌宠的结束信号到添加记录槽函数
    connect(pet, &petwindow::emitEnd, this, &assemPage::assAddRecord);

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

// 添加记录
void assemPage::assAddRecord()
{
    if (aHomePage->isWorking == false)
    {
        return;
    }
    // 标记工作结束
    aHomePage->isWorking = false;

    // 创建信息对话框实例
    assInfoDialog = new infoDialog(this, aHomePage->startTime, aHomePage->endTime,
                                   aHomePage->consumTime, aHomePage->counter);

    if (assInfoDialog->exec() == QDialog::Accepted)
    {
        qDebug("exec01");
        // 获取当前日期
        assInfoDialog->today = assInfoDialog->getTodayTime();
        // 获取详细信息
        assInfoDialog->detail = assInfoDialog->getdetailText();
        // 获取分类信息
        assInfoDialog->classfy = assInfoDialog->getclassBox();
        // connect(assInfoDialog,&infoDialog::infoAddRecord);
        // assInfoDialog->addRecord();
        qDebug("exec02");
        if (!DBM.tableExits(assInfoDialog->today))
        {
            // 如果表不存在，则创建表
            DBM.creatTable(assInfoDialog->today);
            // 插入数据到表中
            DBM.insertData(assInfoDialog->today,
                           assInfoDialog->startTime, assInfoDialog->endTime,
                           assInfoDialog->consumTime, assInfoDialog->counter,
                           assInfoDialog->detail, assInfoDialog->classfy);
            qDebug("addRecord02");
        }
        else
        {
            // 插入数据到已存在的表中
            DBM.insertData(assInfoDialog->today, assInfoDialog->startTime, assInfoDialog->endTime,
                           assInfoDialog->consumTime, assInfoDialog->counter, assInfoDialog->detail,
                           assInfoDialog->classfy);
            qDebug("addRecord03");
        }
        // 释放信息对话框资源
        delete assInfoDialog;
        // 更新日历
        aSubPage->calendar->updateCalendar();
        // 绘制饼图
        aChartPage->drawPieChart();
    }
    else
    {
        qDebug("exec02");
        // 释放信息对话框资源
        delete assInfoDialog;
    }
}

// 更新记录
void assemPage::assUpdateRecord(card *oneCard)
{
    // 调用数据库管理类的更新记录方法
    DBM.updateRecord(oneCard->cardCreateDate, oneCard->cardEverntId,
                     oneCard->cardBegTime, oneCard->cardEndTime,
                     oneCard->cardConsuTime, oneCard->cardcounter,
                     oneCard->cardDetail, oneCard->cardClass);
}

// 删除记录
void assemPage::assDeleteRecord(card *oneCard)
{
    // 调用数据库管理类的删除记录方法
    DBM.deleteRecord(oneCard->cardCreateDate, oneCard->cardEverntId);
    if (DBM.getRowCount(oneCard->cardCreateDate) == 0)
    {
        // 如果表中记录数为 0，则更新日历和绘制饼图
        aSubPage->calendar->updateCalendar();
        aChartPage->drawPieChart();
    }
}

// 添加空记录
void assemPage::assAddEmptyRecord()
{
    // 创建信息对话框实例，用于添加空记录
    assInfoDialog = new infoDialog(this, "", "", 0, 0, "", true);

    if (assInfoDialog->exec() == QDialog::Accepted)
    {
        qDebug("exec01");
        // 获取子页面选择的日期
        assInfoDialog->today = aSubPage->selectedToday;

        // 获取开始时间
        assInfoDialog->startTime = assInfoDialog->getstartLine();
        // 获取结束时间
        assInfoDialog->endTime = assInfoDialog->getendLine();
        // 获取消耗时间
        assInfoDialog->consumTime = assInfoDialog->getconsumBox();
        // 获取计数器值
        assInfoDialog->counter = assInfoDialog->getcounterBox();
        // 获取分类信息
        assInfoDialog->classfy = assInfoDialog->getclassBox();
        // 获取详细信息
        assInfoDialog->detail = assInfoDialog->getdetailText();

        qDebug() << assInfoDialog->today;
        qDebug() << assInfoDialog->detail;
        qDebug() << assInfoDialog->classfy;
        // connect(assInfoDialog,&infoDialog::infoAddRecord,);
        // assInfoDialog->addRecord();
        if (!DBM.tableExits(assInfoDialog->today))
        {
            // 如果表不存在，则创建表
            DBM.creatTable(assInfoDialog->today);
            // 插入数据到表中
            DBM.insertData(assInfoDialog->today,
                           assInfoDialog->startTime, assInfoDialog->endTime,
                           assInfoDialog->consumTime, assInfoDialog->counter,
                           assInfoDialog->detail, assInfoDialog->classfy);
            qDebug("addRecord02");
        }
        else
        {
            // 插入数据到已存在的表中
            DBM.insertData(assInfoDialog->today, assInfoDialog->startTime, assInfoDialog->endTime,
                           assInfoDialog->consumTime, assInfoDialog->counter, assInfoDialog->detail,
                           assInfoDialog->classfy);
            qDebug("addRecord03");
        }
        // 显示卡片
        aSubPage->showCard();
        // 释放信息对话框资源
        delete assInfoDialog;
        // 更新日历
        aSubPage->calendar->updateCalendar();
        aChartPage->drawChart();
    }
    else
    {
        qDebug("exec02");
        // 释放信息对话框资源
        delete assInfoDialog;
    }
}

// 对记录进行排序
void assemPage::assSortedRecord(QSqlQuery &query, QString oneDay)
{
    // 调用数据库管理类的排序记录方法
    query = DBM.sortedRecord(oneDay);
}

// 返回指定日期的记录
void assemPage::assReturnRecord(QSqlQuery &query, QString selectedDay)
{
    if (!DBM.tableExits(selectedDay))
    {
        // 如果表不存在，则返回空查询
        query = QSqlQuery();
        return;
    }
    qDebug() << "assReturnRecord called with selectedDay:" << selectedDay;
    // 调用数据库管理类的返回消耗时间记录方法
    query = DBM.returnConsuTime(selectedDay);
}

// 返回指定周的记录
void assemPage::assReturnWeekRecord(QSqlQuery &query, QStringList selectedWeek)
{
    // 调用数据库管理类的返回周总计时间记录方法
    query = DBM.returnSumCounterTime(selectedWeek);
}

// 设置日期按钮样式
void assemPage::setDayButtonStyle(QPushButton *dayButton, QDate day)
{
    // 定义空日期按钮样式
    QString dayEmptyButtonStyle = QString("QPushButton {"
                                          "    background-color:rgba(224,224,224,128);"
                                          "    color: #333333;"
                                          "    border: none;"
                                          "    padding: 0px;"
                                          "    border-radius: 5px;"
                                          "}"
                                          "QPushButton:hover {"
                                          "    background-color:rgba(255,255,255,128);"
                                          "}");
    // 定义非空日期按钮样式
    QString dayNoEmptyButtonStyle = QString("QPushButton {"
                                            "    background-color:rgba(152 ,245 ,255,128);"
                                            "    color: #333333;"
                                            "    border: none;"
                                            "    padding: 0px;"
                                            "    border-radius: 5px;"
                                            "}"
                                            "QPushButton:hover {"
                                            "    background-color:rgba(152 ,245 ,255,200);"
                                            "}");

    // 将日期转换为字符串格式
    QString strDay = day.toString("yyyy-MM-dd");
    if (DBM.tableExits(strDay) && DBM.getRowCount(strDay) > 0)
    {
        // 如果表存在且有记录，则设置非空日期按钮样式
        dayButton->setStyleSheet(dayNoEmptyButtonStyle);
    }
    else
    {
        // 否则设置空日期按钮样式
        dayButton->setStyleSheet(dayEmptyButtonStyle);
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
