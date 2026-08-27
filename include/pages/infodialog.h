#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>
#include <QDateTime>   // todayTimer 为值成员，需要完整类型

// 项目内类型仅以指针形式使用
class mybutton;

// 定义命名空间 Ui，包含自动生成的 UI 类
namespace Ui {
class infoDialog;
}

// infoDialog 类继承自 QDialog，用于显示信息对话框
class infoDialog : public QDialog
{
    Q_OBJECT

    // 定义信号，当需要添加记录时发出该信号
signals:
    void infoAddRecord(infoDialog * infoDialog);

    // 构造函数和析构函数
public:
    // 显式构造函数，接收父窗口指针、开始时间、结束时间、消耗时间、计数器值、详细信息和是否可编辑标志
    explicit infoDialog(QWidget *parent = nullptr,
                        const QString &startTime="",const QString &endTime="",
                        const int &consumTime=0,const int &counter=0,
                        QString detail="",bool infoEditable=false
                        );
    // 析构函数
    ~infoDialog();

    // 获取当前日期的函数
    QString getTodayTime();
    // 获取详细信息的函数
    QString getDetail();
    // 获取开始时间的函数
    QString getstartLine();
    // 获取结束时间的函数
    QString getendLine();
    // 获取消耗时间的函数
    int getconsumBox();
    // 获取计数器值的函数
    int getcounterBox();
    // 获取分类信息的函数
    QString getclassBox();
    // 获取详细文本信息的函数
    QString getdetailText();

    // 成员变量，用于存储开始时间、结束时间、消耗时间、计数器值、详细信息、分类信息和当前日期
    QString startTime;
    QString endTime;
    int consumTime;
    int counter;
    QString detail;
    QString classfy;

    QString today;
    QDateTime  todayTimer;

    // 私有成员变量
private:
    // 指向自动生成的 UI 类的指针
    Ui::infoDialog *ui;

    // 标志位，指示对话框是否可编辑
    bool infoEditable;

    // 自定义按钮，用于确认操作
    mybutton* okButton = nullptr;
    // 自定义按钮，用于取消操作
    mybutton* noButton = nullptr;

protected:
    // 重写绘制事件处理函数，用于绘制对话框背景
    void paintEvent(QPaintEvent *event) override;
    // 重写大小改变事件处理函数，用于更新对话框
    void resizeEvent(QResizeEvent *event) override;
};

#endif // INFODIALOG_H
