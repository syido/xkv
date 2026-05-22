cmake_minimum_required(VERSION 3.16)

project(xkv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 本项目使用vcpkg管理依赖，在此检查
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "vcpkg工具链未定义")
endif()


# 测试入口: xkv_core
set(XkvCore "xkv_core")
file(
    GLOB_RECURSE XkvCoreSource
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"
)

add_library(${XkvCore} STATIC ${XkvCoreSource})

target_include_directories(
    ${XkvCore} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(reflectcpp CONFIG REQUIRED)
target_link_libraries(${XkvCore} PRIVATE reflectcpp::reflectcpp)

# 跨平台
if(WIN32)
    target_link_libraries(${XkvCore} PUBLIC ws2_32)
endif()

set_target_properties(
    ${XkvCore} PROPERTIES
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)


# 命令行: xkv_cli
set(XkvCli "xkv_cli")
file(
    GLOB_RECURSE XkvCliSource
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cli/*.cpp"
)

add_executable(${XkvCli} ${XkvCliSource})
target_link_libraries(${XkvCli} PRIVATE ${XkvCore})

target_include_directories(
    ${XkvCli} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Asio依赖库
find_package(asio CONFIG REQUIRED)
target_link_libraries(${XkvCli} PRIVATE asio::asio)

set_target_properties(
    ${XkvCli} PROPERTIES
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)
