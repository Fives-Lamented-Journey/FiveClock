#include "mybutton.h"

// 自定义按钮类的构造函数
mybutton::mybutton(const QString &normalIconPath, const QString &hoverIconPath,
                   const QString &pressedIconPath, QWidget *parent)
    : QPushButton(parent) {

    // 加载不同状态下的图标到对应的像素图对象中
    normalPixmap.load(normalIconPath);
    hoverPixmap.load(hoverIconPath);
    pressedPixmap.load(pressedIconPath);

    // 设置初始图标为正常状态图标
    setIcon(QIcon(normalPixmap));
    // 设置图标大小为 32x32 像素
    setIconSize(QSize(32, 32));
    // 设置按钮的样式表，使其背景透明，无边框，无内边距和外边距
    this->setStyleSheet("background: transparent;border:none;padding: 0; margin: 0;");

    // 连接按钮的按下信号到 onPressed 槽函数
    connect(this, &QPushButton::pressed, this, &mybutton::onPressed);
    // 连接按钮的释放信号到 onReleased 槽函数
    connect(this, &QPushButton::released, this, &mybutton::onReleased);
}

// 按钮按下时的槽函数
void mybutton::onPressed() {
    // 按钮按下时，将按钮的图标设置为按下状态的图标
    setIcon(QIcon(pressedPixmap));
}

// 按钮释放时的槽函数
void mybutton::onReleased() {
    // 按钮释放时，如果鼠标还在按钮上，则显示悬停图标，否则显示正常图标
    if (underMouse()) {
        setIcon(QIcon(hoverPixmap));
    } else {
        setIcon(QIcon(normalPixmap));
    }
}

// 鼠标进入按钮区域时的事件处理函数
void mybutton::enterEvent(QEnterEvent *event)
{
    // 鼠标进入时，将按钮的图标设置为悬停状态的图标
    setIcon(QIcon(hoverPixmap));
    // 调用父类的 enterEvent 函数，确保父类的事件处理逻辑正常执行
    QPushButton::enterEvent(event);
}

// 鼠标离开按钮区域时的事件处理函数
void mybutton::leaveEvent(QEvent *event)
{
    // 鼠标离开时，将按钮的图标设置为正常状态的图标
    setIcon(QIcon(normalPixmap));
    // 调用父类的 leaveEvent 函数，确保父类的事件处理逻辑正常执行
    QPushButton::leaveEvent(event);
}
