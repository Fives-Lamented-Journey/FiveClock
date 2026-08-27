#ifndef SPLASHPAGE_H
#define SPLASHPAGE_H

#include <QWidget>
#include <QStringList> // sentence/backImage 为值成员且带 NSDMI 初始化，需要完整类型

QT_BEGIN_NAMESPACE
// 以下 Qt 类型在头文件中仅以指针形式使用，使用前向声明降低编译依赖
class QTimer;
class QPropertyAnimation;
QT_END_NAMESPACE

// 命名空间 Ui，用于包含由 Qt Designer 生成的界面类
namespace Ui {
class splashPage;
}

// 定义 splashPage 类，继承自 QWidget
class splashPage : public QWidget
{
    Q_OBJECT

public:
    // 构造函数，explicit 关键字防止隐式转换，parent 为父窗口指针，默认为 nullptr
    explicit splashPage(QWidget *parent = nullptr);
    // 析构函数
    ~splashPage();

    // 显示闪屏页的函数
    void splashShow();

    // 生成随机句子的函数
    QString generateSentence();
    // 生成随机背景图片路径的函数
    QString generateImage();

signals:
    // 当闪屏页即将关闭时发出的信号
    void aboutToClose();

private slots:
    // 定时器超时的槽函数
    void timerOut();

private:
    // 指向 Ui::splashPage 类的指针，用于访问界面元素
    Ui::splashPage *ui;

    // 存储随机句子的字符串列表
    QStringList sentence={"你太习惯这个世界了，才会对任何事情都不感到惊奇",
                            "没有真相，只有视角",
                            "熟知并非真知",
                            "有种死亡叫边做边怀疑",
                            "你太年轻，太理想主义了。你以为每个问题都能找到答案",
                            "我愚笨。得说很多话，做很多事，犯很多错才能进步一点点",
                            " 见三岁孩童抱金砖于闹市，世人皆魔鬼；\n 遇笑脸弥勒旁立护法韦陀，群魔皆圣贤。",
                           "哪里会有人喜欢孤独,不过是不喜欢失望罢了"};
    // 存储背景图片路径的字符串列表
    QStringList backImage{":/pic/res/pic/bgi05.png",
                          ":/pic/res/pic/bgi14.jpg"
                          };
    // 定时器指针，用于控制闪屏页的显示时间
    QTimer * splaTimer;
    // 淡入淡出动画指针，用于实现闪屏页的淡入淡出效果
    QPropertyAnimation *fadeOutAnimation;

protected:
    // 重写绘制事件处理函数，用于绘制背景图片
    void paintEvent(QPaintEvent *event) override;
    // 重写窗口大小改变事件处理函数，用于更新界面
    void resizeEvent(QResizeEvent *event) override;
};

#endif // SPLASHPAGE_H
