cmake_minimum_required(VERSION 3.21)

project(rkit
    VERSION 2.0.0
    DESCRIPTION "轻量级 C 工具箱,包含 poll(2) 事件监听池等模块"
    LANGUAGES C
)

# ============================================================
# 1. 核心库目标
#    无论本仓库作为顶层项目,还是被其他项目通过
#    FetchContent / add_subdirectory 引入,都只构建这一部分。
# ============================================================

add_library(rkit STATIC
    src/mypoll.c
)

# 对外统一使用 rkit::rkit 这个名字链接,
# 避免消费方直接依赖裸目标名 rkit。
add_library(rkit::rkit ALIAS rkit)

# include/ 是公开接口,BUILD_INTERFACE 表示只在构建树内生效;
# src/ 只给库自身使用,不对外暴露。
target_include_directories(rkit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 这些属性只作用于 rkit 目标本身,不会传播给消费方。
# C_STANDARD:用 C17 编译库源码
# C_STANDARD_REQUIRED:编译器不支持 C17 时直接报错
# POSITION_INDEPENDENT_CODE:静态库带 -fPIC,方便被链进消费方的共享库
set_target_properties(rkit PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# 只给库自身开警告,不污染消费方。
target_compile_options(rkit PRIVATE
    -Wall -Wextra
)

# ============================================================
# 2. 仅顶层项目才构建的内容
#    被其他项目引入时 PROJECT_IS_TOP_LEVEL 为 false,
#    下面整块都不会执行:
#    - 不构建示例程序 app
#    - 不构建测试 test_poll
#    - 不引入 CTest
#    - 不修改消费方的构建类型和输出目录
# ============================================================

if(PROJECT_IS_TOP_LEVEL)
    # 默认 Release,可用 -DCMAKE_BUILD_TYPE=Debug 覆盖。
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING
            "构建类型(Debug/Release/MinSizeRel/RelWithDebInfo)" FORCE)
    endif()

    # 示例和测试也使用 C17。
    set(CMAKE_C_STANDARD 17)
    set(CMAKE_C_STANDARD_REQUIRED ON)

    # 供 clangd / clang-tidy 使用。
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    # 生成物统一输出到构建根目录。
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

    # --------------------------------------------------------
    # 2.1 示例程序
    # --------------------------------------------------------
    add_executable(app
        src/main.c
    )
    target_compile_options(app PRIVATE -Wall -Wextra)
    target_link_libraries(app
        PRIVATE
            rkit
    )

    # --------------------------------------------------------
    # 2.2 测试
    #     BUILD_TESTING 由 include(CTest) 提供,默认 ON;
    #     可用 -DBUILD_TESTING=OFF 关闭。
    # --------------------------------------------------------
    include(CTest)
    if(BUILD_TESTING)
        # 每个工具一个测试可执行目标,避免单测程序随模块增多而膨胀。
        add_executable(test_poll
            tests/test_poll.c
        )
        target_compile_options(test_poll PRIVATE -Wall -Wextra)
        target_link_libraries(test_poll
            PRIVATE
                rkit
        )
        add_test(NAME test_poll COMMAND test_poll)
    endif()
endif()
