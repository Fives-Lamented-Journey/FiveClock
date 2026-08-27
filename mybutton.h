#ifndef MYBUTTON_H
#define MYBUTTON_H

#include <QObject>
#include <QPushButton>
#include <qwidget.h>

// 自定义按钮类，继承自 QPushButton
class mybutton : public QPushButton
{
    Q_OBJECT
public:
    // 构造函数，接收三个图标的路径（正常状态、悬停状态、按下状态）和父窗口指针
    // normalIconPath: 正常状态下按钮图标的文件路径
    // hoverIconPath: 鼠标悬停时按钮图标的文件路径
    // pressedIconPath: 按钮被按下时图标的文件路径
    // parent: 父窗口指针，默认为 nullptr
    mybutton(
        const QString &normalIconPath,
        const QString &hoverIconPath,
        const QString &pressedIconPath,
        QWidget *parent = nullptr);

private:
    // 存储不同状态下的图标像素图
    QPixmap normalPixmap; // 正常状态下的图标像素图
    QPixmap hoverPixmap;  // 鼠标悬停时的图标像素图
    QPixmap pressedPixmap; // 按钮被按下时的图标像素图

private slots:
    // 按钮按下时触发的槽函数
    void onPressed();
    // 按钮释放时触发的槽函数
    void onReleased();

    // QWidget 接口重写的函数
protected:
    // 鼠标进入按钮区域时触发的事件处理函数
    void enterEvent(QEnterEvent *event) override;
    // 鼠标离开按钮区域时触发的事件处理函数
    void leaveEvent(QEvent *event) override;
};

#endif // MYBUTTON_H
