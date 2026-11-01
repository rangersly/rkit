#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rio.h"

static int failures = 0;

static void ShowTest(int test) {
    if (test) {
        printf("\e[32mOK!\e[0m\n");
    } else {
        printf("\e[31mERROR!\e[0m\n");
        failures++;
    }
}

static int SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void) {
    printf("==================\n");
    printf("===Test io running===\n");
    printf("==================\n\n");

    int p[2];
    char buf[64];
    ssize_t n;

    printf("---基本读写---\n");
    if (pipe(p) != 0) {
        perror("pipe");
        return 1;
    }
    printf("IoWrite 写入 5 字节 : ");
    ShowTest(IoWrite(p[1], "hello", 5) == 5);
    memset(buf, 0, sizeof(buf));
    printf("IoRead 读回相同内容 : ");
    ShowTest(IoRead(p[0], buf, sizeof(buf)) == 5 &&
             memcmp(buf, "hello", 5) == 0);
    close(p[0]);
    close(p[1]);

    printf("\n---非阻塞无数据---\n");
    if (pipe(p) != 0) {
        perror("pipe");
        return 1;
    }
    printf("设置读端非阻塞 : ");
    ShowTest(SetNonBlocking(p[0]) == 0);
    errno = 0;
    printf("空 pipe IoRead 返回 EAGAIN : ");
    ShowTest(IoRead(p[0], buf, sizeof(buf)) == -1 &&
             (errno == EAGAIN || errno == EWOULDBLOCK));
    close(p[0]);
    close(p[1]);

    printf("\n---错误与 EOF---\n");
    errno = 0;
    printf("无效 fd IoRead 返回 EBADF : ");
    ShowTest(IoRead(-1, buf, sizeof(buf)) == -1 && errno == EBADF);
    errno = 0;
    printf("无效 fd IoWrite 返回 EBADF : ");
    ShowTest(IoWrite(-1, "x", 1) == -1 && errno == EBADF);
    if (pipe(p) != 0) {
        perror("pipe");
        return 1;
    }
    close(p[1]);
    printf("对端关闭 IoRead 返回 0 : ");
    ShowTest(IoRead(p[0], buf, sizeof(buf)) == 0);
    close(p[0]);

    printf("\n---非阻塞部分写---\n");
    if (pipe(p) != 0) {
        perror("pipe");
        return 1;
    }
    printf("设置写端非阻塞 : ");
    ShowTest(SetNonBlocking(p[1]) == 0);
    static char chunk[5000];
    int partial_seen = 0;
    for (;;) {
        n = IoWrite(p[1], chunk, sizeof(chunk));
        if (n < 0) {
            break;
        }
        if (n < (ssize_t)sizeof(chunk)) {
            partial_seen = 1;
        }
    }
    printf("写满后 IoWrite 返回 EAGAIN : ");
    ShowTest(n == -1 && errno == EAGAIN);
    printf("过程中出现部分写 : ");
    ShowTest(partial_seen);
    close(p[0]);
    close(p[1]);

    printf("\n==================\n");
    if (failures == 0) {
        printf("\e[32mALL TESTS PASSED\e[0m\n");
    } else {
        printf("\e[31m%d TEST(S) FAILED\e[0m\n", failures);
    }
    printf("==================\n");

    return failures == 0 ? 0 : 1;
}
