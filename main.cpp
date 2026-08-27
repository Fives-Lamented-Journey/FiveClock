#include "assempage.h"

#include <QApplication>
#include <QPropertyAnimation>
#include "splashpage.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    splashPage* splash = new splashPage();
    assemPage w;
    QPropertyAnimation *fadeInAnimation = new QPropertyAnimation(&w, "windowOpacity", qApp);
    fadeInAnimation->setDuration(500);
    fadeInAnimation->setStartValue(0.0);
    fadeInAnimation->setEndValue(1.0);

    QObject::connect(splash, &splashPage::aboutToClose, [&w, fadeInAnimation]() {
        w.setWindowState(w.windowState() | Qt::WindowFullScreen);
        w.show();
        fadeInAnimation->start();
    });

    splash->splashShow();

    qDebug()<<"w.show()";
    return a.exec();
}
