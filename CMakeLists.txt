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
#add_compile_definitions(QT_NO_DEBUG_OUTPUT)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(QT_NO_DEBUG_OUTPUT)
endif()

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

add_subdirectory(libfivecore)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog REQUIRED)
# ================================================================
# 主应用 FiveClock
# ================================================================
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
    src/pages/backuplogdialog.cpp

    # cpp源文件 - widgets
    src/widgets/customcalendarwidget.cpp
    src/widgets/mybutton.cpp

    # cpp源文件 - db
    src/db/sqldbm.cpp

    # cpp源文件 - service 业务服务层
    src/service/record_service.cpp
    src/service/timer_service.cpp
    src/service/notification_service.cpp
    src/service/backup_service.cpp


    # cpp源文件 - config 配置服务
    src/config/config_service.cpp

    # cpp源文件 - timer 计时模型
    src/timer/timermodel.cpp

    # 头文件
    include/app/assempage.h
    include/app/splashpage.h
    include/app/petwindow.h
    include/pages/card.h
    include/pages/chartpage.h
    include/pages/homePage.h
    include/pages/infodialog.h
    include/pages/subpage.h
    include/pages/backuplogdialog.h

    include/widgets/customcalendarwidget.h
    include/widgets/mybutton.h
    include/db/idb_access.h
    include/db/sqldbm.h

    include/config/config_service.h

    include/service/record_service.h
    include/service/timer_service.h
    include/service/notification_service.h
    include/service/backup_service.h

    include/timer/timermodel.h
    include/dto/time_record.h

    # UI表单
    src/ui/assempage.ui
    src/ui/petwindow.ui
    src/ui/splashpage.ui
    src/ui/card.ui
    src/ui/chartpage.ui
    src/ui/homePage.ui
    src/ui/infodialog.ui
    src/ui/subpage.ui
    src/ui/backuplogdialog.ui

    # 资源文件
    pic.qrc
    style.qrc

)

target_include_directories(FiveClock PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(FiveClock PROPERTIES
    AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

if(WIN32)
    target_sources(FiveClock PRIVATE windowsIcon.ico)
endif()

target_link_libraries(FiveClock
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
    Qt6::Multimedia
    Qt6::MultimediaWidgets

    fivecore
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

install(TARGETS FiveClock
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ================================================================
# 单元测试 FiveClockTests（GTest + GMock）
# ================================================================
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    # 使用本地安装的 GTest/GMock
    # 安装路径：G:\cppsoft\googletest-1.14.0-install
    set(GTest_DIR "G:/cppsoft/googletest-1.14.0-install/lib/cmake/GTest" CACHE PATH "GTest cmake config")
    find_package(GTest CONFIG REQUIRED)
    find_package(Qt6 REQUIRED COMPONENTS Test)

    # 测试可执行文件：仅编译服务层+数据层源文件（不含UI）
    add_executable(FiveClockTests
        # 服务层
        src/service/record_service.cpp
        src/service/timer_service.cpp
        src/service/notification_service.cpp
        src/service/backup_service.cpp

        # 数据层
        src/db/sqldbm.cpp
        # 计时模型
        src/timer/timermodel.cpp
        # 配置服务
        src/config/config_service.cpp

        # 【新增】libfivecore单元测试
        test/test_fivecore_util.cpp

        # 测试用例
        test/test_record_service.cpp
        test/test_timer_service.cpp


        # QObject 头文件（确保 AUTOMOC 处理 MOC 代码生成）
        include/db/idb_access.h
        include/db/sqldbm.h
        include/service/record_service.h
        include/service/timer_service.h
        include/service/notification_service.h
        include/service/backup_service.h

        include/timer/timermodel.h
        include/config/config_service.h
        include/dto/time_record.h
        test/mock_db_access.h

    )

    # 测试目标覆盖 QT_NO_DEBUG_OUTPUT（允许 qDebug 输出用于诊断）
    target_compile_definitions(FiveClockTests PRIVATE
        QT_TEST
    )
    # 测试目标：取消 QT_NO_DEBUG_OUTPUT 限制
    target_compile_options(FiveClockTests PRIVATE -UQT_NO_DEBUG_OUTPUT)

    target_include_directories(FiveClockTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )

    target_link_libraries(FiveClockTests
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
        Qt6::Test

        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main

        fivecore
        spdlog::spdlog
    )

    # 启用自动测试发现
    enable_testing()
    add_test(NAME FiveClockTests COMMAND FiveClockTests)
endif()
