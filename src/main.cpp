#include "app/assempage.h"
#include "app/splashpage.h"

#include <QApplication>
#include <QPropertyAnimation>

// ================================================================
// 启动流程（B-14e：以具名函数封装，main 只保留骨架顺序）
// 启动时序：创建闪屏 → 创建主窗口 → 主窗口淡入动画 → 连接启动衔接 → 显示闪屏 → 进入事件循环
// ================================================================

namespace {

// 创建主窗口淡入动画
// 注意：parent 为 qApp，动画对象与进程同生命周期（原实现如此，保持不变）
QPropertyAnimation *createFadeInAnimation(assemPage &mainWindow)
{
    QPropertyAnimation *fadeIn = new QPropertyAnimation(&mainWindow, "windowOpacity", qApp);
    fadeIn->setDuration(500);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    return fadeIn;
}

// 连接启动衔接：闪屏 5 秒计时结束（aboutToClose）→ 主窗口全屏显示并淡入
// 生命周期说明：
//   - splash 为 heap 分配 + WA_DeleteOnClose（见 splashpage 构造函数），淡出动画结束后自动析构，禁止改为栈对象
//   - mainWindow 为 main 栈对象，存活至事件循环结束，lambda 按引用捕获因此安全
//   - fadeIn 指针按值捕获，其对象由 qApp 管理，生命周期覆盖整个启动流程
void connectStartupFlow(splashPage *splash, assemPage &mainWindow, QPropertyAnimation *fadeIn)
{
    QObject::connect(splash, &splashPage::aboutToClose, [&mainWindow, fadeIn]() {
        mainWindow.setWindowState(mainWindow.windowState() | Qt::WindowFullScreen);
        mainWindow.show();
        fadeIn->start();
    });
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 闪屏（heap + WA_DeleteOnClose 自动释放）
    splashPage *splash = new splashPage();

    // 主窗口（main 栈对象，存活至事件循环结束）
    assemPage w;

    // 启动衔接：动画 + 闪屏关闭 → 主窗口显示
    QPropertyAnimation *fadeIn = createFadeInAnimation(w);
    connectStartupFlow(splash, w, fadeIn);

    splash->splashShow();

    qDebug() << "w.show()";
    return a.exec();
}
