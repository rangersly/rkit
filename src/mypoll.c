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
    struct pollfd *ready;
    void **ready_user_data;
    // 每次 Create/Destory 递增,供 PollerRun 检测回调内的销毁或重建
    uint64_t generation;
};

typedef struct Poller poller_t;

// 初始化poller
static poller_t poller = {.fds = NULL,
                          .user_data = NULL,
                          .capacity = 0,
                          .nfds = 0,
                          .ready = NULL,
                          .ready_user_data = NULL,
                          .generation = 0};

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
    struct pollfd *ready = calloc(max_fds, sizeof(struct pollfd));
    if (ready == NULL) {
        perror("PollerCreate: ready calloc fail");
        free(fds);
        free(user_data);
        return -1;
    }
    void **ready_user_data = calloc(max_fds, sizeof(void *));
    if (ready_user_data == NULL) {
        perror("PollerCreate: ready_user_data calloc fail");
        free(fds);
        free(user_data);
        free(ready);
        return -1;
    }
    free(poller.fds);
    free(poller.user_data);
    free(poller.ready);
    free(poller.ready_user_data);
    poller.fds = fds;
    poller.user_data = user_data;
    poller.ready = ready;
    poller.ready_user_data = ready_user_data;
    poller.capacity = max_fds;
    poller.nfds = 0;
    poller.generation++;
    return 0;
}

void PollerDestory(void) {
    free(poller.fds);
    free(poller.user_data);
    free(poller.ready);
    free(poller.ready_user_data);
    poller.fds = NULL;
    poller.user_data = NULL;
    poller.ready = NULL;
    poller.ready_user_data = NULL;
    poller.capacity = 0;
    poller.nfds = 0;
    poller.generation++;
}

int PollerAdd(int fd, short events, void *user_data) {
    if (poller.fds == NULL) {
        return -1;  // poller未创建即使用
    }
    for (nfds_t i = 0; i < poller.nfds; i++) {
        if (poller.fds[i].fd == fd && poller.fds[i].events == events) {
            return -3;  // 已存在相同的(fd, events)项
        }
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

int PollerDel(int fd, short events) {
    if (poller.fds == NULL) {
        return -1;  // poller未创建即使用
    }
    for (nfds_t i = 0; i < poller.nfds; i++) {
        if (poller.fds[i].fd != fd || poller.fds[i].events != events) {
            continue;
        }
        nfds_t tail = poller.nfds - 1;
        poller.fds[i] = poller.fds[tail];
        poller.user_data[i] = poller.user_data[tail];
        poller.fds[tail].fd = -1;
        poller.fds[tail].events = 0;
        poller.fds[tail].revents = 0;
        poller.user_data[tail] = NULL;
        poller.nfds = tail;
        return 0;
    }
    return -2;  // 未找到指定删除内容
}

int PollerRun(int timeout_ms, poller_cb callback, int *err_fd) {
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
    // 记录本轮代际,回调内Create/Destory会使其变化
    uint64_t generation = poller.generation;
    // 先快照本轮就绪项,回调内Add/Del不会破坏本轮遍历
    nfds_t nready = 0;
    for (nfds_t i = 0; i < poller.nfds; i++) {
        if (poller.fds[i].revents == 0) {
            continue;
        }
        poller.ready[nready] = poller.fds[i];
        poller.ready_user_data[nready] = poller.user_data[i];
        nready++;
    }
    for (nfds_t i = 0; i < nready; i++) {
        // 在回调前取出全部数据,避免回调销毁快照后再次访问
        int fd = poller.ready[i].fd;
        short revents = poller.ready[i].revents;
        short events = poller.ready[i].events;
        void *user_data = poller.ready_user_data[i];
        if (revents & (POLLIN | POLLOUT)) {
            callback(fd, revents, user_data);
            if (poller.generation != generation) {
                // 回调内销毁或重建了监听池,中止本轮剩余分发
                return -3;
            }
        }
        if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fprintf(stderr, "PollerRun: fd %d revents error, please close\n",
                    fd);
            PollerDel(fd, events);
            if (err_fd != NULL) {
                *err_fd = fd;
            }
            return 1;
        }
    }
    return 0;
}
