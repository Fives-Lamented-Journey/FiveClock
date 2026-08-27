#include "chartpage.h"
#include "ui_chartpage.h"

// 构造函数，初始化 chartPage 类的对象
chartPage::chartPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::chartPage)
{
    // 设置 UI
    ui->setupUi(this);
    // 创建消费时间统计的图表对象
    consumChart = new QChart();
    // 创建分神次数统计的图表对象
    counterChart = new QChart();

    // 设置图表背景不可见
    counterChart->setBackgroundVisible(false);
    consumChart->setBackgroundVisible(false);

    // 初始化选中的日期为当前日期
    selectedDay = QDate::currentDate();
    // 设置日期编辑框的日期为当前日期
    ui->selectedDateEdit->setDate(QDate::currentDate());
    // 初始化周选择下拉框
    iniWeekCombox();

    // 设置柱状图的边距为 0
    QMargins colMargins(0, 0, 0, 0);
    // 设置饼状图的边距为 0
    QMargins pieMargins(0, 0, 0, 0);
    consumChart->setMargins(pieMargins);
    counterChart->setMargins(colMargins);

    // 创建垂直布局
    QVBoxLayout *vbox = new QVBoxLayout();
    // 添加伸缩空间
    vbox->addStretch(1);
    // 添加主页按钮到布局
    vbox->addWidget(homeButton);
    vbox->addStretch(1);
    // 添加卡片按钮到布局
    vbox->addWidget(subButton);
    vbox->addStretch(1);
    // 添加图表按钮到布局
    vbox->addWidget(chartButton);
    vbox->addStretch(1);
    // 设置按钮框架的布局
    ui->buttonFrame->setLayout(vbox);
}

// 析构函数，释放 UI 对象的内存
chartPage::~chartPage()
{
    delete ui;
}

// 绘制所有图表的函数，调用绘制柱状图和饼状图的函数
void chartPage::drawChart()
{
    drawColChart();
    drawPieChart();
}

// 清除指定图表的内容，包括系列、轴和重置属性
void chartPage::clearChart(QChart *chart)
{
    // 1. 移除所有系列
    chart->removeAllSeries();

    // 2. 移除所有轴
    for (auto axis : chart->axes()) {
        chart->removeAxis(axis);
    }

    // 3. 重置图表属性
    chart->setTitle("");
    chart->legend()->hide();
    chart->setMargins(QMargins());
}

// 绘制饼状图的函数
void chartPage::drawPieChart()
{
    // 生成绘制饼状图所需的数据
    generateData();
    qDebug() << "Entering drawPieChart()";
    // 清除消费时间统计图表的内容
    clearChart(consumChart);

    // 如果数据为空，显示无记录的背景图片
    if (data.isEmpty()) {
        ui->consumGraphicsView->setStyleSheet("background-image:url(:/pic/res/pic/noRecord-background.png);"
                                              "background-repeat:no-repeat;"
                                              "background-position: center;"
                                              "background-attachment: fixed;"
                                              );
        ui->consumGraphicsView->setChart(consumChart);
        return;
    }
    else
    {
        // 清除样式表
        ui->consumGraphicsView->setStyleSheet("");
    }

    // 创建饼状图系列
    QPieSeries *series = new QPieSeries();
    // 设置标签可见
    series->setLabelsVisible(true);
    // 设置饼状图的大小
    series->setPieSize(0.5);
    // 遍历数据，将数据添加到饼状图系列中
    foreach (const QString &eventClass, data.keys()) {
        series->append(eventClass, data[eventClass]);
    }

    // 如果系列中没有切片，说明无数据可绘制，释放系列对象并返回
    if (series->slices().isEmpty()) {
        qDebug() << "无数据可绘制饼图";
        delete series;  // 避免内存泄漏
        return;
    }

    // 输出切片的数量
    qDebug() << "Number of slices: " << series->slices().size();
    // 定义颜色设置列表
    QList<QBrush> colorSetter;
    QBrush brushFir(QColor(240, 255, 255));//Azure
    QBrush brushSec(QColor(255, 218, 185));//PeachPuff
    QBrush brushThi(QColor(193, 255, 193));//DarkSeaGreen1
    QBrush brushFou(QColor(230, 230, 250));//lavender
    QBrush brushFiv(QColor(255, 228, 225));//MistyRose
    colorSetter << brushFir << brushSec << brushThi << brushFou << brushFou << brushFiv;
    // 遍历系列中的切片，设置标签、颜色等属性
    for (int i = 0; i < series->count(); i++) {
        QPieSlice *slice = series->slices().at(i);
        slice->setLabel(slice->label() + QString::asprintf(":%.0fmin,%.1f%%", slice->value(), slice->percentage() * 100));
        if (i == series->count() - 1) slice->setExploded(true);
        slice->setLabelVisible(true);
        slice->setPen(QPen(Qt::transparent));
        slice->setBrush(colorSetter.at(i));
        slice->setLabelBrush(colorSetter.at(i));
    }

    // 将系列添加到消费时间统计图表中
    consumChart->addSeries(series);
    // 计算总消费时间
    int sum = getSumConsuTime();
    // 设置图表标题
    consumChart->setTitle(QString("记录总时长:%1h %2min").arg(sum / 60).arg(sum % 60));

    // 设置图例可见并靠下方显示
    consumChart->legend()->setVisible(true);
    consumChart->legend()->setAlignment(Qt::AlignBottom);
    // 将消费时间统计图表设置到图形视图中
    ui->consumGraphicsView->setChart(consumChart);

    // 更新图形视图
    ui->consumGraphicsView->update();
    qDebug() << "Exiting drawPieChart()";
}

