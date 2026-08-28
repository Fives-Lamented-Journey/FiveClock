#ifndef PETWINDOW_H
#define PETWINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
// 以下 Qt 类型在头文件中仅以指针/引用形式使用，使用前向声明降低编译依赖
class QLabel;
class QMenu;
class QLCDNumber;
QT_END_NAMESPACE

// 项目内类型仅以指针形式使用
class ConfigService;

// 命名空间，包含用户界面类
namespace Ui {
class petwindow;
}

// 宠物窗口类，继承自 QWidget
class petwindow : public QWidget
{
    // 声明该类使用 Qt 的信号和槽机制
    Q_OBJECT

public:
    // 构造函数，接收一个父窗口指针作为参数，默认为 nullptr
    explicit petwindow(QWidget *parent = nullptr);
    // 析构函数
    ~petwindow();
    // 创建右键菜单的函数
    void createContextMenu();
    // 注入配置服务
    void setConfigService(ConfigService *service);

public slots:
    // 更新 QLCDNumber 显示时间的槽函数
    void changedLcd(QString newtime);

private slots:
    // 隐藏宠物窗口的槽函数
    void hidePet();
    // 退出应用程序的槽函数
    void quitapp();

signals:
    // 开始计时信号
    void emitStart();
    // 暂停/继续计时信号
    void emitPause();
    // 分神次数增加信号
    void emitCounter();
    // 结束计时信号
    void emitEnd();

private:
    // 用户界面指针
    Ui::petwindow *ui;

    // 宠物标签指针
    QLabel* petLabel;
    // 拖动位置
    QPoint dragPosition;
    // 右键菜单指针
    QMenu* contextMenu;
    // 用于显示时间的 QLCDNumber 指针
    QLCDNumber* time;

    // 宠物是否正在移动的标志
    bool petMoving;
    // 宠物的最后位置
    QPoint petLastPose;

    // 注入的配置服务指针
    ConfigService* m_configService = nullptr;

protected:
    // 鼠标移动事件处理函数
    void mouseMoveEvent(QMouseEvent *event) override;
    // 鼠标按下事件处理函数
    void mousePressEvent(QMouseEvent *event) override;
    // 鼠标释放事件处理函数
    void mouseReleaseEvent(QMouseEvent *event) override;
    // 右键菜单事件处理函数
    void contextMenuEvent(QContextMenuEvent *event) override;
    // 绘制事件处理函数
    void paintEvent(QPaintEvent *event) override;
    // 窗口大小改变事件处理函数
    void resizeEvent(QResizeEvent *event) override;
};

#endif // PETWINDOW_H
