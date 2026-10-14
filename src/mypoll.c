#include "mypoll.h"

#include <errno.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <unistd.h>

struct Poller {
    struct pollfd *fds;
    void **user_data;
    nfds_t capacity;
    nfds_t nfds;
};

typedef struct Poller poller_t;

// 初始化poller
static poller_t poller = {
    .fds = NULL, .user_data = NULL, .capacity = 0, .nfds = 0};

int PollerCreate(nfds_t max_fds) {
    poller.fds = (struct pollfd *)calloc(max_fds, sizeof(struct pollfd));
    if (poller.fds == NULL) {
        perror("PollerCreate fail");
        return -1;
    }
    poller.user_data = calloc(max_fds, sizeof(void *));
    poller.capacity = max_fds;
    poller.nfds = 0;
    return 0;
}

void PollerDestory() {
    if (poller.fds != NULL) {
        free(poller.fds);
        free(poller.user_data);
    }
    poller.fds = NULL;
    poller.user_data = NULL;
    poller.capacity = 0;
    poller.nfds = 0;
}

int PollerAdd(int fd, short events, void *user_data) {
    if (poller.fds == NULL) {
        return -1;  // poller未创建即使用
    }
    if (poller.nfds >= poller.capacity) {
        return -2;  // 超过申请的最大fd数量,无法继续添加
    }
    poller.fds[poller.nfds].fd = fd;
    poller.fds[poller.nfds].events = events;
    poller.user_data[poller.nfds] = user_data;
    poller.nfds++;
    return 0;
}

int PollerDel(int fd) {
    if (poller.fds == NULL) {
        return -1;  // poller未创建即使用
    }
    for (nfds_t i = 0; i < poller.nfds; i++) {
        if (poller.fds[i].fd != fd) {
            if ((i + 1) == poller.nfds) {
                return -2;  // 未找到指定删除内容
            }
            continue;
        }
        poller.nfds--;
        poller.fds[i] = poller.fds[poller.nfds];
        poller.user_data[i] = poller.user_data[poller.nfds];
        poller.fds[poller.nfds].fd = -1;
        poller.fds[poller.nfds].revents = 0;
        break;
    }
    return 0;
}

int PollerRun(int timeout_ms, poller_cb callback) {
    int ready = poll(poller.fds, poller.nfds, timeout_ms);
    if (ready < 0) {
        perror("PollerWait: poll error");
        return -1;
    }
    for (uint8_t i = 0; ready != 0 || i < poller.nfds; i++) {
        if (poller.fds[i].revents == 0) {
            continue;
        }
        if (poller.fds[i].revents & (POLLIN | POLLOUT)) {
            callback(poller.fds[i].fd, poller.fds[i].revents,
                     poller.user_data[i]);
            ready--;
            continue;
        }
        if (poller.fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fprintf(stderr, "PollerRun : fd %d revents error, please close",
                    poller.fds[i].fd);
            PollerDel(poller.fds[i].fd);
            return poller.fds[i].fd;
        }
    }
    return 0;
}