// 绘制柱状图的函数，默认索引为 0
void chartPage::drawColChart(int index)
{
    qDebug() << "执行drawCloChart函数";
    // 生成绘制柱状图所需的周数据
    generateWeekData(index);
    // 清除分神次数统计图表的内容
    clearChart(counterChart);

    // 如果周数据为空，显示无周记录的背景图片
    if (weekData.isEmpty()) {
        qDebug() << "weekData的记录为空";
        ui->counterGraphicsView->setStyleSheet("background-image:url(:/pic/res/pic/noWeekRecord.png);"
                                               "background-repeat:no-repeat;"
                                               "background-position: center;"
                                               "background-attachment: fixed;"
                                               );
        ui->counterGraphicsView->setChart(counterChart);
        return;
    }
    else
    {
        // 清除样式表
        ui->counterGraphicsView->setStyleSheet("");
    }

    // 创建柱状图集合
    QBarSet *set = new QBarSet("分神次数");
    // 遍历周数据，将数据添加到柱状图集合中
    foreach (const QString &day, weekData.keys()) {
        set->append(weekData[day]);
    }

    // 创建柱状图系列
    QBarSeries *series = new QBarSeries();
    // 设置标签可见
    series->setLabelsVisible(true);
    // 设置标签格式
    series->setLabelsFormat("@value次");

    // 将柱状图集合添加到系列中
    series->append(set);
    // 将系列添加到分神次数统计图表中
    counterChart->addSeries(series);
    // 设置图表标题
    counterChart->setTitle("分神次数统计");

    // 创建 X 轴
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    // 将选中的周添加到 X 轴
    axisX->append(selectedWeek);
    // 设置网格线不可见
    axisX->setGridLineVisible(false);
    // 将 X 轴添加到图表中，并设置对齐方式为底部
    counterChart->addAxis(axisX, Qt::AlignBottom);
    // 将系列关联到 X 轴
    series->attachAxis(axisX);

    // 创建 Y 轴
    QValueAxis *axisY = new QValueAxis();
    // 设置网格线不可见
    axisY->setGridLineVisible(false);
    // 设置 Y 轴的范围
    axisY->setRange(0, 150);
    // 设置 Y 轴的标题
    axisY->setTitleText("分神次数");
    // 将 Y 轴添加到图表中，并设置对齐方式为左侧
    counterChart->addAxis(axisY, Qt::AlignLeft);
    // 将系列关联到 Y 轴
    series->attachAxis(axisY);

    // 设置图例可见并靠下方显示
    counterChart->legend()->setVisible(true);
    counterChart->legend()->setAlignment(Qt::AlignBottom);
    // 将分神次数统计图表设置到图形视图中
    ui->counterGraphicsView->setChart(counterChart);

    // 更新消费时间统计图形视图
    ui->consumGraphicsView->update();
}

