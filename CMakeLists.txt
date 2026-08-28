# 供 clangd 等语言服务使用（配合根目录 .clangd 指向 build）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
# 目录分层：src/ 实现与表单，include/ 公共头文件（与 src 平级）
# 分层子目录：app 主窗口与全局窗口，pages 业务页面，widgets 自定义控件，db 数据层
add_executable(FiveClock

    # 入口
    src/main.cpp

    # cpp源文件 - app
    src/app/assempage.cpp
    src/app/splashpage.cpp
    src/app/petwindow.cpp

    # cpp源文件 - pages
    src/pages/card.cpp
    src/pages/chartpage.cpp
    src/pages/homePage.cpp
    src/pages/infodialog.cpp
    src/pages/subpage.cpp

    # cpp源文件 - widgets
    src/widgets/customcalendarwidget.cpp
    src/widgets/mybutton.cpp

    # cpp源文件 - db
    src/db/sqldbm.cpp

    # cpp源文件 - service 业务服务层
    src/service/record_service.cpp

    # cpp源文件 - timer 计时模型
    src/timer/timermodel.cpp

    # 头文件（CMake里HEADERS不是必须，但写上方便IDE索引）
    include/app/assempage.h
    include/app/splashpage.h
    include/app/petwindow.h
    include/pages/card.h
    include/pages/chartpage.h
    include/pages/homePage.h
    include/pages/infodialog.h
    include/pages/subpage.h
    include/widgets/customcalendarwidget.h
    include/widgets/mybutton.h
    include/db/sqldbm.h
    include/service/record_service.h
    include/timer/timermodel.h

    # UI表单 FORMS（AUTOUIC 对 .ui 位置不敏感，生成的 ui_*.h 进入 autogen 全局 include 目录）
    src/ui/assempage.ui
    src/ui/petwindow.ui
    src/ui/splashpage.ui
    src/ui/card.ui
    src/ui/chartpage.ui
    src/ui/homePage.ui
    src/ui/infodialog.ui
    src/ui/subpage.ui

    # 资源文件 RESOURCES += pic.qrc（相对路径引用 res/，保持项目根目录）
    pic.qrc
)

# 源码按分层路径互相 include（#include "app/xxx.h" / "pages/xxx.h" / "widgets/xxx.h" / "db/xxx.h"），
# 将公共头文件根目录加入搜索路径即可
target_include_directories(FiveClock PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# .ui 已集中到 src/ui，AUTOUIC 解析 cpp 中的 ui_*.h include 时
# 除 cpp 同目录外还需在此目录查找 .ui 源文件
set_target_properties(FiveClock PROPERTIES
    AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
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
