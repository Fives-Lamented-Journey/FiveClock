cmake_minimum_required(VERSION 3.16)

project(FiveClock LANGUAGES CXX)

# Qt自动工具：MOC、UIC、RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 对应pro：DEFINES += QT_NO_DEBUG_OUTPUT
add_compile_definitions(QT_NO_DEBUG_OUTPUT)

# 寻找Qt6全部需要的组件
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Charts
    Multimedia
    MultimediaWidgets
)

# 源文件，完全复制你pro里的 SOURCES
add_executable(FiveClock

    # cpp源文件
    card.cpp
    chartpage.cpp
    customcalendarwidget.cpp
    homePage.cpp
    infodialog.cpp
    main.cpp
    assempage.cpp
    mybutton.cpp
    petwindow.cpp
    splashpage.cpp
    sqldbm.cpp
    subpage.cpp

    # 头文件（CMake里HEADERS不是必须，但写上方便IDE索引）
    assempage.h
    card.h
    chartpage.h
    customcalendarwidget.h
    homePage.h
    infodialog.h
    mybutton.h
    petwindow.h
    splashpage.h
    sqldbm.h
    subpage.h

    # UI表单 FORMS
    assempage.ui
    card.ui
    chartpage.ui
    homePage.ui
    infodialog.ui
    petwindow.ui
    splashpage.ui
    subpage.ui

    # 资源文件 RESOURCES += pic.qrc
    pic.qrc
)

# Windows程序图标，对应 pro win32 { RC_ICONS = windowsIcon.ico }
if(WIN32)
    target_sources(FiveClock PRIVATE windowsIcon.ico)
endif()

# 链接Qt6模块
target_link_libraries(FiveClock
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)

# 安装规则，和原pro保持一致
install(TARGETS FiveClock
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
