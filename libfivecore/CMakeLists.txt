cmake_minimum_required(VERSION 3.20)
project(libfivecore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(fivecore STATIC
        src/util/string_utils.cpp
        src/util/sha256_utils.cpp
)

target_include_directories(fivecore
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
