#ifndef CHARTPAGE_H
#define CHARTPAGE_H

#include <QWidget>
#include <QtCharts>
#include <qmap.h>
#include <qsqlquery.h>
#include "mybutton.h"

// 命名空间，包含 UI 类的定义
namespace Ui {
class chartPage;
}

// chartPage 类，继承自 QWidget
class chartPage : public QWidget
{
    Q_OBJECT

public:
    // 构造函数，显式声明，可防止隐式类型转换
    explicit chartPage(QWidget *parent = nullptr);
    // 析构函数
    ~chartPage();
    // 消费时间统计的图表对象指针
    QChart *consumChart;
    // 分神次数统计的图表对象指针
    QChart *counterChart;

    // 绘制所有图表的函数
    void drawChart();
    // 清除指定图表内容的函数
    void clearChart(QChart* chart);
    // 绘制饼状图的函数
    void drawPieChart();
    // 绘制柱状图的函数，默认索引为 0
    void drawColChart(int index = 0);

    // 生成绘制饼状图所需数据的函数
    void generateData();
    // 生成绘制柱状图所需周数据的函数，默认索引为 0
    void generateWeekData(int index = 0);
    // 初始化周选择下拉框的函数
    void iniWeekCombox();

    // 获取选中周的日期列表的函数，默认索引为 0
    void getSelectedWeek(int index = 0);
    // 计算总消费时间的函数
    int getSumConsuTime();

    // 主页按钮对象指针
    mybutton *homeButton = new mybutton(":/icon/res/icon/home_init.png",
                                        ":/icon/res/icon/home_hover.png",
                                        ":/icon/res/icon/home_pressed.png");
    // 卡片按钮对象指针
    mybutton *subButton = new mybutton(":/icon/res/icon/card_ini.png",
                                       ":/icon/res/icon/card_hover.png",
                                       ":/icon/res/icon/card_pressed.png");
    // 图表按钮对象指针
    mybutton *chartButton = new mybutton(":/icon/res/icon/chartButton_pressed.png",
                                         ":/icon/res/icon/chartButton_hover.png",
                                         ":/icon/res/icon/chartButton_pressed.png");

signals:
    // 发射获取指定日期数据的信号
    void getData(QSqlQuery &query, QString selectedDay);
    // 发射获取选中周数据的信号
    void getWeekData(QSqlQuery &query, QStringList selectedWeek);

private slots:
    // 日期编辑框日期改变时的槽函数
    void on_selectedDateEdit_userDateChanged(const QDate &date);
    // 周选择下拉框选中项改变时的槽函数
    void on_weekComboBox_currentIndexChanged(int index);

private:
    // UI 对象指针
    Ui::chartPage *ui;

    // 存储绘制饼状图所需的数据
    QMap<QString, int> data;
    // 存储绘制柱状图所需的周数据
    QMap<QString, int> weekData;

    // 选中周的日期列表
    QStringList selectedWeek;
    // 选中的日期
    QDate selectedDay;

protected:
    // 重写绘制事件处理函数
    void paintEvent(QPaintEvent *event) override;
    // 重写大小改变事件处理函数
    void resizeEvent(QResizeEvent *event) override;
};

#endif // CHARTPAGE_H
