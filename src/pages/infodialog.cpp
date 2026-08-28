#include "pages/infodialog.h"
#include "ui_infodialog.h"
#include "widgets/mybutton.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QDateTime>
#include <QDebug>

// 构造函数：将初始值填入 UI 控件，不再存储为成员
infoDialog::infoDialog(QWidget *parent, const QString &startTime, const QString &endTime,
                       const int &consumTime, const int &counter, QString detail, bool infoEditable)
    : QDialog(parent)
    , ui(new Ui::infoDialog)
    , m_infoEditable(infoEditable)
{
    ui->setupUi(this);

    // 填充初始值到 UI 控件
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

    // 创建自定义按钮
    m_okButton = new mybutton(":/icon/res/icon/confirm_ini_buton.png",
                            ":/icon/res/icon/confirm_hover_button.png",
                            ":/icon/res/icon/confirm_pressed_pressed.png",
                            this);
    m_noButton = new mybutton(":/icon/res/icon/cancel_ini_button.png",
                           ":/icon/res/icon/cancel_hover_button.png",
                           ":/icon/res/icon/cancel_pressed_button.png",
                           this);

    // 创建水平布局
    QHBoxLayout *buttonFameLayout = new QHBoxLayout();
    buttonFameLayout->addStretch(1);
    buttonFameLayout->addWidget(m_okButton);
    buttonFameLayout->addStretch(1);
    buttonFameLayout->addWidget(m_noButton);
    buttonFameLayout->addStretch(1);
    ui->buttonFrame->setLayout(buttonFameLayout);

    // 连接按钮信号
    connect(m_okButton, &QPushButton::clicked, this, &infoDialog::accept);
    connect(m_noButton, &QPushButton::clicked, this, &infoDialog::reject);
}

infoDialog::~infoDialog()
{
    delete ui;
}

// ---------- Getter 实现（从 UI 控件实时读取） ----------

QString infoDialog::getTodayTime() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd");
}

QString infoDialog::getstartLine() const
{
    return ui->startLineEdit->text();
}

QString infoDialog::getendLine() const
{
    return ui->endLineEdit->text();
}

int infoDialog::getconsumBox() const
{
    return ui->consumSpinBox->value();
}

int infoDialog::getcounterBox() const
{
    return ui->counterSpinBox->value();
}

QString infoDialog::getclassBox() const
{
    return ui->classComboBox->currentText();
}

QString infoDialog::getdetailText() const
{
    return ui->detailTextEdit->toPlainText();
}

// ---------- 绘制事件 ----------

void infoDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QPixmap pic(":/pic/res/pic/dialog_background .png");
    painter.drawPixmap(0, 0, width(), height(), pic);
    QWidget::paintEvent(event);
}

void infoDialog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}
