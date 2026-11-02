# serial 模块 API 参考

`rkit` 工具箱中的串口模块:以非阻塞方式打开串口并完成原始模式 8N1、无流控配置。

## 概述

`rserial.h` 提供 `SerialOpen`,把"打开串口设备 + 读取旧配置 + 设置原始模式 + 设置波特率"的常见流程封装成一次调用。返回的 fd 可直接配合 `rio.h` 的 `IoRead` / `IoWrite` 以及 `mypoll.h` 的监听池使用。

## 头文件与包含

```c
#include "rserial.h"
```

公开头文件带 `#ifndef` 防护符与 `extern "C"` 守卫,可直接被 C 与 C++ 调用。

## API 参考

### SerialOpen

```c
int SerialOpen(const char *path, int baud);
```

打开并配置串口,成功返回文件描述符。

- `path`:串口设备路径,如 `"/dev/ttyUSB0"`、`"/dev/ttyS0"`;不可为 `NULL`。
- `baud`:整数波特率,必须是下表中的值,且与对端设备一致。
- 返回:成功返回 `>=0` 的 fd;失败返回 `-1`,并保留真实 `errno`。

支持的整数波特率与 termios 常量对应关系:

| 整数 `baud` | termios 常量 |
|---|---|
| `9600` | `B9600` |
| `19200` | `B19200` |
| `38400` | `B38400` |
| `57600` | `B57600` |
| `115200` | `B115200` |

传入不支持的整数(如 `0`、负数或表中未列出的值)时,函数在打开设备前即以 `errno = EINVAL` 失败返回,不会产生 fd。

内部行为:

| 步骤 | 说明 |
|---|---|
| 打开 | `O_RDWR \| O_NOCTTY \| O_NONBLOCK \| O_CLOEXEC` |
| 读取配置 | `tcgetattr`,失败即中止 |
| 原始模式 | `cfmakeraw`,关闭回显、行缓冲、信号字符与换行转换 |
| 8N1 | 清除 `CSIZE / PARENB / CSTOPB / CRTSCTS`,设置 `CS8 / CLOCAL / CREAD` |
| 波特率 | 整数映射为 `B*` 常量后,`cfsetispeed` / `cfsetospeed` 都必须成功 |
| 生效 | `tcsetattr(TCSANOW)` |
| 清理 | `tcflush(TCIFLUSH)` 丢弃打开前残留输入 |

失败时统一关闭已打开的 fd 并恢复 `errno`。

## 使用示例

```c
#include "mypoll.h"
#include "rio.h"
#include "rserial.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void OnReadable(int fd, short revents, void *user_data) {
    (void)revents;
    (void)user_data;
    char buf[256];
    ssize_t n = IoRead(fd, buf, sizeof(buf));
    if (n > 0) {
        write(STDOUT_FILENO, buf, (size_t)n);
    }
}

int main(void) {
    int fd = SerialOpen("/dev/ttyUSB0", 115200);
    if (fd < 0) {
        fprintf(stderr, "open serial failed: %s\n", strerror(errno));
        return 1;
    }
    if (PollerCreate(16) != 0 || PollerAdd(fd, POLLIN, NULL) != 0) {
        close(fd);
        PollerDestory();
        return 1;
    }

    for (;;) {
        if (PollerRun(-1, OnReadable, NULL) != 0) {
            break;
        }
    }

    PollerDestory();
    close(fd);
    return 0;
}
```

编译(假设已构建静态库 `librkit.a`):

```bash
cc -Iinclude example.c -Lbuild -lrkit -o example
```

## 注意事项

- **非阻塞**:返回的 fd 已带 `O_NONBLOCK`,读写可能返回 `EAGAIN` / `EWOULDBLOCK`;通常配合监听池等待可读/可写。
- **fd 所有权**:`SerialOpen` 不负责关闭返回的 fd,调用方需自行 `close`。
- **波特率匹配**:`baud` 必须与对端一致,否则会乱码或收不到数据;仅支持 9600/19200/38400/57600/115200,其他值返回 `EINVAL`。
- **非 `speed_t` 接口**:不再接受 `B9600` 等常量。`B9600` 的数值并不等于 `9600`,传入会因不在支持表内而报 `EINVAL`。
- **平台相关**:依赖 POSIX termios,目前面向 Linux/类 Unix 系统。
