#ifndef CARD_H
#define CARD_H

#include <QWidget>
#include <QHBoxLayout>
#include "mybutton.h"
#include "infodialog.h"
namespace Ui {
class card;
}

// 定义一个名为 card 的类，继承自 QWidget
class card : public QWidget
{
    Q_OBJECT

public:
    // 构造函数，接收多个参数用于初始化卡片信息
    // parent：父窗口指针，默认为 nullptr
    // cardEventId：卡片事件的 ID，默认为 0
    // cardBegTime：卡片事件的开始时间，默认为空字符串
    // cardEndTime：卡片事件的结束时间，默认为空字符串
    // cardConsuTime：卡片事件的持续时间，默认为 0
    // cardcounter：分神次数，默认为 0
    // cardDetail：卡片的详细信息，默认为空字符串
    // cardClass：卡片的类别，默认为空字符串
    // cardCreateDate：卡片的创建日期，默认为空字符串
    explicit card(QWidget *parent = nullptr,int cardEventId=0,
                  const QString& cardBegTime="",const QString& cardEndTime="",int cardConsuTime=0,
                  int cardcounter=0 ,const QString&  cardDetail="",const QString& cardClass="",
                  const QString& cardCreateDate=""
                  );
    // 析构函数
    ~card();

public:
    // 卡片事件的开始时间
    QString cardBegTime;
    // 卡片事件的结束时间
    QString cardEndTime;
    // 卡片的类别
    QString cardClass;
    // 卡片的详细信息
    QString cardDetail;
    // 卡片事件的 ID
    int cardEverntId;
    // 卡片事件的持续时间
    int cardConsuTime;
    // 分神次数
    int cardcounter;
    // 卡片的创建日期
    QString cardCreateDate;

signals:
    // 信号：用于通知卡片信息需要更新，参数为当前卡片对象的指针
    void upCardInfo(card* oneCard);
    // 信号：用于通知卡片信息需要删除，参数为当前卡片对象的指针
    void delCardInfo(card* oneCard);
private slots:
    // 槽函数：触发 upCardInfo 信号
    void emitUpCardInfo();
    // 槽函数：触发 delCardInfo 信号
    void emitDelCardInfo();

private:
    // 删除按钮，使用自定义的 mybutton 类
    mybutton* delButton=new mybutton(":/icon/res/icon/del_ini.png",
                                       ":/icon/res/icon/del_hover.png",
                                       ":/icon/res/icon/del_pressed.png");
    // 更新按钮，使用自定义的 mybutton 类
    mybutton* updButton=new mybutton(":/icon/res/icon/alter_ini.png",
                                       ":/icon/res/icon/alter_hover.png",
                                       ":/icon/res/icon/alter_pressed.png");

private:
    // 水平布局，用于管理删除和更新按钮
    QHBoxLayout* cardHbox;
    // 信息对话框指针
    infoDialog* cardDialog;

private:
    // UI 指针，用于访问 UI 设计文件中的控件
    Ui::card *ui;
};

#endif // CARD_H
