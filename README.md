# rkit

轻量级 C 工具箱。以模块化方式提供一组可独立使用的通用工具,首个模块是基于 `poll(2)` 的事件监听池 `mypoll`。C17,无第三方依赖,并附带示例程序与 CTest 测试。

## 模块

| 模块 | 公开头文件 | 文档 | 说明 |
|---|---|---|---|
| mypoll | [`include/mypoll.h`](include/mypoll.h) | [`docs/mypoll.md`](docs/mypoll.md) | 基于 `poll(2)` 的事件监听池 |
| io | [`include/rio.h`](include/rio.h) | [`docs/io.md`](docs/io.md) | 非阻塞 `read(2)` / `write(2)` 薄封装 |

新增工具按 [新增工具约定](#新增工具约定) 接入,并在此表登记。

## mypoll 特性

- 基于 `poll(2)` 的事件监听池,注册 `(fd, events)` 组合后由回调统一分发
- 同一个 fd 可用不同 `events` 注册多个监听项
- 就绪事件**快照分发**,回调内可安全地增删监听项
- 出错 fd(`POLLERR`/`POLLHUP`/`POLLNVAL`)自动移出监听池并回传,交由调用方关闭
- C 静态库 + 示例程序 + CTest 测试,内置 clang-format / clang-tidy / clangd 配置

## 目录结构

```text
.
├── .clang-format              # clang-format 风格配置
├── .clang-tidy                # clang-tidy 静态检查配置
├── .clangd                    # clangd 配置(指向 build/ 的编译数据库)
├── CMakeLists.txt             # 根构建文件
├── CONTEXT.md                 # 项目词汇表
├── LICENSE                    # MIT
├── README.md
├── docs/
│   ├── io.md                  # io 模块 API 参考
│   └── mypoll.md              # mypoll 模块 API 参考
├── include/
│   ├── mypoll.h               # mypoll 公开头文件(extern "C" 守卫)
│   └── rio.h                  # io 公开头文件(extern "C" 守卫)
├── src/
│   ├── io.c                   # io 实现
│   ├── mypoll.c               # mypoll 实现
│   └── main.c                 # 示例程序
└── tests/
    ├── test_io.c              # io 的 CTest 测试
    └── test_poll.c            # mypoll 的 CTest 测试
```

约定:`include/` 只放公开头文件,`src/` 放实现与示例,`tests/` 放测试;公开头文件统一用 `#ifndef` 防护符 + `extern "C"` 守卫以便 C++ 调用。

## 构建与运行

需要 CMake 3.21 及以上。

```bash
# 配置(默认 Release)
cmake -B build -S .

# 构建
cmake --build build

# 运行示例程序
./build/app

# 运行测试
ctest --test-dir build
```

### 常用选项

```bash
# Debug 构建
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# 关闭测试
cmake -B build -S . -DBUILD_TESTING=OFF

# 只构建某个目标
cmake --build build --target app
```

## 快速上手

从标准输入读取并回显,遇到出错 fd 则关闭退出:

```c
#include "mypoll.h"

#include <stdio.h>
#include <unistd.h>

static void OnEvent(int fd, short revents, void *user_data) {
    (void)user_data;
    char buf[256];
    if (revents & POLLIN) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            write(STDOUT_FILENO, buf, (size_t)n);
        }
    }
}

int main(void) {
    if (PollerCreate(16) != 0) {
        return 1;
    }
    if (PollerAdd(STDIN_FILENO, POLLIN, NULL) != 0) {
        PollerDestory();
        return 1;
    }

    for (;;) {
        int err_fd = -1;
        int ret = PollerRun(-1, OnEvent, &err_fd);
        if (ret == 1) {  /* 有 fd 出错,已移出监听池 */
            close(err_fd);
            break;
        }
        if (ret < 0) {
            break;
        }
    }

    PollerDestory();
    return 0;
}
```

## 作为库使用(FetchContent)

消费方只需在 CMake 中声明本仓库的本地路径,即可链接 `rkit::rkit`。本仓库被引入时只构建核心静态库,不会构建示例程序 `app` 或测试。

### 消费方 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.21)
project(myapp LANGUAGES C)

include(FetchContent)
FetchContent_Declare(rkit
    # 本地仓库绝对路径，换成你自己的路径
    GIT_REPOSITORY /home/rangersly/poll
    # 使用本地仓库的 main 分支；需要固定版本时改成 commit hash
    GIT_TAG main
)
FetchContent_MakeAvailable(rkit)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE rkit::rkit)
```

### 消费方 main.c

```c
#include <mypoll.h>

