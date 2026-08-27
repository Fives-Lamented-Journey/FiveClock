#include "pages/subpage.h"
#include "ui_subpage.h"
#include "pages/card.h"
#include "widgets/customcalendarwidget.h"
#include "pages/infodialog.h"
#include "widgets/mybutton.h"
#include "db/sqldbm.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QSqlQuery>
#include <QDate>
#include <QPainter>

// 构造函数，初始化子页面
subPage::subPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::subPage)
{
    ui->setupUi(this);

    // 初始化所有 mybutton（统一以 this 为 parent，确保 Qt 自动管理生命周期）
    homeButton = new mybutton(":/icon/res/icon/home_init.png",
                              ":/icon/res/icon/home_hover.png",
                              ":/icon/res/icon/home_pressed.png",
                              this);
    subButton = new mybutton(":/icon/res/icon/card_pressed.png",
                             ":/icon/res/icon/card_hover.png",
                             ":/icon/res/icon/card_pressed.png",
                             this);
    chartButton = new mybutton(":/icon/res/icon/chartButton_ini.png",
                               ":/icon/res/icon/chartButton_hover.png",
                               ":/icon/res/icon/chartButton_pressed.png",
                               this);
    subAddRecordButton = new mybutton(":/icon/res/icon/add_ini_button.png",
                                      ":/icon/res/icon/add_hover_button.png",
                                      ":/icon/res/icon/add_pressed_button.png",
                                      this);

    // 创建自定义日历部件
    calendar = new CustomCalendarWidget(this);

    // 创建子部件和布局
    subWidget = new QWidget(this);
    subHbox = new QVBoxLayout();
    subScrollVbox = new QVBoxLayout();

    // 创建垂直布局并添加按钮
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->addStretch(1);
    vBox->addWidget(homeButton);
    vBox->addStretch(1);
    vBox->addWidget(subButton);
    vBox->addStretch(1);
    vBox->addWidget(chartButton);
    vBox->addStretch(1);

    // 将布局设置到子页面框架上
    ui->subPageFrame->setLayout(vBox);

    // 将子水平布局关联到UI中的日历部件
    subHbox = new QVBoxLayout(ui->calendarWidget);
    subHbox->addWidget(calendar);
    // 清除布局的边距
    subHbox->setContentsMargins(0, 0, 0, 0);

    // 为子部件设置布局
    subWidget->setLayout(subScrollVbox);
    // 将子部件添加到滚动区域
    ui->scrollArea->setWidget(subWidget);

    // 设置添加记录按钮的父部件为滚动区域
    subAddRecordButton->setParent(ui->scrollArea);
    // 初始化添加按钮的位置
    updateAddButtonPosition();
    // 设置添加按钮的固定大小
    subAddRecordButton->setFixedSize(70, 70);
    // 设置添加按钮的图标大小
    subAddRecordButton->setIconSize(subAddRecordButton->size());

    // 连接滚动条信号到更新按钮位置的槽函数
    connect(ui->scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &subPage::updateAddButtonPosition);
    connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, &subPage::updateAddButtonPosition);

    // 连接窗口大小变化信号到更新按钮位置的槽函数
    connect(this, &subPage::resized, this, &subPage::updateAddButtonPosition);

    // 显示卡片记录
    showCard();
    // 连接日历日期选择信号到更新卡片列表的槽函数
    connect(calendar, &CustomCalendarWidget::dateSelected, this, &subPage::subUpdateCardList);
}

// 析构函数，释放UI资源
subPage::~subPage()
{
    delete ui;
}

// 注入数据管理对象
void subPage::setDataBase(sqlDBM *dbm)
{
    m_dbm = dbm;
}

