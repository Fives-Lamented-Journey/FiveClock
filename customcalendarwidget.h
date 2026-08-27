#ifndef CUSTOMCALENDARWIDGET_H
#define CUSTOMCALENDARWIDGET_H

#include <QWidget>
#include <QDate>   // currentDate 为值成员，需要完整类型

QT_BEGIN_NAMESPACE
// 以下 Qt 类型在头文件中仅以指针/引用形式使用，使用前向声明降低编译依赖
class QGridLayout;
class QLabel;
class QPushButton;
QT_END_NAMESPACE

// 自定义日历小部件类，继承自QWidget
// 该类用于创建一个自定义的日历界面，包含月份切换按钮和日期显示
class CustomCalendarWidget : public QWidget
{
    // 声明这是一个包含信号和槽机制的Qt类
    // 信号和槽机制是Qt用于对象间通信的核心机制
    Q_OBJECT

public:
    // 构造函数，接收一个可选的父窗口指针
    // 当创建CustomCalendarWidget对象时，如果提供了父窗口指针，该对象将作为子窗口显示在父窗口中
    explicit CustomCalendarWidget(QWidget* parent = nullptr);

    // 析构函数，用于释放资源
    // 当CustomCalendarWidget对象被销毁时，会调用该函数来释放动态分配的内存
    ~CustomCalendarWidget();

    // 设置网格布局中每行的拉伸因子
    void setStrench();

    // 更新日历显示的函数
    // 根据当前显示的日期更新日历界面，包括月份显示和日期按钮的创建
    void updateCalendar();

signals:
    // 信号：当用户选择一个日期时发出，携带所选日期信息
    // 其他对象可以连接到这个信号，以便在用户选择日期时做出相应的处理
    void dateSelected(const QDate& date);

    // 信号：用于设置日期按钮的样式
    // 传递日期按钮指针和对应的日期，以便根据日期设置不同的样式
    void setButtonStyle(QPushButton* & dayButotn,QDate day);

    // 私有槽函数，用于处理按钮点击事件
private slots:
    // 处理上一个月按钮点击事件
    // 当用户点击上一个月按钮时，调用该函数将当前显示的日期减去一个月并更新日历
    void prevMonth();

    // 处理下一个月按钮点击事件
    // 当用户点击下一个月按钮时，调用该函数将当前显示的日期加上一个月并更新日历
    void nextMonth();

    // 处理日期按钮点击事件
    // 当用户点击某个日期按钮时，调用该函数发出dateSelected信号并打印所选日期
    void dayClicked(int day);

private:
    // 成员变量
    // 用于布局小部件的网格布局
    // 网格布局可以将小部件按照行和列的方式排列
    QGridLayout* gridLayout;

    // 显示当前月份和年份的标签
    // 该标签会显示当前日历所显示的月份和年份
    QLabel* monthYearLabel;

    // 上一个月按钮
    // 点击该按钮可以切换到上一个月的日历显示
    QPushButton* prevButton;

    // 下一个月按钮
    // 点击该按钮可以切换到下一个月的日历显示
    QPushButton* nextButton;

    // 当前显示的日期
    // 记录当前日历所显示的月份和年份
    QDate currentDate;
};

#endif // CUSTOMCALENDARWIDGET_H
