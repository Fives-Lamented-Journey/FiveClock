#include "widgets/customcalendarwidget.h"
#include "service/record_service.h"
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QApplication>
#include <QDebug>
#include <QSet>
#include <QStyle>
#include <qtimer.h>

// 注入记录业务服务：部件通过它批量查询有记录日期、订阅 dataChanged 信号，不再直连 dbm
void CustomCalendarWidget::setRecordService(RecordService *service)
{
    // 保存指针供 refreshDayStyles 查询使用（构造期未注入时全部按钮按无记录样式展示）
    m_recordService = service;
    // UniqueConnection 防御重复注入导致的重复刷新；context 为 this，析构时自动断开
    connect(service, &RecordService::dataChanged, this, &CustomCalendarWidget::onDataChanged, Qt::UniqueConnection);
}

// 数据变更通知的响应槽：更新日历显示与日期按钮样式
void CustomCalendarWidget::onDataChanged()
{
    updateCalendar();
}

// 批量刷新日期按钮样式：一次枚举当月有记录的日期集合后统一设置，
// 替代原先逐按钮发信号查询的用法（每次刷新由 N 次数据库往返降为 1 次）
void CustomCalendarWidget::refreshDayStyles()
{
    // 收集当月有记录的日期（服务层沿用"表存在且有记录"的判断条件，一次批量返回）
    QSet<QString> markedDays;
    if (m_recordService) {
        markedDays = m_recordService->markedDaysOfMonth(currentDate.year(), currentDate.month());
    }

    // 遍历网格中的日期按钮，按是否有记录/是否今日设置动态属性
    // （日期按钮样式由 app.qss 的 CustomCalendarWidget QPushButton[markedDay=...]/[isToday=...] 规则提供）
    const QDate today = QDate::currentDate();
    for (int i = 2; i < 7; ++i) {
        for (int j = 0; j < gridLayout->columnCount(); ++j) {
            QLayoutItem* item = gridLayout->itemAtPosition(i, j);
            if (item) {
                if (QPushButton* btn = qobject_cast<QPushButton*>(item->widget())) {
                    // 将按钮对应的日期转换为 yyyy-MM-dd 字符串
                    QDate day(currentDate.year(), currentDate.month(), btn->text().toInt());
                    QString strDay = day.toString("yyyy-MM-dd");
                    // 设置动态属性并强制刷新样式：
                    // onDataChanged 触发的刷新发生在按钮已显示之后，
                    // 属性变化需 unpolish/polish 才能让属性选择器规则重新生效
                    btn->setProperty("markedDay", markedDays.contains(strDay));
                    btn->setProperty("isToday", day == today);
                    btn->style()->unpolish(btn);
                    btn->style()->polish(btn);
                }
            }
        }
    }
}

