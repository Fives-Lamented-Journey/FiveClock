cmake_minimum_required(VERSION 3.20)
project(libfivecore)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 依赖搜索提示：仅当这些本机目录存在时才生效；其他机器请用 -DCMAKE_PREFIX_PATH 或 vcpkg 提供依赖
if(EXISTS "G:/cppsoft/curl-8.22.0_1-win64-mingw")
    set(CURL_ROOT "G:/cppsoft/curl-8.22.0_1-win64-mingw")
endif()
if(EXISTS "C:/msys64/ucrt64")
    set(OPENSSL_ROOT_DIR "C:/msys64/ucrt64")
endif()
if(EXISTS "G:/cppsoft/googletest-1.14.0-install/lib/cmake/GTest")
    set(GTest_DIR "G:/cppsoft/googletest-1.14.0-install/lib/cmake/GTest" CACHE PATH "GTest cmake config")
endif()

# 只查找一次GTest，CONFIG模式
find_package(GTest CONFIG REQUIRED)

add_library(fivecore STATIC
        src/fileutil.cpp
        src/hash.cpp
        src/http.cpp
        include/fivecore/result.h
        src/http_async.cpp
        src/INetTask.cpp
        src/CurlMultiContext.cpp
        src/HttpGetTask.cpp
        src/HttpPostTask.cpp
        src/HttpPutStreamTask.cpp
        include/fivecore/HttpGetstreamTask.h
        src/HttpGetStreamTask.cpp
        include/fivecore/TaskScheduler.h
        src/TaskScheduler.cpp
)

target_include_directories(fivecore
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(OpenSSL REQUIRED)
target_link_libraries(fivecore PUBLIC OpenSSL::Crypto)

find_package(CURL REQUIRED)
target_link_libraries(fivecore PUBLIC CURL::libcurl)

find_package(nlohmann_json CONFIG REQUIRED)


enable_testing()

add_executable(fivecore_unit_test
        tests/unit_hash.cpp
        tests/unit_http.cpp
        tests/unit_result.cpp
        tests/unit_async_http.cpp
        tests/unit_async_int.cpp
        tests/unit_task_scheduler.cpp
)


target_link_libraries(fivecore_unit_test
        PRIVATE
        fivecore
        GTest::gtest
        GTest::gtest_main
        nlohmann_json::nlohmann_json
)

if(WIN32)
    # unit_task_scheduler 的 LocalProbe 使用 winsock2
    target_link_libraries(fivecore_unit_test PRIVATE ws2_32)
endif()

target_compile_definitions(fivecore_unit_test PRIVATE HTTP_INTEGRATION_TEST)

include(GoogleTest)
gtest_discover_tests(fivecore_unit_test)



