# io 模块 API 参考

`rkit` 工具箱中的非阻塞 I/O 模块:对 `read(2)` / `write(2)` 做薄封装,内部自动重试 `EINTR`。

## 概述

`rio.h` 只提供两个函数,不做缓冲、不做 write-all,也不修改 fd 的阻塞模式:

- `IoRead` 发起一次 `read(2)`。
- `IoWrite` 发起一次 `write(2)`。

调用方负责把 fd 设为非阻塞,并处理部分读写与 `EAGAIN` / `EWOULDBLOCK`。

## 头文件与包含

```c
#include "rio.h"
```

公开头文件带 `#ifndef` 防护符与 `extern "C"` 守卫,可直接被 C 与 C++ 调用。

## 返回语义

两个函数都采用 errno 风格:成功返回字节数,失败返回 `-1` 并保留 `errno`。

| 函数 | 返回值 | 含义 |
|---|---|---|
| `IoRead` | `>0` | 实际读到的字节数 |
| `IoRead` | `0` | 对端已关闭(EOF) |
| `IoRead` | `-1` | 读取失败或暂时不可读;`errno` 为 `EAGAIN` / `EWOULDBLOCK` 表示非阻塞 fd 暂无数据 |
| `IoWrite` | `>0` | 实际写入的字节数,可能小于 `len` |
| `IoWrite` | `-1` | 写入失败或暂时不可写;`errno` 为 `EAGAIN` / `EWOULDBLOCK` 表示非阻塞 fd 暂时不可写 |

内部仅对 `EINTR` 自动重试;其余错误原样返回,不会吞掉 `EAGAIN`。

## API 参考

### IoRead

```c
ssize_t IoRead(int fd, void *buf, size_t len);
```

- `fd`:待读取的文件描述符。
- `buf`:接收数据的缓冲区。
- `len`:缓冲区长度。
- 返回:实际读到的字节数;EOF 返回 `0`;失败返回 `-1` 且保留 `errno`。
- 一次调用最多返回 `len` 字节,不做循环读满。

### IoWrite

```c
ssize_t IoWrite(int fd, const void *buf, size_t len);
```

- `fd`:待写入的文件描述符。
- `buf`:待写入的数据。
- `len`:待写入的字节数。
- 返回:实际写入的字节数,可能小于 `len`;失败返回 `-1` 且保留 `errno`。
- 不做 write-all,调用方需自行处理部分写入。

## 与 mypoll 配合

典型用法:非阻塞 fd 暂时不可读/可写时返回 `EAGAIN`,注册到监听池等待事件后重试。

```c
#include "mypoll.h"
#include "rio.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

static void OnReadable(int fd, short revents, void *user_data) {
    (void)revents;
    (void)user_data;
    char buf[256];
    ssize_t n = IoRead(fd, buf, sizeof(buf));
    if (n > 0) {
        /* 处理数据 */
    } else if (n == 0) {
        PollerDel(fd, POLLIN);
        close(fd);
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        PollerDel(fd, POLLIN);
        close(fd);
    }
}

int SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```

## 注意事项

- **不设置 O_NONBLOCK**:封装保持 fd 原有阻塞模式;传入阻塞 fd 时会阻塞。
- **部分读写**:返回值可能小于请求长度,需由调用方累积或重试。
- **EAGAIN 不是错误**:配合 `PollerAdd` 等待可读/可写后再次调用即可。
- **不负责关闭 fd**:fd 的所有权归调用方。
