# rkit

轻量级 C 工具箱的项目语境,约定领域术语与工程结构。首个工具模块是 `mypoll`(基于 `poll(2)` 的事件监听池)。

## Language

**工具箱 (rkit)**:
本仓库整体,承载一组可独立使用的通用 C 工具模块。项目名 `rkit`,CMake 项目名与核心库目标同名。
_Avoid_: 框架、SDK

**工具模块 (module)**:
工具箱中的一个独立工具,包含公开头文件、实现、测试与文档。简单模块使用单个头文件,体量大时使用目录。
_Avoid_: 组件、插件

**模块头文件 (module header)**:
`include/` 下每个工具模块的公开头文件。简单模块命名为 `r<tool>.h`(如 `rio.h`);体量大、需要多个头文件的模块放入 `include/<tool>/` 目录。统一带 `#ifndef` 防护符(形如 `RKIT_<TOOL>_H_`)与 `extern "C"` 守卫。
_Avoid_: 公共接口头、对外头文件

**io 模块 (io)**:
`rio.h` 提供的非阻塞 `read(2)` / `write(2)` 薄封装,由 `IoRead` / `IoWrite` 实现。只做单次系统调用并对 `EINTR` 重试,不改变 fd 阻塞模式,也不做读写缓冲。
_Avoid_: 异步 I/O、I/O 框架

**部分读写 (partial I/O)**:
单次 read/write 成功但字节数小于请求长度的情形。io 模块不隐藏该行为,由调用方重试或累积。
_Avoid_: 短读、残包

**serial 模块 (serial)**:
`rserial.h` 提供的串口打开封装,由 `SerialOpen` 实现。以非阻塞方式打开设备,配置为原始模式下的 8N1、无流控,并丢弃残留输入;返回的 fd 交给调用方使用和关闭。
_Avoid_: 串口驱动、串口框架

**监听池 (poller)**:
`mypoll` 模块的核心对象,持有 `fds`、`user_data`、`capacity`、`nfds` 等全局静态状态,进程内唯一。由 `PollerCreate` 创建、`PollerDestory` 释放。
_Avoid_: 事件循环、reactor

**监听项 (poll item)**:
监听池中的一个 `(fd, events)` 组合。同一个 fd 可用不同 events 注册多个监听项,`(fd, events)` 组合唯一。
_Avoid_: 连接、注册表项

**就绪事件 (revents)**:
`poll(2)` 返回的事件位掩码,取值为 `POLLIN`、`POLLOUT`、`POLLERR`、`POLLHUP`、`POLLNVAL` 等的按位或,回传给回调。
_Avoid_: 返回事件

**事件回调 (callback / PollerCb)**:
`PollerRun` 对每个就绪监听项调用的用户函数,签名为 `void (*)(int fd, short revents, void *user_data)`。
_Avoid_: 处理器、handler

**出错 fd (bad fd)**:
`revents` 含 `POLLERR`/`POLLHUP`/`POLLNVAL` 的监听项。`PollerRun` 会将其移出监听池并经 `err_fd` 返回,由调用方关闭。
_Avoid_: 异常连接

**公开头文件 (public header)**:
`include/mypoll.h`,`mypoll` 模块对外暴露的接口。带 `#ifndef` 防护符与 `extern "C"` 守卫,`include/` 为搜索根。
_Avoid_: 对外头文件、公共接口头

**库目标 (library target)**:
由 `add_library` 定义的静态库 `rkit`(别名 `rkit::rkit`)。`include/` 通过 `PUBLIC` 传播,`src/` 通过 `PRIVATE` 仅自用。
_Avoid_: 模块

**测试目标 (test target)**:
由 `add_executable` + `add_test` 定义、通过 CTest 运行的测试程序。每个工具模块一个,命名为 `test_<tool>`(如 `test_poll`),源文件为 `tests/test_<tool>.c`。
_Avoid_: 用例
