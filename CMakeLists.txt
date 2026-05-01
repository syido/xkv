cmake_minimum_required(VERSION 3.16)

project(xkv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 本项目使用vcpkg管理依赖，在此检查
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "vcpkg工具链未定义")
endif()

# 测试入口
set(XkvTest "xkv_test")
file(
    GLOB_RECURSE XkvTestSource
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"
)

add_library(
    ${XkvTest} SHARED
    ${XkvTestSource}
)

target_include_directories(
    ${XkvTest} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# find_package(figcone CONFIG REQUIRED)

set_target_properties(
    ${XkvTest} PROPERTIES
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)
