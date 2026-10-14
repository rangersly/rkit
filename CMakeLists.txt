cmake_minimum_required(VERSION 3.20)

project(mypoll
    VERSION 1.0.0
    DESCRIPTION "基于 poll(2) 的事件监听池"
    LANGUAGES C
)

# ============================================================
# 全局设置
# ============================================================

# 默认构建类型为 Release,可用 -DCMAKE_BUILD_TYPE=Debug 覆盖
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "构建类型(Debug/Release/MinSizeRel/RelWithDebInfo)" FORCE)
endif()

# C 标准
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 导出 compile_commands.json,供 clangd / clang-tidy 使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 生成物统一输出到构建根目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# 通用警告选项
add_compile_options(-Wall -Wextra)

# ============================================================
# 静态库:C 实现,对外公开 include/ 下的头文件
# ============================================================
add_library(mypoll STATIC
    src/mypoll.c
)

target_include_directories(mypoll
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ============================================================
# 可执行程序:示例程序,链接 mypoll 静态库
# ============================================================
add_executable(app
    src/main.c
)

target_link_libraries(app
    PRIVATE
        mypoll
)

# ============================================================
# 测试:CTest,可用 -DBUILD_TESTING=OFF 关闭
# ============================================================
include(CTest)

if(BUILD_TESTING)
    add_executable(tests
        tests/test.c
    )
    target_link_libraries(tests
        PRIVATE
            mypoll
    )
    add_test(NAME tests COMMAND tests)
endif()
