#include "rserial.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <termios.h>
#include <unistd.h>

int SerialOpen(const char *path, speed_t baud) {
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }

    // O_NONBLOCK 避免 open 等待载波;O_CLOEXEC 防止 fd 泄漏给子进程
    int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }

    struct termios tty;
    // 必须先成功读出旧配置,否则后续操作会基于未初始化内存
    if (tcgetattr(fd, &tty) < 0) {
        goto fail;
    }

    cfmakeraw(&tty);

    // cfmakeraw 不会清除 2 停止位与硬件流控,这里显式清除以稳定落到 8N1
    tty.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS);
    tty.c_cflag |= (CS8 | CLOCAL | CREAD);

    if (cfsetispeed(&tty, baud) < 0) {
        goto fail;
    }
    if (cfsetospeed(&tty, baud) < 0) {
        goto fail;
    }

    // 非阻塞模式下 VMIN/VTIME 由内核忽略,这里仅表达意图
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) < 0) {
        goto fail;
    }

    // 丢弃打开前残留的输入,避免读到上一次会话的脏数据
    tcflush(fd, TCIFLUSH);

    return fd;

fail: {
    // close 可能覆盖 errno,保存后恢复,便于上层打印准确原因
    int saved_errno = errno;
    close(fd);
    errno = saved_errno;
    return -1;
}
}
