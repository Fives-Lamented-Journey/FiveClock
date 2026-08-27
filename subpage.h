#ifndef SUBPAGE_H
#define SUBPAGE_H

#include <QWidget>
// #include "sqldbm.h"
#include <qsqlquery.h>
#include "card.h"
#include <QVBoxLayout>
#include "customcalendarwidget.h"
#include <QPainter>
#include "infodialog.h"
#include <qscrollbar.h>

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
    // 显示卡片记录的函数
    void showCard();
    // 选中的当前日期
    QString selectedToday = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    // 主页按钮
    mybutton *homeButton = new mybutton(":/icon/res/icon/home_init.png",
                                        ":/icon/res/icon/home_hover.png",
                                        ":/icon/res/icon/home_pressed.png");
    // 子页面按钮
    mybutton *subButton = new mybutton(":/icon/res/icon/card_pressed.png",
                                       ":/icon/res/icon/card_hover.png",
                                       ":/icon/res/icon/card_pressed.png");
    // 图表按钮
    mybutton *chartButton = new mybutton(":/icon/res/icon/chartButton_ini.png",
                                         ":/icon/res/icon/chartButton_hover.png",
                                         ":/icon/res/icon/chartButton_pressed.png");
    // 添加记录按钮
    mybutton *subAddRecordButton = new mybutton(":/icon/res/icon/add_ini_button.png",
                                                ":/icon/res/icon/add_hover_button.png",
                                                ":/icon/res/icon/add_pressed_button.png");
    // 自定义日历部件
    CustomCalendarWidget *calendar;

private:
    // 卡片部件
    card* subCard;

    // 滚动布局
    QVBoxLayout* subScrollVbox;
    // 水平布局
    QVBoxLayout* subHbox;
    // 子部件
    QWidget*  subWidget;
    // 信息对话框
    infoDialog* subDialog;

    // 日期时间对象
    QDateTime subDateTime;

signals:
    // 页面切换到子页面的信号
    void pageChangeToSub();
    // 页面切换到主页的信号
    void pageChangeToHome();
    // 排序记录的信号
    void sortedRecord(QSqlQuery& query, QString oneDay);
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
