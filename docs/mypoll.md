# mypoll 模块 API 参考

`rkit` 工具箱中的事件监听池模块：基于 `poll(2)` 的轻量级事件监听池封装（C17，无第三方依赖）。

## 概述

`mypoll` 提供一个进程级的全局事件循环：

- 用 `PollerCreate` 创建监听池并设定最大容量。
- 用 `PollerAdd` 注册待监听的 `(fd, events)` 组合。
- 用 `PollerRun` 阻塞等待，并按回调分发就绪事件。
- 用 `PollerDel` 注销单个监听项。
- 用 `PollerDestory` 释放全部资源。

同一进程内只存在一个监听池，`PollerCreate` 会覆盖式重建。库不带锁，**非线程安全**。

## 头文件与包含

```c
#include "mypoll.h"
```

公开头文件带 `#ifndef` 防护符与 `extern "C"` 守卫，可直接被 C 与 C++ 调用。

## 错误码汇总

| 函数 | 返回值 | 含义 |
|---|---|---|
| 通用 | `0` | 成功 |
| `PollerCreate` | `-1` | `max_fds` 为 0 或内存申请失败 |
| `PollerAdd` | `-1` | 监听池未创建 |
| `PollerAdd` | `-2` | 监听池已满 |
| `PollerAdd` | `-3` | 已存在完全相同的 `(fd, events)` 项 |
| `PollerDel` | `-1` | 监听池未创建 |
| `PollerDel` | `-2` | 未找到匹配的 `(fd, events)` 项 |
| `PollerRun` | `-1` | 监听池未创建，或 `callback` 为 `NULL` |
| `PollerRun` | `-2` | `poll(2)` 调用出错 |
| `PollerRun` | `1` | 存在出错 fd，已移出监听池并经 `err_fd` 返回 |
| `PollerRun` | `-3` | 回调期间监听池被 `PollerDestory` 销毁或被 `PollerCreate` 重建 |

## 数据结构

```c
typedef void (*poller_cb)(int fd, short revents, void *user_data);
```

事件就绪回调：

| 参数 | 说明 |
|---|---|
| `fd` | 触发事件的监听文件描述符 |
| `revents` | 就绪事件位掩码，可能含 `POLLIN`、`POLLOUT`、`POLLERR`、`POLLHUP`、`POLLNVAL` 等 |
| `user_data` | 注册该 `(fd, events)` 时传入的用户指针，可为 `NULL` |

## API 参考

### PollerCreate

```c
int PollerCreate(nfds_t max_fds);
```

创建（或重建）全局监听池。

- `max_fds`：允许注册的最大 `(fd, events)` 项数，建议不超过 1024；传 0 非法。
- 返回：成功 `0`；`max_fds` 为 0 或内存申请失败返回 `-1`。
- 若监听池已存在，会先释放旧池再重建。

### PollerDestory

```c
void PollerDestory(void);
```

注销全局监听池并释放其占用的内存。

- 可重复调用；调用后可用 `PollerCreate` 重新创建。
- 仅释放监听池自身资源，**不会**关闭任何已注册的 fd。

### PollerAdd

```c
int PollerAdd(int fd, short events, void *user_data);
```

向监听池注册一个待监听的 `(fd, events)` 组合。

- `fd`：监听的文件描述符。
- `events`：监听的事件，为 `POLLIN`、`POLLOUT` 的按位或。
- `user_data`：事件触发时原样回传给回调的用户指针，可为 `NULL`。
- 返回：成功 `0`；未创建 `-1`；已满 `-2`；已存在完全相同的 `(fd, events)` 返回 `-3`。
- 同一个 fd 可用不同 `events` 重复注册；但 `(fd, events)` 组合必须唯一。

### PollerDel

```c
int PollerDel(int fd, short events);
```

注销监听池中与 `(fd, events)` 完全匹配的项。

- `fd`：要注销的文件描述符。
- `events`：注册该项时使用的 events，必须完全一致。
- 返回：成功 `0`；未创建 `-1`；未找到匹配项 `-2`。
- 删除采用末尾元素填补，不保证剩余项的顺序。

### PollerRun

```c
int PollerRun(int timeout_ms, poller_cb callback, int *err_fd);
```

等待并分发监听池中已就绪的事件。

- `timeout_ms`：等待超时（毫秒），`-1` 表示无限等待。
- `callback`：事件就绪回调，不可为 `NULL`。
- `err_fd`：出错 fd 的输出参数，可为 `NULL`；仅在返回 `1` 时写入。
- 返回：正常 `0`；未创建或 `callback` 为 `NULL` 返回 `-1`；`poll(2)` 出错返回 `-2`；存在出错 fd 返回 `1`；回调内销毁或重建监听池返回 `-3`。
- 本轮就绪事件在调用前已快照，回调执行期间可安全地调用 `PollerAdd` / `PollerDel`。
- 若在回调内调用 `PollerDestory`（或 `PollerCreate` 重建），监听池状态相对本轮开始时已改变，剩余就绪项不再分发，`PollerRun` 立即返回 `-3`；调用方应停止使用并按需重新 `PollerCreate`。
- 出错项（`revents` 含 `POLLERR/POLLHUP/POLLNVAL`）会被自动移出监听池并经 `err_fd` 返回，调用方需自行关闭该 fd；若同一 fd 注册了多项，仅移出事发的该项。
- 当 `revents` 同时包含普通事件与错误标志时，先回调再返回错误。

## 使用示例

从标准输入回显，遇到出错 fd 则关闭退出：

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
        if (ret == 1) {
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

编译（假设已构建静态库 `librkit.a`）：

```bash
cc -Iinclude example.c -Lbuild -lrkit -o example
```

## 注意事项

- **单例**：全局静态状态，同进程仅一个监听池，`PollerCreate` 会覆盖重建。
- **非线程安全**：多线程使用需自行加锁。
- **fd 所有权**：库只管理监听关系，不负责打开或关闭 fd；关闭 fd 前应先 `PollerDel`（或依赖 `PollerRun` 的自动移除）。
- **重复 fd**：允许同 fd 多 `events`，以 `(fd, events)` 为唯一键区分。
