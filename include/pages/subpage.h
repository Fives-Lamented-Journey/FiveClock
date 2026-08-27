#ifndef SUBPAGE_H
#define SUBPAGE_H

#include <QWidget>
#include <QDateTime>   // selectedToday 的 NSDMI 中调用了 QDateTime 静态方法

QT_BEGIN_NAMESPACE
// 以下 Qt 类型在头文件中仅以指针/引用形式使用，使用前向声明降低编译依赖
class QVBoxLayout;
class QSqlQuery;
class QDate;
QT_END_NAMESPACE

// 项目内类型仅以指针形式使用，使用前向声明替代 include
class card;
class CustomCalendarWidget;
class mybutton;
class sqlDBM;

namespace Ui {
class subPage;
}

// 子页面类，继承自QWidget
class subPage : public QWidget
{
    Q_OBJECT

public:
    // 显式构造函数
    explicit subPage(QWidget *parent = nullptr);
    // 析构函数
    ~subPage();

public:
    // 注入数据管理对象（页面通过它直接获取记录，不再依赖父窗口的信号回填）
    void setDataBase(sqlDBM *dbm);
    // 显示卡片记录的函数
    void showCard();
    // 选中的当前日期
    QString selectedToday = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    // 主页按钮
    mybutton *homeButton = nullptr;
    // 子页面按钮
    mybutton *subButton = nullptr;
    // 图表按钮
    mybutton *chartButton = nullptr;
    // 添加记录按钮
    mybutton *subAddRecordButton = nullptr;
    // 自定义日历部件
    CustomCalendarWidget *calendar;

private:
    // 卡片部件
    card* subCard;

    // 注入的数据管理对象指针（未注入时 showCard 展示空记录）
    sqlDBM* m_dbm = nullptr;

    // 滚动布局
    QVBoxLayout* subScrollVbox;
    // 水平布局
    QVBoxLayout* subHbox;
    // 子部件
    QWidget*  subWidget;

signals:
    // 页面切换到子页面的信号
    void pageChangeToSub();
    // 页面切换到主页的信号
    void pageChangeToHome();
    // 更新记录的信号
    void updateRecord(card* oneCard);
    // 删除记录的信号
    void deleteRecord(card* oneCard);
    // 窗口大小改变的信号
    void resized();

private slots:
    // 当日期选择改变时更新卡片列表的槽函数
    void subUpdateCardList(const QDate &date);
    // 更新卡片信息的槽函数
    void subUpdateCard(card* oneCard);
    // 删除卡片信息的槽函数
    void subDelCard(card* oneCard);

private:
    // UI对象指针
    Ui::subPage *ui;

protected:
    // 绘制事件处理函数
    void paintEvent(QPaintEvent *event) override;
    // 窗口大小改变事件处理函数
    void resizeEvent(QResizeEvent *event) override;
    // 显示事件处理函数
    void showEvent(QShowEvent *event) override;

private slots:
    // 更新添加按钮位置的槽函数
    void updateAddButtonPosition();
};

#endif // SUBPAGE_H
