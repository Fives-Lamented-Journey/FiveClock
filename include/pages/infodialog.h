#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>

// 项目内类型仅以指针形式使用
class mybutton;

// 定义命名空间 Ui，包含自动生成的 UI 类
namespace Ui {
class infoDialog;
}

// infoDialog 类继承自 QDialog，用于显示信息对话框
// 职责：纯 UI 收集器 — 仅负责展示和收集用户输入
// 约束：不持有业务数据成员，所有数据通过 getter 从 UI 控件实时读取
class infoDialog : public QDialog
{
    Q_OBJECT

signals:
    void infoAddRecord(infoDialog * infoDialog);

public:
    // 构造函数：传入初始值用于 UI 显示（计时结束后回填），infoEditable 控制是否可编辑
    explicit infoDialog(QWidget *parent = nullptr,
                        const QString &startTime="", const QString &endTime="",
                        const int &consumTime=0, const int &counter=0,
                        QString detail="", bool infoEditable=false);
    ~infoDialog();

    // ---------- Getter 接口（从 UI 控件实时读取） ----------
    QString getTodayTime() const;      // 当前日期（yyyy-MM-dd）
    QString getdetailText() const;     // 详细描述文本
    QString getstartLine() const;      // 开始时间
    QString getendLine() const;       // 结束时间
    int getconsumBox() const;          // 消耗时长（分钟）
    int getcounterBox() const;         // 分神次数
    QString getclassBox() const;      // 事件分类显示名

private:
    Ui::infoDialog *ui;
    bool m_infoEditable = false;

    mybutton* m_okButton = nullptr;
    mybutton* m_noButton = nullptr;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};

#endif // INFODIALOG_H
