#include "pages/subpage.h"
#include "ui_subpage.h"
#include "pages/card.h"
#include "widgets/customcalendarwidget.h"
#include "pages/infodialog.h"
#include "widgets/mybutton.h"
#include "service/record_service.h"
#include "config/config_service.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QPalette>
#include <QBrush>
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
    // 内容容器保持透明：天空底由视口的画刷提供（见 updateViewportSkyBrush），
    // 卡片直接浮于天空图上，与日历区视觉一致
    subWidget->setObjectName("scrollContents");
    subHbox = new QVBoxLayout();
    subScrollVbox = new QVBoxLayout();
    // 玻璃卡片之间的垂直间距与滚动区边距
    subScrollVbox->setSpacing(12);
    subScrollVbox->setContentsMargins(12, 12, 12, 12);

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

    // 滚动性能关键：视口不透明（天空图无 alpha）以启用 Qt 滚动块拷贝快路径，
    // 拖动滑块时只重绘暴露条而非全视口；底图为平移对齐的天空画刷，与页面背景无缝连续
    ui->scrollArea->viewport()->setAutoFillBackground(true);
    updateViewportSkyBrush();
    // 滚轮步进调小（Qt 默认单步约 3 行字号，一次滚过头，手感发"跳"）
    ui->scrollArea->verticalScrollBar()->setSingleStep(6);

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

    // 添加记录按钮：直接在 subPage 内唤起 InfoDialog → 调用 RecordService 新增
    connect(subAddRecordButton, &QPushButton::clicked, this, &subPage::onAddRecordClicked);
}

// 析构函数，释放UI资源
subPage::~subPage()
{
    delete ui;
}

// 注入记录业务服务：页面通过它查询/增删改记录、订阅 dataChanged 信号
void subPage::setRecordService(RecordService *service)
{
    // 保存指针供 showCard 查询使用（构造期未注入时展示空记录）
    m_recordService = service;
    // UniqueConnection 防御重复注入导致的重复刷新；context 为 this，析构时自动断开
    connect(service, &RecordService::dataChanged, this, &subPage::refreshCards, Qt::UniqueConnection);
}

// 注入配置服务：页面通过它获取常量文本
void subPage::setConfigService(ConfigService *service)
{
    m_configService = service;
}