// 显示卡片记录的函数
void subPage::showCard()
{
    // 清除滚动布局中的旧组件
    while (subScrollVbox->count() > 0) {
        QLayoutItem *item = subScrollVbox->takeAt(0);
        QWidget *widget = item->widget();
        if (widget) {
            delete widget;
        }
        delete item;
    }

    int subEventId;
    QString subBegTime;
    QString subEndTime;
    int subConsuTime;
    int subCounter;
    QString subDetail;
    QString subClass;

    QSqlQuery query;
    if (m_dbm) {
        // 直接通过注入的数据管理对象获取当日记录
        query = m_dbm->sortedRecord(selectedToday);
    }
    // 未注入数据管理对象时（如构造期首次刷新），query 保持为空，展示空记录提示
    qDebug() << "sortedRecordover";

    // 如果查询结果为空
    if (query.size() == 0)
    {
        qDebug() << "query.size=0";
        // 创建提示标签
        QLabel *tipLab = new QLabel();
        tipLab->setText("记录为空,主人要记得坚持记录哦");
        // 设置滚动区域的背景颜色
        ui->scrollArea->setStyleSheet("background-color:red");
        // 设置提示标签的样式
        tipLab->setStyleSheet("font-size:50px;color:white");
        // 将提示标签添加到滚动布局中
        subScrollVbox->addWidget(tipLab);
    }

    // 遍历查询结果
    while (query.next()) {
        subEventId = query.value(0).toInt();
        subBegTime = query.value(1).toString();
        subEndTime = query.value(2).toString();
        subConsuTime = query.value(3).toInt();
        subCounter = query.value(4).toInt();
        subDetail = query.value(5).toString();
        subClass = query.value(6).toString();

        // 创建卡片部件
        subCard = new card(this, subEventId, subBegTime, subEndTime, subConsuTime,
                           subCounter, subDetail, subClass, selectedToday);

        // 连接卡片更新信号到更新卡片的槽函数
        connect(subCard, &card::upCardInfo, this, &subPage::subUpdateCard);
        // 连接卡片删除信号到删除卡片的槽函数
        connect(subCard, &card::delCardInfo, this, &subPage::subDelCard);

        // 将卡片添加到滚动布局中
        subScrollVbox->addWidget(subCard);
    }

    // 为子部件设置布局
    subWidget->setLayout(subScrollVbox);
    // 将子部件添加到滚动区域
    ui->scrollArea->setWidget(subWidget);
}

// 当日期选择改变时更新卡片列表的函数
void subPage::subUpdateCardList(const QDate &date)
{
    // 更新选中的日期
    selectedToday = date.toString("yyyy-MM-dd");
    qDebug() << "subUpdateCardList:" + selectedToday;
    // 显示卡片记录
    showCard();
    // 更新页面
    update();
}

// 更新卡片信息的函数
void subPage::subUpdateCard(card *oneCard)
{
    // 创建信息对话框（栈对象：exec 为模态阻塞，返回后自动析构，无需手动 delete）
    infoDialog subDialog(this, oneCard->cardBegTime, oneCard->cardEndTime,
                         oneCard->cardConsuTime, oneCard->cardcounter,
                         oneCard->cardDetail, true);

    // 如果对话框以接受方式关闭
    if (subDialog.exec() == QDialog::Accepted)
    {
        qDebug() << subDialog.getstartLine();
        qDebug() << subDialog.getendLine();
        qDebug() << subDialog.getconsumBox();
        qDebug() << subDialog.getcounterBox();
        qDebug() << subDialog.getclassBox();
        qDebug() << subDialog.getdetailText();

        // 更新卡片信息
        oneCard->cardBegTime = subDialog.getstartLine();
        oneCard->cardEndTime = subDialog.getendLine();
        oneCard->cardConsuTime = subDialog.getconsumBox();
        oneCard->cardcounter = subDialog.getcounterBox();
        oneCard->cardClass = subDialog.getclassBox();
        oneCard->cardDetail = subDialog.getdetailText();

        // 发出更新记录的信号
        emit updateRecord(oneCard);
        // 显示卡片记录
        showCard();
        // 重绘页面
        repaint();
    }
}

// 删除卡片信息的函数
void subPage::subDelCard(card *oneCard)
{
    // 发出删除记录的信号
    emit deleteRecord(oneCard);
    // 显示卡片记录
    showCard();
    // 重绘页面
    repaint();
}

// 绘制事件处理函数，用于绘制背景图片
void subPage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QPixmap pic(":/pic/res/pic/bgi14.jpg");
    // 绘制背景图片
    painter.drawPixmap(0, 0, width(), height(), pic);
    // 调用基类的绘制事件处理函数
    QWidget::paintEvent(event);
}

// 窗口大小改变事件处理函数
void subPage::resizeEvent(QResizeEvent *event)
{
    // 调用基类的窗口大小改变事件处理函数
    QWidget::resizeEvent(event);
    // 发出窗口大小改变的信号
    emit resized();
    // 更新页面
    update();
}

// 更新添加按钮位置的函数
void subPage::updateAddButtonPosition()
{
    if (!subAddRecordButton || !ui->scrollArea)
        return;

    // 获取滚动区域的视口
    QWidget* viewport = ui->scrollArea->viewport();

    // 计算按钮在视口右下角的位置
    int x = viewport->width() - subAddRecordButton->width() - 50;
    int y = viewport->height() - subAddRecordButton->height() - 50;

    // 将坐标转换为滚动区域的坐标系
    QPoint pos = viewport->mapTo(ui->scrollArea, QPoint(x, y));

    // 设置按钮的位置
    subAddRecordButton->move(pos);
    // 将按钮置于顶层
    subAddRecordButton->raise();
}

// 重写显示事件处理函数，确保首次显示时按钮位置正确
void subPage::showEvent(QShowEvent *event)
{
    // 调用基类的显示事件处理函数
    QWidget::showEvent(event);
    // 更新添加按钮的位置
    updateAddButtonPosition();
}