// 自定义日历小部件的构造函数实现
CustomCalendarWidget::CustomCalendarWidget(QWidget* parent) : QWidget(parent)
{
    // 创建一个网格布局并设置为当前小部件的布局
    // 网格布局用于将各个小部件（如按钮、标签）按照网格的形式排列
    gridLayout = new QGridLayout(this);

    // 创建上一个月按钮
    // 该按钮用于切换到上一个月的日历显示
    prevButton = new QPushButton("<", this);

    // 创建下一个月按钮
    // 该按钮用于切换到下一个月的日历显示
    nextButton = new QPushButton(">", this);

    // 创建用于显示月份和年份的标签
    // 该标签会显示当前日历所显示的月份和年份
    monthYearLabel = new QLabel(this);

    // 连接上一个月按钮的点击信号到prevMonth槽函数
    // 当用户点击上一个月按钮时，会触发prevMonth函数
    connect(prevButton, &QPushButton::clicked, this, &CustomCalendarWidget::prevMonth);

    // 连接下一个月按钮的点击信号到nextMonth槽函数
    // 当用户点击下一个月按钮时，会触发nextMonth函数
    connect(nextButton, &QPushButton::clicked, this, &CustomCalendarWidget::nextMonth);

    // 设置翻月按钮的动态属性
    // （样式由 app.qss 的 QPushButton[calendarNavButton="true"] 规则提供：
    //   绿色底/白字/圆角，悬停加深）
    prevButton->setProperty("calendarNavButton", true);
    nextButton->setProperty("calendarNavButton", true);

    // 设置月份和年份标签的 objectName
    // （样式由 app.qss 的 QLabel#calendarMonthYearLabel 规则提供：18px 加粗/透明底/圆角）
    monthYearLabel->setObjectName("calendarMonthYearLabel");

    // 将上一个月按钮添加到网格布局的第0行第0列
    gridLayout->addWidget(prevButton, 0, 0);

    // 将月份和年份标签添加到网格布局的第0行第1列，跨越5列并居中显示
    gridLayout->addWidget(monthYearLabel, 0, 1, 1, 5, Qt::AlignCenter);

    // 将下一个月按钮添加到网格布局的第0行第6列
    gridLayout->addWidget(nextButton, 0, 6);

    // 星期几的表头列表
    QStringList headers = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

    // 循环创建并添加表头标签到网格布局的第1行
    // （样式由 app.qss 的 QLabel[calendarHeaderLabel="true"] 规则提供：加粗/透明底/大内边距）
    for (int i = 0; i < 7; ++i) {
        QLabel* headerLabel = new QLabel(headers[i], this);
        headerLabel->setProperty("calendarHeaderLabel", true);

        gridLayout->addWidget(headerLabel, 1, i);
        headerLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    // 日历整体白色背景由 app.qss 的 CustomCalendarWidget 规则提供

    // 将当前显示的日期初始化为系统当前日期
    currentDate = QDate::currentDate();

    // 调用更新日历的函数，显示当前月份的日历
    updateCalendar();
}

// 自定义日历小部件的析构函数实现
CustomCalendarWidget::~CustomCalendarWidget()
{
    // 释放资源
    // 目前代码中没有显式的资源释放操作，可根据实际情况添加
}

// 设置网格布局中每行的拉伸因子
void CustomCalendarWidget::setStrench()
{
    for(int row=0;row<gridLayout->rowCount();row++)
    {
        if (row == 1) {
            // 第1行的拉伸因子设置为1
            gridLayout->setRowStretch(row, 1);
        } else {
            // 其他行的拉伸因子设置为2
            gridLayout->setRowStretch(row, 2);
        }
    }
}

// 处理上一个月按钮点击事件的槽函数
void CustomCalendarWidget::prevMonth()
{
    // 将当前显示的日期减去一个月
    currentDate = currentDate.addMonths(-1);

    // 调用更新日历的函数，显示上一个月的日历
    updateCalendar();
}

// 处理下一个月按钮点击事件的槽函数
void CustomCalendarWidget::nextMonth()
{
    // 将当前显示的日期加上一个月
    currentDate = currentDate.addMonths(1);

    // 调用更新日历的函数，显示下一个月的日历
    updateCalendar();
}

// 处理日期按钮点击事件的槽函数
void CustomCalendarWidget::dayClicked(int day)
{
    // 根据当前显示的年份、月份和点击的日期创建一个QDate对象
    QDate selectedDate(currentDate.year(), currentDate.month(), day);

    // 发出dateSelected信号，携带所选日期信息
    emit dateSelected(selectedDate);

    // 打印所选日期
    qDebug() << "Selected date:" << selectedDate.toString(Qt::ISODate);
}

// 更新日历显示的函数
void CustomCalendarWidget::updateCalendar()
{
    // 设置月份和年份标签的文本为当前显示日期的月份和年份
    monthYearLabel->setText(currentDate.toString("MMMM yyyy"));

    // 清除之前的日期按钮
    // 遍历网格布局中第2行及以下的所有小部件并删除
    for (int i = 2; i < gridLayout->rowCount(); ++i) {
        for (int j = 0; j < gridLayout->columnCount(); ++j) {
            QLayoutItem* item = gridLayout->itemAtPosition(i, j);
            if (item) {
                QWidget* widget = item->widget();
                if (widget) {
                    delete widget;
                }
            }
        }
    }

    // 获取当前显示月份的第一天
    QDate firstDayOfMonth(currentDate.year(), currentDate.month(), 1);

    // 获取第一天是星期几
    int firstDayOfWeek = firstDayOfMonth.dayOfWeek();

    // 调整周日为0
    if (firstDayOfWeek == 7) firstDayOfWeek = 0;

    // 获取当前显示月份的天数
    int daysInMonth = currentDate.daysInMonth();

    // 日期计数器，从1开始
    int dayCounter = 1;


    // 循环创建并添加日期按钮到网格布局
    for (int row = 2; row < 7; ++row) {
        for (int col = 0; col < 7; ++col) {
            // 如果是第一行且列号小于第一天是星期几，或者日期计数器超过了当前月份的天数，则跳过
            if ((row == 2 && col < firstDayOfWeek) || dayCounter > daysInMonth) {
                continue;
            }
            // 创建日期按钮
            QPushButton* dayButton = new QPushButton(QString::number(dayCounter), this);

            // 连接日期按钮的点击信号到dayClicked槽函数
            dayButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            connect(dayButton, &QPushButton::clicked, [this, dayCounter]() { dayClicked(dayCounter); });

            // 将日期按钮添加到网格布局
            gridLayout->addWidget(dayButton, row, col);

            // 日期计数器加1
            ++dayCounter;
        }
    }

    // 循环结束后统一设置日期按钮样式（批量：1 次数据库枚举）
    refreshDayStyles();

    // 处理当前事件队列中的所有事件，确保界面更新
    QApplication::processEvents();

    // 设置网格布局中每行的拉伸因子
    setStrench();

    // 强制触发样式更新
    // 在一个单触发定时器中再次设置日期按钮的样式，确保样式更新生效
    QTimer::singleShot(0, this, [this]() {
        refreshDayStyles();
    });
}