// 数据变更通知的响应槽：刷新当前日期的卡片列表
void subPage::refreshCards()
{
    // 显示卡片记录
    showCard();
    // 请求重绘（update 会合并多次请求为一次绘制，repaint 的同步立即重绘会造成卡顿）
    update();
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

    // 通过业务服务层获取当日记录（未注入服务时为空列表，展示空记录提示）
    QVector<TimeRecord> records;
    if (m_recordService) {
        records = m_recordService->queryRecordsOfDay(QDate::fromString(selectedToday, "yyyy-MM-dd"));
    }
    qDebug() << "sortedRecordover";

    // 如果记录为空
    if (records.isEmpty())
    {
        qDebug() << "records is empty";
        // 创建提示标签
        QLabel *tipLab = new QLabel();
        tipLab->setText(m_configService
                        ? m_configService->emptyRecordTip()
                        : QStringLiteral("记录为空"));
        // 设置提示标签的 objectName 供全局样式表匹配（样式由 app.qss 的 QLabel#emptyRecordTip 提供）
        tipLab->setObjectName("emptyRecordTip");
        // 将提示标签添加到滚动布局中
        subScrollVbox->addWidget(tipLab);
    }

    // 遍历记录
    for (const TimeRecord &record : records) {
        // 创建卡片部件
        subCard = new card(this, record.eventId, record.begTime, record.endTime,
                           record.consumTime, record.counter, record.detail,
                           record.eventClass, selectedToday);

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

// 更新卡片信息的函数（直接调用 RecordService，由 dataChanged 自动触发刷新）
void subPage::subUpdateCard(card *oneCard)
{
    // 创建信息对话框（栈对象：exec 为模态阻塞，返回后自动析构）
    infoDialog subDialog(this, oneCard->cardBegTime, oneCard->cardEndTime,
                         oneCard->cardConsuTime, oneCard->cardcounter,
                         oneCard->cardDetail, true);

    // 如果对话框以接受方式关闭
    if (subDialog.exec() == QDialog::Accepted)
    {
        // 更新卡片信息
        oneCard->cardBegTime = subDialog.getstartLine();
        oneCard->cardEndTime = subDialog.getendLine();
        oneCard->cardConsuTime = subDialog.getconsumBox();
        oneCard->cardcounter = subDialog.getcounterBox();
        oneCard->cardClass = subDialog.getclassBox();
        oneCard->cardDetail = subDialog.getdetailText();

        // 组装 TimeRecord DTO，直接调用 RecordService 更新
        if (m_recordService) {
            TimeRecord rec;
            rec.eventId = oneCard->cardEverntId;
            rec.begTime = oneCard->cardBegTime;
            rec.endTime = oneCard->cardEndTime;
            rec.consumTime = oneCard->cardConsuTime;
            rec.counter = oneCard->cardcounter;
            rec.detail = oneCard->cardDetail;
            rec.eventClass = oneCard->cardClass;

            QDate date = QDate::fromString(oneCard->cardCreateDate, "yyyy-MM-dd");
            m_recordService->updateRecord(date, rec);
            // 写库成功后 RecordService::dataChanged 触发 refreshCards 自动刷新
        }
    }
}

// 删除卡片信息的函数（直接调用 RecordService，由 dataChanged 自动触发刷新）
void subPage::subDelCard(card *oneCard)
{
    if (m_recordService) {
        QDate date = QDate::fromString(oneCard->cardCreateDate, "yyyy-MM-dd");
        m_recordService->deleteRecord(date, oneCard->cardEverntId);
        // 写库成功后 RecordService::dataChanged 触发 refreshCards 自动刷新
    }
}

// 添加空记录的槽函数（直接调用 RecordService，由 dataChanged 自动触发刷新）
void subPage::onAddRecordClicked()
{
    // 创建信息对话框（栈对象，默认空记录模式）
    infoDialog subDialog(this, "", "", 0, 0, "", true);

    if (subDialog.exec() == QDialog::Accepted)
    {
        // 组装 TimeRecord DTO
        TimeRecord rec;
        rec.begTime = subDialog.getstartLine();
        rec.endTime = subDialog.getendLine();
        rec.consumTime = subDialog.getconsumBox();
        rec.counter = subDialog.getcounterBox();
        rec.detail = subDialog.getdetailText();
        rec.eventClass = subDialog.getclassBox();

        // 使用当前选中日期
        QDate date = QDate::fromString(selectedToday, "yyyy-MM-dd");

        if (m_recordService) {
            m_recordService->addEmptyRecord(date, rec);
            // 写库成功后 RecordService::dataChanged 触发 refreshCards 自动刷新
        }
    }
}

// 绘制事件处理函数，用于绘制背景图片
void subPage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // 背景缩放缓存：仅在窗口尺寸变化时重算一次，其余帧直接贴图
    // （每帧从 qrc 解码 PNG 并拉伸大图是滚动卡顿的根因）
    if (m_bgScaled.size() != size()) {
        m_bgScaled = QPixmap(":/pic/res/pic/bgi05.png")
                         .scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_bgScaled);
    // 调用基类的绘制事件处理函数
    QWidget::paintEvent(event);
}

// 窗口大小改变事件处理函数
void subPage::resizeEvent(QResizeEvent *event)
{
    // 调用基类的窗口大小改变事件处理函数
    QWidget::resizeEvent(event);
    // 布局稳定后刷新视口天空画刷（缩放缓存 + 平移对齐量都随尺寸/位置变化）
    updateViewportSkyBrush();
    // 发出窗口大小改变的信号
    emit resized();
    // 更新页面
    update();
}

// 滚动区视口的天空画刷：以缩放缓存的天空图为不透明底，平移 scrollArea
// 在页面中的位置量，使视口内天空与周边（侧栏/日历区）背景无缝连续；
// 视口不透明（图片无 alpha）保住滚动块拷贝快路径
void subPage::updateViewportSkyBrush()
{
    if (!ui->scrollArea || size().isEmpty())
        return;

    // 背景缩放缓存：仅在窗口尺寸变化时重算一次
    if (m_bgScaled.size() != size()) {
        m_bgScaled = QPixmap(":/pic/res/pic/bgi05.png")
                         .scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    QWidget *vp = ui->scrollArea->viewport();
    const QPoint pagePos = ui->scrollArea->pos();
    QBrush skyBrush(m_bgScaled);
    // 设备坐标 (x,y) 采样图片 (x - pagePos.x(), y - pagePos.y())，
    // 与 subPage::paintEvent 在 (0,0) 绘制整页背景的坐标对齐
    skyBrush.setTransform(QTransform::fromTranslate(pagePos.x(), pagePos.y()));
    QPalette vpPal = vp->palette();
    vpPal.setBrush(QPalette::Window, skyBrush);
    vp->setPalette(vpPal);
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
    // 仅在位置真正变化时才 move/raise：滚动条拖动时 valueChanged 高频触发，
    // 每次都 raise 会造成无谓的层叠调整开销
    if (subAddRecordButton->pos() != pos) {
        // 设置按钮的位置
        subAddRecordButton->move(pos);
        // 将按钮置于顶层
        subAddRecordButton->raise();
    }
}

// 重写显示事件处理函数，确保首次显示时按钮位置正确
void subPage::showEvent(QShowEvent *event)
{
    // 调用基类的显示事件处理函数
    QWidget::showEvent(event);
    // 更新添加按钮的位置
    updateAddButtonPosition();
}
