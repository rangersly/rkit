# mypoll

基于 `poll(2)` 的事件监听池库的项目语境,约定领域术语与工程结构。

## Language

**监听池 (poller)**:
本库的核心对象,持有 `fds`、`user_data`、`capacity`、`nfds` 等全局静态状态,进程内唯一。由 `PollerCreate` 创建、`PollerDestory` 释放。
_Avoid_: 事件循环、reactor

**监听项 (poll item)**:
监听池中的一个 `(fd, events)` 组合。同一个 fd 可用不同 events 注册多个监听项,`(fd, events)` 组合唯一。
_Avoid_: 连接、注册表项

**就绪事件 (revents)**:
`poll(2)` 返回的事件位掩码,取值为 `POLLIN`、`POLLOUT`、`POLLERR`、`POLLHUP`、`POLLNVAL` 等的按位或,回传给回调。
_Avoid_: 返回事件

**事件回调 (callback / poller_cb)**:
`PollerRun` 对每个就绪监听项调用的用户函数,签名为 `void (*)(int fd, short revents, void *user_data)`。
_Avoid_: 处理器、handler

**出错 fd (bad fd)**:
`revents` 含 `POLLERR`/`POLLHUP`/`POLLNVAL` 的监听项。`PollerRun` 会将其移出监听池并经 `err_fd` 返回,由调用方关闭。
_Avoid_: 异常连接

**公开头文件 (public header)**:
`include/mypoll.h`,对外暴露的接口。带 `#ifndef` 防护符与 `extern "C"` 守卫,`include/` 为搜索根。
_Avoid_: 对外头文件、公共接口头

**库目标 (library target)**:
由 `add_library` 定义的静态库 `mypoll`,`include/` 通过 `PUBLIC` 传播,`src/` 通过 `PRIVATE` 仅自用。
_Avoid_: 模块

**可执行目标 (executable target)**:
由 `add_executable` 定义的示例程序 `app`,链接 `mypoll`。
_Avoid_: 程序、入口

**测试目标 (test target)**:
由 `add_executable` + `add_test` 定义、通过 CTest 运行的测试程序 `tests`。
_Avoid_: 用例