// 生成绘制饼状图所需的数据
void chartPage::generateData()
{
    qDebug() << "Entering generateData";
    // 清空数据
    data.clear();

    // 创建 SQL 查询对象
    QSqlQuery query;
    // 发射信号，获取指定日期的数据
    emit getData(query, selectedDay.toString("yyyy-MM-dd"));

    // 遍历查询结果，将数据插入到 data 中
    while (query.next()) {
        data.insert(query.value(0).toString(), query.value(1).toInt());
    }
    qDebug() << "Exiting generateData";
}

// 生成绘制柱状图所需的周数据
void chartPage::generateWeekData(int index)
{
    // 清空周数据
    weekData.clear();
    // 获取选中的周
    getSelectedWeek(index);

    // 创建 SQL 查询对象
    QSqlQuery query;
    // 发射信号，获取选中周的数据
    emit getWeekData(query, selectedWeek);
    // 如果查询结果为空，输出提示信息并返回
    if (!query.next()) {
        qDebug() << "generateWeekData的query的记录为空";
        return;
    }
    // 将查询结果指针移动到第一条记录之前
    query.seek(-1);
    // 遍历查询结果，将数据插入到 weekData 中
    while (query.next()) {
        weekData.insert(query.value(0).toString(), query.value(1).toInt());
    }
}

// 初始化周选择下拉框
void chartPage::iniWeekCombox()
{
    // 获取当前日期
    QDate currentDate = QDate::currentDate();
    // 循环添加 4 周的日期范围到下拉框中
    for (int i = 1; i <= 4; i++) {
        QDate startDate = currentDate.addDays(i * (-7));
        QDate endDate = currentDate.addDays((i - 1) * (-7) - 1);
        ui->weekComboBox->addItem(startDate.toString("yyyy.MM.dd") + " -- " + endDate.toString("yyyy.MM.dd"));
    }
}

// 获取选中的周的日期列表
void chartPage::getSelectedWeek(int index)
{
    // 清空选中的周的日期列表
    selectedWeek.clear();
    // 获取当前日期
    QDate currentDate = QDate::currentDate();

    // 计算一周前的日期
    QDate oneWeekAgo = currentDate.addDays((index + 1) * (-7));

    // 循环添加过去一周的日期到 selectedWeek 中
    for (int i = 0; i < 7; ++i) {
        QDate date = oneWeekAgo.addDays(i);
        selectedWeek << date.toString("yyyy-MM-dd");
    }
}

// 计算总消费时间
int chartPage::getSumConsuTime()
{
    // 定义迭代器
    QMap<QString, int>::ConstIterator i;
    int sum = 0;
    // 遍历 data 中的数据，累加消费时间
    for (i = data.constBegin(); i != data.constEnd(); i++) {
        sum += i.value();
    }
    return sum;
}

// 当日期编辑框中的日期发生变化时，更新选中的日期并重新绘制饼状图
void chartPage::on_selectedDateEdit_userDateChanged(const QDate &date)
{
    selectedDay = date;
    drawPieChart();
}

// 当周选择下拉框的选中项发生变化时，重新绘制柱状图
void chartPage::on_weekComboBox_currentIndexChanged(int index)
{
    qDebug() << ui->weekComboBox->currentIndex();
    drawColChart(index);
}

// 重写绘制事件，绘制背景图片
void chartPage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // 创建画家对象
    QPainter painter(this);
    // 加载背景图片
    QPixmap pic(":/pic/res/pic/bgi14.jpg");
    // 绘制背景图片
    painter.drawPixmap(0, 0, width(), height(), pic);
    // 调用基类的绘制事件处理函数
    QWidget::paintEvent(event);
}

// 重写大小改变事件，更新界面
void chartPage::resizeEvent(QResizeEvent *event)
{
    // 调用基类的大小改变事件处理函数
    QWidget::resizeEvent(event);
    // 更新界面
    update();
}
