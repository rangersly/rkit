# mypoll

基于 `poll(2)` 的轻量级事件监听池库。单文件实现,C17,无第三方依赖,提供回调式的事件分发,并附带示例程序与 CTest 测试。

## 特性

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
│   └── api.md                 # API 参考文档
├── include/
│   └── mypoll.h               # 公开头文件(extern "C" 守卫)
├── src/
│   ├── mypoll.c               # 库实现
│   └── main.c                 # 示例程序
└── tests/
    └── test.c                 # CTest 测试
```

约定:`include/` 只放公开头文件,`src/` 放实现与示例,`tests/` 放测试;公开头文件统一用 `#ifndef` 防护符 + `extern "C"` 守卫以便 C++ 调用。

## 构建与运行

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

## API 概览

完整说明见 [`docs/api.md`](docs/api.md) 与头文件 [`include/mypoll.h`](include/mypoll.h)。

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

## 代码规范

格式化(需安装 clang-format):

```bash
clang-format -i include/*.h src/*.c tests/*.c
```

静态检查(需安装 clang-tidy;本项目不把 clang-tidy 集成进构建,按需手动运行):

```bash
clang-tidy -p build src/mypoll.c src/main.c tests/test.c
```

### 命名约定速查(Google 风格)

| 元素 | 风格 | 示例 |
|---|---|---|
| 类型 / 结构体 / typedef | `CamelCase` | `Poller` |
| 函数 | `CamelCase` | `PollerCreate` |
| 变量 / 参数 / 成员 | `snake_case` | `user_data`, `nfds` |
| 宏 | `UPPER_CASE` | `POLL_MYPOLL_H_` |
| 文件 | `lower_snake_case` | `mypoll.h` |

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
