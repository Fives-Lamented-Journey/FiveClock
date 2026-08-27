#include "infodialog.h"
#include "ui_infodialog.h"
#include "mybutton.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QDebug>

// 构造函数的实现
infoDialog::infoDialog(QWidget *parent,const QString &startTime,const QString &endTime,
                       const int & consumTime,const int & counter,QString detail,bool infoEditable)
    :QDialog(parent),
    startTime(startTime),endTime(endTime),consumTime(consumTime),
    counter(counter),detail(detail),ui(new Ui::infoDialog),
    infoEditable(infoEditable)
{
    // 设置 UI
    ui->setupUi(this);

    // 初始化按钮（统一以 this 为 parent）
    okButton = new mybutton(":/icon/res/icon/confirm_ini_buton.png",
                            ":/icon/res/icon/confirm_hover_button.png",
                            ":/icon/res/icon/confirm_pressed_pressed.png",
                            this);
    noButton = new mybutton(":/icon/res/icon/cancel_ini_button.png",
                           ":/icon/res/icon/cancel_hover_button.png",
                           ":/icon/res/icon/cancel_pressed_button.png",
                           this);

    // 设置开始时间、结束时间、消耗时间、计数器值和详细信息到对应的 UI 控件
    ui->startLineEdit->setText(startTime);
    ui->endLineEdit->setText(endTime);
    ui->consumSpinBox->setValue(consumTime);
    ui->counterSpinBox->setValue(counter);
    ui->detailTextEdit->setText(detail);

    // 根据可编辑标志设置 UI 控件的启用状态
    ui->startLineEdit->setEnabled(infoEditable);
    ui->endLineEdit->setEnabled(infoEditable);
    ui->consumSpinBox->setEnabled(infoEditable);
    ui->counterSpinBox->setEnabled(infoEditable);

    // 创建水平布局
    QHBoxLayout *buttonFameLayout=new QHBoxLayout();

    // 在布局中添加伸缩项，将按钮居中显示
    buttonFameLayout->addStretch(1);
    buttonFameLayout->addWidget(okButton);
    buttonFameLayout->addStretch(1);
    buttonFameLayout->addWidget(noButton);
    buttonFameLayout->addStretch(1);

    // 将布局设置到按钮框架中
    ui->buttonFrame->setLayout(buttonFameLayout);

    // 连接确认按钮的点击信号到对话框的 accept 槽
    connect(okButton,&QPushButton::clicked,this,&infoDialog::accept);
    // 连接取消按钮的点击信号到对话框的 reject 槽
    connect(noButton,&QPushButton::clicked,this,&infoDialog::reject);
}

// 析构函数的实现，释放 UI 对象
infoDialog::~infoDialog()
{
    delete ui;
}

// 获取当前日期的函数实现
QString infoDialog::getTodayTime()
{
    // 获取当前日期和时间
    todayTimer = QDateTime::currentDateTime();
    // 将日期和时间格式化为 "yyyy-MM-dd" 字符串并返回
    return todayTimer.toString("yyyy-MM-dd");
}

// 获取详细信息的函数实现
QString infoDialog::getDetail()
{
    // 获取详细文本编辑框中的纯文本信息
    return ui->detailTextEdit->toPlainText();
}

// 获取开始时间的函数实现
QString infoDialog::getstartLine()
{
    // 获取开始时间编辑框中的文本信息
    return ui->startLineEdit->text();
}

// 获取结束时间的函数实现
QString infoDialog::getendLine()
{
    // 获取结束时间编辑框中的文本信息
    return ui->endLineEdit->text();
}

// 获取消耗时间的函数实现
int infoDialog::getconsumBox()
{
    // 获取消耗时间自旋框中的值
    return ui->consumSpinBox->value();
}

// 获取计数器值的函数实现
int infoDialog::getcounterBox()
{
    // 获取计数器自旋框中的值
    return ui->counterSpinBox->value();
}

// 获取分类信息的函数实现
QString infoDialog::getclassBox()
{
    // 获取分类下拉框中的当前文本信息
    return ui->classComboBox->currentText();
}

// 获取详细文本信息的函数实现
QString infoDialog::getdetailText()
{
    // 获取详细文本编辑框中的纯文本信息
    return ui->detailTextEdit->toPlainText();
}

// 重写的绘制事件处理函数实现
void infoDialog::paintEvent(QPaintEvent *event)
{
    // 忽略事件参数
    Q_UNUSED(event);
    // 创建 QPainter 对象，用于绘制
    QPainter painter(this);
    // 加载背景图片
    QPixmap pic(":/pic/res/pic/dialog_background .png");
    // 绘制背景图片，铺满整个对话框
    painter.drawPixmap(0, 0, width(), height(), pic);
    // 调用基类的绘制事件处理函数
    QWidget::paintEvent(event);
}

// 重写的大小改变事件处理函数实现
void infoDialog::resizeEvent(QResizeEvent *event)
{
    // 调用基类的大小改变事件处理函数
    QWidget::resizeEvent(event);
    // 更新对话框
    update();
}
