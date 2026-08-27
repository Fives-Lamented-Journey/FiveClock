QT       += core gui
QT       +=sql
QT       +=charts
QT       +=multimedia
QT       +=multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
DEFINES += QT_NO_DEBUG_OUTPUT
CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
CONFIG += resources_big

SOURCES += \
    card.cpp \
    chartpage.cpp \
    customcalendarwidget.cpp \
    homePage.cpp \
    infodialog.cpp \
    main.cpp \
    assempage.cpp \
    mybutton.cpp \
    petwindow.cpp \
    splashpage.cpp \
    sqldbm.cpp \
    subpage.cpp

HEADERS += \
    assempage.h \
    card.h \
    chartpage.h \
    customcalendarwidget.h \
    homePage.h \
    infodialog.h \
    mybutton.h \
    petwindow.h \
    splashpage.h \
    sqldbm.h \
    subpage.h

FORMS += \
    assempage.ui \
    card.ui \
    chartpage.ui \
    homePage.ui \
    infodialog.ui \
    petwindow.ui \
    splashpage.ui \
    subpage.ui
TARGET = FiveClock


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    pic.qrc
win32 {
    RC_ICONS = windowsIcon.ico
}
DISTFILES += \
    assemPage.pro.user
