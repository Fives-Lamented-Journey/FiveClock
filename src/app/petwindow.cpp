#include "app/petwindow.h"
#include "ui_petwindow.h"
#include "config/config_service.h"
#include <QLabel>
#include <QMenu>
#include <QLCDNumber>
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>

// 宠物窗口类的构造函数，接收一个父窗口指针作为参数
petwindow::petwindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::petwindow)
{
    // 设置用户界面
    ui->setupUi(this);
    // 调整窗口大小为 200x200
    this->resize(200,200);
    // 创建一个 QLCDNumber 用于显示时间
    time=new QLCDNumber(this);

    // 设置 QLCDNumber 显示的数字位数为 8
    time->setDigitCount(8);
    // 设置 QLCDNumber 的位置和大小
    time->setGeometry(50,0,100,40);
    // 设置 objectName 供全局样式表匹配（去边框样式由 app.qss 的 QLCDNumber#petLcd 提供）
    time->setObjectName("petLcd");
    // 初始显示时间为 00:00:00
    time->display("00:00:00");
    // 显示 QLCDNumber
    time->show();
    // 设置窗口标志，实现无边框、置顶和工具窗口效果
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                   | Qt::Tool);
    // 设置窗口背景为透明
    setAttribute(Qt::WA_TranslucentBackground);

    // 创建右键菜单
    createContextMenu();
}

// 宠物窗口类的析构函数，释放用户界面指针
petwindow::~petwindow()
{
    delete ui;
}

// 注入配置服务
void petwindow::setConfigService(ConfigService *service)
{
    m_configService = service;
    // 重新创建菜单以使用 ConfigService 文本
    if (contextMenu) {
        delete contextMenu;
        contextMenu = nullptr;
        createContextMenu();
    }
}

// 创建右键菜单的函数
void petwindow::createContextMenu()
{
    // 创建一个 QMenu 对象作为右键菜单
    contextMenu = new QMenu(this);

    // 从 ConfigService 获取菜单文本（构造期未注入时使用默认值，注入后会重建菜单）
    auto txtStart    = m_configService ? m_configService->petMenuStart()    : QStringLiteral("开始计时");
    auto txtPause    = m_configService ? m_configService->petMenuPause()    : QStringLiteral("暂停/继续计时");
    auto txtCounter  = m_configService ? m_configService->petMenuCounter()  : QStringLiteral("分神次数+1");
    auto txtEnd      = m_configService ? m_configService->petMenuEnd()      : QStringLiteral("结束计时");
    auto txtHide     = m_configService ? m_configService->petMenuHide()     : QStringLiteral("隐藏");
    auto txtQuit     = m_configService ? m_configService->petMenuQuit()     : QStringLiteral("退出");

    // 创建动作
    QAction *startAction=new QAction(txtStart,this);
    QAction *pauseAction=new QAction(txtPause,this);
    QAction *counterAction=new QAction(txtCounter,this);
    QAction *endAction=new QAction(txtEnd,this);

    QAction *hideAction = new QAction(txtHide, this);
    QAction *quitAction = new QAction(txtQuit, this);

    // 连接信号
    connect(startAction, &QAction::triggered, this, &petwindow::emitStart);
    connect(pauseAction, &QAction::triggered, this, &petwindow::emitPause);
    connect(counterAction, &QAction::triggered, this, &petwindow::emitCounter);
    connect(endAction, &QAction::triggered, this, &petwindow::emitEnd);
    connect(hideAction, &QAction::triggered, this, &petwindow::hidePet);
    connect(quitAction, &QAction::triggered, this, &petwindow::quitapp);

    // 添加到菜单
    contextMenu->addAction(startAction);
    contextMenu->addAction(pauseAction);
    contextMenu->addAction(counterAction);
    contextMenu->addAction(endAction);
    contextMenu->addSeparator();
    contextMenu->addAction(hideAction);
    contextMenu->addAction(quitAction);
}

// 隐藏宠物窗口的槽函数
void petwindow::hidePet()
{
    hide();
}

// 退出应用程序的槽函数
void petwindow::quitapp()
{
    QCoreApplication::quit();
}

// 更新 QLCDNumber 显示时间的槽函数
void petwindow::changedLcd(QString newtime)
{
    time->display(newtime);
}

// 鼠标移动事件处理函数
void petwindow::mouseMoveEvent(QMouseEvent *event)
{
    QPoint eventPos = event->globalPosition().toPoint();
    if (!(windowState() & Qt::WindowMaximized)&&!(windowState() & Qt::WindowFullScreen)&&petMoving&&(event->buttons()&Qt::LeftButton)
        &&(eventPos-petLastPose).manhattanLength()>QApplication::startDragDistance())
    {
        move(eventPos - petLastPose);
        petLastPose = eventPos - this->pos();
    }
    return QWidget::mouseMoveEvent(event);
}

// 鼠标按下事件处理函数
void petwindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button()==Qt::LeftButton)
    {
        petMoving=true;
        petLastPose=event->globalPosition().toPoint()-this->pos();
    }
    return QWidget::mousePressEvent(event);
}

// 鼠标释放事件处理函数
void petwindow::mouseReleaseEvent(QMouseEvent *event)
{
    petMoving =false;
    event->accept();
}

// 右键菜单事件处理函数
void petwindow::contextMenuEvent(QContextMenuEvent *event)
{
    contextMenu->exec(event->globalPos());
}

// 绘制事件处理函数
void petwindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QPixmap pic(":/pic/res/pic/deskpet.png");
    painter.drawPixmap(0, 0, width(), height(), pic);
    QWidget::paintEvent(event);
}

// 窗口大小改变事件处理函数
void petwindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}
