#ifndef ASSEMPAGE_H
#define ASSEMPAGE_H

#include <QWidget>
#include "homePage.h"
#include "subpage.h"
#include "sqldbm.h"
#include "chartpage.h"
#include  <QMouseEvent>
#include "petwindow.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class assemPage;
}
QT_END_NAMESPACE

// assemPage 类，继承自 QWidget
class assemPage : public QWidget
{
    Q_OBJECT

public:
    // 构造函数，用于初始化页面和相关组件
    assemPage(QWidget *parent = nullptr);
    // 析构函数，用于释放资源
    ~assemPage();

    // 创建托盘图标菜单
    void createTrayIconMenu();;

private:
    // 主页面指针
    Widget* aHomePage;
    // 子页面指针
    subPage* aSubPage;
    // 图表页面指针
    chartPage* aChartPage;
    // 信息对话框指针
    infoDialog * assInfoDialog;

    // 数据库管理类实例
    sqlDBM DBM;

    // 标记窗口是否正在移动
    bool assMoving;
    // 记录鼠标按下时的相对位置
    QPoint assLastPose;

    // 系统托盘图标指针
    QSystemTrayIcon *trayIcon;
    // 托盘菜单指针
    QMenu *trayMenu;

    // 桌宠窗口指针
    petwindow *pet;

private slots:
    // 切换到主页面
    void pageChangeToHome();
    // 切换到子页面
    void pageChangeToSub();
    // 切换到图表页面
    void pageChangeToChart();

    // 设置屏幕状态（全屏/正常）
    void SetScreenState();
    // 退出窗口
    void exitWindow();

    // 添加记录
    void assAddRecord();
    // 更新记录
    void assUpdateRecord(card* oneCard);
    // 删除记录
    void assDeleteRecord(card* oneCard);
    // 添加空记录
    void assAddEmptyRecord();

    // 对记录进行排序
    void assSortedRecord(QSqlQuery& query,QString oneDay);

    // 返回指定日期的记录
    void assReturnRecord(QSqlQuery& query,QString selectedDay);
    // 返回指定周的记录
    void assReturnWeekRecord(QSqlQuery& query,QStringList selectedWeek);

    // 设置日期按钮样式
    void setDayButtonStyle(QPushButton*  dayButton,QDate day);

    // 处理托盘图标激活事件
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    // 提示正在工作
    void tipWorkingNow();
    // 提示未开始工作
    void tipNoWorkingNow();
    // 提示暂停工作
    void tipPausingNow();

private:
    // UI 界面指针
    Ui::assemPage *ui;

    // QWidget 接口，处理鼠标按下事件
protected:
    void mousePressEvent(QMouseEvent *event) override;
    // QWidget 接口，处理鼠标释放事件
    void mouseReleaseEvent(QMouseEvent *event) override;
    // QWidget 接口，处理鼠标移动事件
    void mouseMoveEvent(QMouseEvent *event) override;

    // QWidget 接口，处理窗口关闭事件
protected:
    void closeEvent(QCloseEvent *event) override;
    // QWidget 接口，处理窗口状态变化事件
    void changeEvent(QEvent *event) override;
};
#endif // ASSEMPAGE_H
