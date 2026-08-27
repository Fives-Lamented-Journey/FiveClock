#include "petwindow.h"
#include "ui_petwindow.h"
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
    // 设置 QLCDNumber 的样式表，去除边框
    time->setStyleSheet("border:none");
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

// 创建右键菜单的函数
void petwindow::createContextMenu()
{
    // 创建一个 QMenu 对象作为右键菜单
    contextMenu = new QMenu(this);
    // 创建“开始计时”动作
    QAction *startAction=new QAction("开始计时",this);
    // 创建“暂停/继续计时”动作
    QAction *pauseAction=new QAction("暂停/继续计时",this);
    // 创建“分神次数+1”动作
    QAction *counterAction=new QAction("分神次数+1",this);
    // 创建“结束计时”动作
    QAction *endAction=new QAction("结束计时",this);

    // 创建“隐藏”动作
    QAction *hideAction = new QAction("隐藏", this);
    // 创建“退出”动作
    QAction *quitAction = new QAction("退出", this);

    // 连接“开始计时”动作的触发信号到 emitStart 槽函数
    connect(startAction, &QAction::triggered, this,
            &petwindow::emitStart);
    // 连接“暂停/继续计时”动作的触发信号到 emitPause 槽函数
    connect(pauseAction, &QAction::triggered, this,
            &petwindow::emitPause);
    // 连接“分神次数+1”动作的触发信号到 emitCounter 槽函数
    connect(counterAction, &QAction::triggered, this,
            &petwindow::emitCounter);
    // 连接“结束计时”动作的触发信号到 emitEnd 槽函数
    connect(endAction, &QAction::triggered, this,
            &petwindow::emitEnd);

    // 连接“隐藏”动作的触发信号到 hidePet 槽函数
    connect(hideAction, &QAction::triggered, this,
            &petwindow::hidePet);
    // 连接“退出”动作的触发信号到 quitapp 槽函数
    connect(quitAction, &QAction::triggered, this,
            &petwindow::quitapp);

    // 将“开始计时”动作添加到右键菜单
    contextMenu->addAction(startAction);
    // 将“暂停/继续计时”动作添加到右键菜单
    contextMenu->addAction(pauseAction);
    // 将“分神次数+1”动作添加到右键菜单
    contextMenu->addAction(counterAction);
    // 将“结束计时”动作添加到右键菜单
    contextMenu->addAction(endAction);
    // 在菜单中添加分隔线
    contextMenu->addSeparator();

    // 将“隐藏”动作添加到右键菜单
    contextMenu->addAction(hideAction);
    // 将“退出”动作添加到右键菜单
    contextMenu->addAction(quitAction);
}

// 隐藏宠物窗口的槽函数
void petwindow::hidePet()
{
    // 隐藏窗口
    hide();
}

// 退出应用程序的槽函数
void petwindow::quitapp()
{
    // 退出应用程序
    QCoreApplication::quit();
}

// 触发开始计时信号的槽函数
void petwindow::petStart()
{
    // 发射开始计时信号
    emit emitStart();
}

// 触发暂停/继续计时信号的槽函数
void petwindow::petPause()
{
    // 发射暂停/继续计时信号
    emit emitPause();
}

// 触发分神次数增加信号的槽函数
void petwindow::petCounter()
{
    // 发射分神次数增加信号
    emit emitCounter();
}

// 触发结束计时信号的槽函数
void petwindow::petEnd()
{
    // 发射结束计时信号
    emit emitEnd();
}

// 更新 QLCDNumber 显示时间的槽函数
void petwindow::changedLcd(QString newtime)
{
    // 在 QLCDNumber 上显示新的时间
    time->display(newtime);
}

// 鼠标移动事件处理函数
void petwindow::mouseMoveEvent(QMouseEvent *event)
{
    // 获取鼠标事件的全局位置
    QPoint eventPos = event ->globalPosition().toPoint();
    // 判断窗口是否未处于最大化或全屏状态，并且宠物处于移动状态，鼠标左键被按下，且移动距离超过拖动阈值
    if (!(windowState() & Qt::WindowMaximized)&&!(windowState() & Qt::WindowFullScreen)&&petMoving&&(event->buttons()&Qt::LeftButton)
        &&(eventPos-petLastPose).manhattanLength()>QApplication::startDragDistance())
    {
        // 移动窗口到新的位置
        move(eventPos - petLastPose);
        // 更新宠物的最后位置
        petLastPose = eventPos - this->pos();
    }
    // 调用父类的鼠标移动事件处理函数
    return QWidget::mouseMoveEvent(event);
}

// 鼠标按下事件处理函数
void petwindow::mousePressEvent(QMouseEvent *event)
{
    // 判断是否按下鼠标左键
    if (event->button()==Qt::LeftButton)
    {
        // 设置宠物处于移动状态
        petMoving=true;
        // 记录宠物的最后位置
        petLastPose=event->globalPosition().toPoint()-this->pos();
    }
    // 调用父类的鼠标按下事件处理函数
    return QWidget::mousePressEvent(event);
}

// 鼠标释放事件处理函数
void petwindow::mouseReleaseEvent(QMouseEvent *event)
{
    // 设置宠物停止移动
    petMoving =false;
    // 接受鼠标释放事件
    event->accept();
}

// 右键菜单事件处理函数
void petwindow::contextMenuEvent(QContextMenuEvent *event)
{
    // 在鼠标右键点击的位置显示右键菜单
    contextMenu->exec(event->globalPos());
}

// 绘制事件处理函数
void petwindow::paintEvent(QPaintEvent *event)
{
    // 忽略事件参数
    Q_UNUSED(event);
    // 创建一个 QPainter 对象用于绘制
    QPainter painter(this);
    // 加载宠物图片
    QPixmap pic(":/pic/res/pic/deskpet.png");
    // 在窗口上绘制宠物图片
    painter.drawPixmap(0, 0, width(), height(), pic);
    // 调用父类的绘制事件处理函数
    QWidget::paintEvent(event);
}

// 窗口大小改变事件处理函数
void petwindow::resizeEvent(QResizeEvent *event)
{
    // 调用父类的窗口大小改变事件处理函数
    QWidget::resizeEvent(event);
    // 更新窗口内容
    update();
}
