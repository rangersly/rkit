#include "mypoll.h"
#include "rio.h"

#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_POLL 16
#define TICK_MS 2000

static int running = 1;
static int tick = 0;

static void OnEvent(int fd, short revents, void *user_data) {
    const char *name = user_data ? (const char *)user_data : "fd";

    if (!(revents & POLLIN)) {
        return;
    }

    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0) {
        printf("[%s] read: ", name);
        fflush(stdout);
        if (IoWrite(STDOUT_FILENO, buf, (size_t)n) < 0) {
            perror("write");
        }
    } else if (n == 0) {
        printf("[%s] closed, unregister\n", name);
        PollerDel(fd, POLLIN);
        running = 0;
    } else {
        perror("read");
    }
}

int main(void) {
    if (PollerCreate(MAX_POLL) != 0) {
        fprintf(stderr, "PollerCreate failed\n");
        return 1;
    }
    if (PollerAdd(STDIN_FILENO, POLLIN, "stdin") != 0) {
        fprintf(stderr, "PollerAdd failed\n");
        PollerDestory();
        return 1;
    }

    printf("rkit mypoll demo: 输入内容回车回显; 每 %d ms 打印一次心跳; EOF/Ctrl-C 退出\n\n",
           TICK_MS);

    while (running) {
        int err_fd = -1;
        int ret = PollerRun(TICK_MS, OnEvent, &err_fd);

        if (ret == 1) {
            printf("[demo] fd %d error, closed\n", err_fd);
            close(err_fd);
            if (err_fd == STDIN_FILENO) {
                running = 0;
            }
        } else if (ret == 0) {
            printf("[tick %d]\n", ++tick);
        } else if (ret == -2) {
            fprintf(stderr, "[demo] poll error, stop\n");
            break;
        } else {
            break;
        }
    }

    PollerDestory();
    printf("[demo] bye\n");
    return 0;
}
