#ifndef CHARTPAGE_H
#define CHARTPAGE_H

#include <QWidget>
#include <QMap>        // data/weekData 为值成员，需要完整类型
#include <QDate>       // selectedDay 为值成员，需要完整类型
#include <QStringList> // selectedWeek 为值成员，需要完整类型

QT_BEGIN_NAMESPACE
// QChart 在头文件中仅以指针形式使用；完整的 QtCharts 定义在 chartpage.cpp 中包含
class QChart;
QT_END_NAMESPACE

// 项目内类型仅以指针形式使用
class mybutton;
class RecordService;

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
    // 注入记录业务服务：页面通过它获取统计数据、订阅 dataChanged 信号（不再依赖 sqlDBM）
    void setRecordService(RecordService *service);
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
    mybutton *homeButton = nullptr;
    // 卡片按钮对象指针
    mybutton *subButton = nullptr;
    // 图表按钮对象指针
    mybutton *chartButton = nullptr;

private slots:
    // 日期编辑框日期改变时的槽函数
    void on_selectedDateEdit_userDateChanged(const QDate &date);
    // 周选择下拉框选中项改变时的槽函数
    void on_weekComboBox_currentIndexChanged(int index);
    // 数据变更通知的响应槽：重新绘制全部图表（由 RecordService::dataChanged 驱动）
    void refreshCharts();

private:
    // 注入的记录业务服务指针（未注入时取数为空，与旧版信号未被连接时的行为一致）
    RecordService* m_recordService = nullptr;

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
