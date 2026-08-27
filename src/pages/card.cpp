#include "pages/card.h"
#include "ui_card.h"
#include "widgets/mybutton.h"
#include <QHBoxLayout>
#include <QPushButton>

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

    // 定义卡片框架的样式表
    QString qssCardFrame="QFrame#cardframe{border:1px groove white;"
                           "border-radius:20px;"
                           "background-color: rgba(128, 128, 128, 128);}";
    // 设置卡片框架的样式表
    ui->cardframe->setStyleSheet(qssCardFrame);

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
