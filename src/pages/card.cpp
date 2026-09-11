#include "pages/card.h"
#include "ui_card.h"
#include "widgets/mybutton.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>

// 构造函数的实现
card::card(QWidget *parent,int cardEventId,
           const QString& cardBegTime,const QString& cardEndTime,int cardConsuTime,
           int cardcounter ,const QString&  cardDetail,const QString& cardClass,
           const QString& cardCreateDate)
    : QWidget(parent)
    // 初始化成员变量
    ,cardBegTime(cardBegTime),cardEndTime(cardEndTime)
    ,cardClass(cardClass),cardDetail(cardDetail)
    ,cardEverntId(cardEventId),cardConsuTime(cardConsuTime),cardcounter(cardcounter)
    ,cardCreateDate(cardCreateDate)
    , ui(new Ui::card)
{
    // 设置 UI
    ui->setupUi(this);

    // 初始化按钮（统一以 this 为 parent）
    delButton = new mybutton(":/icon/res/icon/del_ini.png",
                             ":/icon/res/icon/del_hover.png",
                             ":/icon/res/icon/del_pressed.png",
                             this);
    updButton = new mybutton(":/icon/res/icon/alter_ini.png",
                            ":/icon/res/icon/alter_hover.png",
                            ":/icon/res/icon/alter_pressed.png",
                            this);

    // 设置开始和结束时间的标签文本
    ui->begEndLab->setText(cardBegTime+"--"+cardEndTime);
    // 设置类别和持续时间的标签文本
    ui->classConsuLab->setText(cardClass+":"+QString::number(cardConsuTime)+"min");
    // 设置分神次数的标签文本
    ui->counterShowLab->setText("分神次数:"+QString::number(cardcounter));
    // 设置详细信息的文本浏览器内容
    ui->detailTextBrowser->setText(cardDetail);

    // 卡片框架的边框/圆角/玻璃底样式由 app.qss 的 QFrame#cardframe 规则提供。
    // QFrame 对 app 级样式表的背景/边框默认不走样式绘制，必须显式开启 WA_StyledBackground；
    // 同时关闭系统原生面板框（StyledPanel），避免与 QSS 边框重复绘制。
    ui->cardframe->setAttribute(Qt::WA_StyledBackground, true);
    ui->cardframe->setFrameShape(QFrame::NoFrame);
    // 内部嵌套框架仅作布局容器，同样关闭原生面板框（视觉由 QSS 展平）
    ui->frame->setFrameShape(QFrame::NoFrame);
    ui->delAltFrame->setFrameShape(QFrame::NoFrame);

    // 创建水平布局
    cardHbox=new QHBoxLayout();
    // 添加伸缩项，使按钮居中显示
    cardHbox->addStretch(1);
    // 将删除按钮添加到布局中
    cardHbox->addWidget(delButton);
    // 添加伸缩项
    cardHbox->addStretch(1);
    // 将更新按钮添加到布局中
    cardHbox->addWidget(updButton);
    // 添加伸缩项
    cardHbox->addStretch(1);
    // 设置删除和更新按钮所在框架的布局
    ui->delAltFrame->setLayout(cardHbox);

    // 连接删除按钮的点击信号到 emitDelCardInfo 槽函数
    connect(delButton,&QPushButton::clicked,this,&card::emitDelCardInfo);
    // 连接更新按钮的点击信号到 emitUpCardInfo 槽函数
    connect(updButton,&QPushButton::clicked,this,&card::emitUpCardInfo);
}

// 析构函数的实现
card::~card()
{
    // 释放 UI 对象的内存
    delete ui;
}

// 触发 upCardInfo 信号的槽函数实现
void card::emitUpCardInfo()
{
    // 发射 upCardInfo 信号，传递当前卡片对象的指针
    emit upCardInfo(this);
}

// 触发 delCardInfo 信号的槽函数实现
void card::emitDelCardInfo()
{
    // 发射 delCardInfo 信号，传递当前卡片对象的指针
    emit delCardInfo(this);
}
