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
    if (max_fds == 0) {
        return -1;
    }
    struct pollfd *fds = (struct pollfd *)calloc(max_fds, sizeof(struct pollfd));
    if (fds == NULL) {
        perror("PollerCreate: fds calloc fail");
        return -1;
    }
    void **user_data = calloc(max_fds, sizeof(void *));
    if (user_data == NULL) {
        perror("PollerCreate: user_data calloc fail");
        free(fds);
        return -1;
    }
    free(poller.fds);
    free(poller.user_data);
    poller.fds = fds;
    poller.user_data = user_data;
    poller.capacity = max_fds;
    poller.nfds = 0;
    return 0;
}

void PollerDestory() {
    free(poller.fds);
    free(poller.user_data);
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
    if (poller.fds == NULL) {
        return -1;  // poller未创建即使用
    }
    if (callback == NULL) {
        return -1;  // 回调函数为空
    }
    int ready = poll(poller.fds, poller.nfds, timeout_ms);
    if (ready < 0) {
        perror("PollerRun: poll error");
        return -2;
    }
    if (ready == 0) {
        return 0;
    }
    for (nfds_t i = 0; i < poller.nfds; i++) {
        short revents = poller.fds[i].revents;
        if (revents == 0) {
            continue;
        }
        if (revents & (POLLIN | POLLOUT)) {
            callback(poller.fds[i].fd, revents, poller.user_data[i]);
        }
        if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
            int bad_fd = poller.fds[i].fd;
            fprintf(stderr, "PollerRun: fd %d revents error, please close\n",
                    bad_fd);
            PollerDel(bad_fd);
            return bad_fd;
        }
    }
    return 0;
}
