#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "rserial.h"

static int failures = 0;

static void ShowTest(int test) {
    if (test) {
        printf("\e[32mOK!\e[0m\n");
    } else {
        printf("\e[31mERROR!\e[0m\n");
        failures++;
    }
}

static int OpenPtyMaster(char *slave_path, size_t size) {
    int master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0) {
        return -1;
    }
    if (grantpt(master) < 0 || unlockpt(master) < 0) {
        close(master);
        return -1;
    }
    const char *name = ptsname(master);
    if (name == NULL) {
        close(master);
        return -1;
    }
    snprintf(slave_path, size, "%s", name);
    return master;
}

int main(void) {
    printf("==================\n");
    printf("===Test serial====\n");
    printf("==================\n\n");

    printf("---参数与错误路径---\n");
    errno = 0;
    printf("path 为 NULL 返回 EINVAL : ");
    ShowTest(SerialOpen(NULL, B9600) == -1 && errno == EINVAL);
    errno = 0;
    printf("设备不存在返回 -1 : ");
    ShowTest(SerialOpen("/nonexistent/rkit-no-such-device", B9600) == -1);

    printf("\n---伪终端上打开与配置---\n");
    char path[128];
    int master = OpenPtyMaster(path, sizeof(path));
    printf("创建伪终端 : ");
    ShowTest(master >= 0);

    if (master >= 0) {
        int fd = SerialOpen(path, B9600);
        printf("SerialOpen 成功 : ");
        ShowTest(fd >= 0);

        if (fd >= 0) {
            struct termios tty;
            printf("tcgetattr 成功 : ");
            ShowTest(tcgetattr(fd, &tty) == 0);

            printf("数据位为 CS8 : ");
            ShowTest((tty.c_cflag & CSIZE) == CS8);
            printf("无校验/1停止位/无流控 : ");
            ShowTest((tty.c_cflag & (PARENB | CSTOPB | CRTSCTS)) == 0);
            printf("CLOCAL 与 CREAD 已设置 : ");
            ShowTest((tty.c_cflag & (CLOCAL | CREAD)) == (CLOCAL | CREAD));
            printf("输入输出波特率为 B9600 : ");
            ShowTest(cfgetispeed(&tty) == B9600 && cfgetospeed(&tty) == B9600);
            printf("原始模式(无 ICANON/ECHO) : ");
            ShowTest((tty.c_lflag & (ICANON | ECHO)) == 0);

            int flags = fcntl(fd, F_GETFL, 0);
            printf("fd 带 O_NONBLOCK : ");
            ShowTest(flags >= 0 && (flags & O_NONBLOCK) != 0);

            close(fd);
        }
        close(master);
    }

    printf("\n==================\n");
    if (failures == 0) {
        printf("\e[32mALL TESTS PASSED\e[0m\n");
    } else {
        printf("\e[31m%d TEST(S) FAILED\e[0m\n", failures);
    }
    printf("==================\n");

    return failures == 0 ? 0 : 1;
}
