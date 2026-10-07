#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("POLL test app run!\n");
    struct pollfd fds[1];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    while (1) {
        sleep(3);
        int ret = poll(fds, 1, 0);
        if (ret == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        } else if (ret == 0) {
            printf("超时，没有输入。\n");
        } else {
            // 检查 revents 是否包含 POLLIN
            if (fds[0].revents & POLLIN) {
                char buf[256];
                ssize_t n = read(fds[0].fd, buf, sizeof(buf) - 1);
                if (n > 0) {
                    buf[n] = '\0';
                    printf("读取到输入: %s", buf);
                }
            }
        }
    }

    return 0;
}
