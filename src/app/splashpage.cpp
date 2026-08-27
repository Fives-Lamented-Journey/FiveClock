#include "app/splashpage.h"
#include "ui_splashpage.h"
#include <QTimer>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QFontDatabase>
#include <QPainter>

// 构造函数的实现
splashPage::splashPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::splashPage)
{
    // 设置界面
    ui->setupUi(this);
    // 设置窗口为闪屏窗口标志
    this->setWindowFlag(Qt::SplashScreen);
    // 设置窗口为全屏状态
    this->setWindowState(Qt::WindowFullScreen);
    // 设置窗口关闭时自动删除
    this->setAttribute(Qt::WA_DeleteOnClose);

    // 添加自定义字体到字体数据库
    int fontId = QFontDatabase::addApplicationFont(":/font/res/font/ZhouZiFang.ttf");
    // 获取字体家族名称
    QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
    // 创建自定义字体对象，字体大小为 20
    QFont customFont("ZhouZiFang", 20);
    // 设置字体颜色样式
    QString style=QString("color:rgba(0,0,0,222)");

    // 在标签上显示随机生成的句子
    ui->label->setText(generateSentence());
    // 设置标签的字体为自定义字体
    ui->label->setFont(customFont);
    // 设置标签的样式
    ui->label->setStyleSheet(style);

    // 创建定时器对象
    splaTimer=new QTimer(this);
    // 停止定时器
    splaTimer->stop();
    // 设置定时器类型为粗略定时器
    splaTimer->setTimerType(Qt::CoarseTimer);
    // 设置定时器间隔为 5000 毫秒（即 5 秒）
    splaTimer->setInterval(5000);
    // 设置定时器为单次触发
    splaTimer->setSingleShot(true);
    // 连接定时器超时信号到 timerOut 槽函数
    connect(splaTimer,&QTimer::timeout,this,&splashPage::timerOut);

    // 创建淡入淡出动画对象，作用于窗口透明度属性
    fadeOutAnimation = new QPropertyAnimation(this, "windowOpacity");
    // 设置动画持续时间为 500 毫秒
    fadeOutAnimation->setDuration(500);
    // 设置动画起始值为 1.0（完全不透明）
    fadeOutAnimation->setStartValue(1.0);
    // 设置动画结束值为 0.0（完全透明）
    fadeOutAnimation->setEndValue(0.0);
    // 连接动画结束信号到窗口关闭函数
    connect(fadeOutAnimation, &QPropertyAnimation::finished, this, &QWidget::close);
}

// 析构函数的实现，释放 ui 对象的内存
splashPage::~splashPage()
{
    delete ui;
}

// 显示闪屏页的函数实现
void splashPage::splashShow()
{
    // 显示窗口
    this->show();
    // 启动定时器
    splaTimer->start();
}

// 生成随机句子的函数实现
QString splashPage::generateSentence()
{
    // 从 sentence 列表中随机选择一个句子并返回
    return sentence.at(QRandomGenerator::global()->bounded(sentence.size()));
}

// 生成随机背景图片路径的函数实现
QString splashPage::generateImage()
{
    // 从 backImage 列表中随机选择一个图片路径并返回
    return backImage.at(QRandomGenerator::global()->bounded(backImage.size()));
}

// 定时器超时的槽函数实现
void splashPage::timerOut()
{
    // 发出即将关闭的信号
    emit aboutToClose();
    // 启动淡入淡出动画
    fadeOutAnimation->start();
}

// 重写绘制事件处理函数
void splashPage::paintEvent(QPaintEvent *event)
{
    // 忽略 event 参数
    Q_UNUSED(event);
    // 创建 QPainter 对象用于绘制
    QPainter painter(this);
    // 加载随机生成的背景图片
    QPixmap pic(generateImage());
    // 绘制背景图片，铺满整个窗口
    painter.drawPixmap(0, 0, width(), height(), pic);
    // 调用父类的绘制事件处理函数
    QWidget::paintEvent(event);
}

// 重写窗口大小改变事件处理函数
void splashPage::resizeEvent(QResizeEvent *event)
{
    // 调用父类的窗口大小改变事件处理函数
    QWidget::resizeEvent(event);
    // 更新界面
    update();
}