int main(void) {
    if (PollerCreate(16) != 0) {
        return 1;
    }
    PollerDestory();
    return 0;
}
```

### 配置与构建

```bash
cmake -S . -B build
cmake --build build
./build/myapp
```

`GIT_REPOSITORY` 指向本地仓库时不会联网，但它执行的是 `git clone`，只能看到**已提交**的内容。如果本仓库还有未提交的改动，消费方拿到的仍是上一次提交的版本。需要先提交；或者临时用下面的参数直接指向工作区：

```bash
cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_RKIT=/home/rangersly/poll
```

确认消费方没有构建示例和测试：

```bash
cmake --build build --target help
```

目标列表中应只有 `myapp`、`rkit` 等，不应出现 `app`、`test_poll` 或 `test_io`。

### 固定版本

本地开发可以用 `main`，需要可复现构建时改成提交哈希：

```cmake
GIT_TAG <commit-hash>
```

以后把仓库推到远端时，只需替换地址和 tag：

```cmake
GIT_REPOSITORY git@github.com:you/rkit.git
GIT_TAG v2.1.0
```

### 更新本地依赖

修改本仓库并提交后，如果消费方没有自动更新，删除缓存再重新配置：

```bash
rm -rf build/_deps/rkit-*
cmake -S . -B build
```

## mypoll API 概览

完整说明见 [`docs/mypoll.md`](docs/mypoll.md) 与头文件 [`include/mypoll.h`](include/mypoll.h)。

| 接口 | 说明 |
|---|---|
| `int PollerCreate(nfds_t max_fds)` | 创建(或重建)全局监听池 |
| `void PollerDestory(void)` | 释放监听池,可重复调用 |
| `int PollerAdd(int fd, short events, void *user_data)` | 注册一个 `(fd, events)` 监听项 |
| `int PollerDel(int fd, short events)` | 按 `(fd, events)` 注销监听项 |
| `int PollerRun(int timeout_ms, poller_cb callback, int *err_fd)` | 等待并分发就绪事件 |

### 返回码

| 函数 | 返回码 | 含义 |
|---|---|---|
| `PollerCreate` | `-1` | `max_fds` 为 0 或内存申请失败 |
| `PollerAdd` | `-1` / `-2` / `-3` | 未创建 / 已满 / 已存在相同 `(fd, events)` |
| `PollerDel` | `-1` / `-2` | 未创建 / 未找到匹配项 |
| `PollerRun` | `-1` / `-2` / `1` | 未创建或回调为空 / `poll` 出错 / 存在出错 fd |

## io API 概览

完整说明见 [`docs/io.md`](docs/io.md) 与头文件 [`include/rio.h`](include/rio.h)。

| 接口 | 说明 |
|---|---|
| `ssize_t IoRead(int fd, void *buf, size_t len)` | 单次 `read(2)`,EINTR 自动重试 |
| `ssize_t IoWrite(int fd, const void *buf, size_t len)` | 单次 `write(2)`,EINTR 自动重试 |

成功返回字节数,失败返回 `-1` 并保留 `errno`;`errno` 为 `EAGAIN` / `EWOULDBLOCK` 表示非阻塞 fd 暂时不可读/可写。封装不设置 `O_NONBLOCK`,也不保证读满或写完。

## 新增工具约定

新增一个工具时,按以下模板接入:

| 项目 | 约定 |
|---|---|
| 公开头文件 | 简单工具:`include/r<tool>.h`(如 `rarray.h`);体量大时:`include/<tool>/` 多文件 |
| 头文件守卫 | `RKIT_<TOOL>_H_` |
| extern "C" | 公开头文件统一加守卫 |
| 符号前缀 | 语义前缀,如 `Array_` / `Queue_` / `Map_` / `Log_` / `Pool_` |
| 实现 | `src/<tool>.c` |
| 测试 | `tests/test_<tool>.c` + CTest 目标 `test_<tool>` |
| 文档 | `docs/<tool>.md`,并在上方[模块](#模块)表登记 |
| 构建 | 显式加入 `rkit` 目标源列表 |

## 代码规范

格式化(需安装 clang-format):

```bash
clang-format -i include/*.h src/*.c tests/*.c
```

静态检查(需安装 clang-tidy;本项目不把 clang-tidy 集成进构建,按需手动运行):

```bash
clang-tidy -p build src/mypoll.c src/io.c src/main.c tests/test_poll.c tests/test_io.c
```

### 命名约定速查(Google 风格)

| 元素 | 风格 | 示例 |
|---|---|---|
| 类型 / 结构体 / typedef | `CamelCase` | `Poller` |
| 函数 | `CamelCase` | `PollerCreate` |
| 变量 / 参数 / 成员 | `snake_case` | `user_data`, `nfds` |
| 宏 | `UPPER_CASE` | `POLL_MYPOLL_H_`, `RKIT_ARRAY_H_` |
| 文件 | `lower_snake_case` | `mypoll.h`, `rarray.h` |

具体规则以 [`.clang-tidy`](.clang-tidy) 与 [`.clang-format`](.clang-format) 为准。

## 设计约束

- **全局单例**:监听池持有全局静态状态,同一进程内仅一个;`PollerCreate` 会覆盖式重建。
- **非线程安全**:多线程使用需自行加锁。
- **fd 所有权**:库只维护监听关系,不负责打开或关闭 fd;关闭前应先 `PollerDel`,或依赖 `PollerRun` 的自动移除。

## 编辑器支持(clangd)

本仓库自带 `.clangd`,让 clangd 到 `build/` 读取 `compile_commands.json`,以正确解析 `include/` 与 `src/` 的头文件路径。先生成编译数据库即可:

```bash
cmake -B build -S .
```

## 许可证

[MIT](LICENSE)
