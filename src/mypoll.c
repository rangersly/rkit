#include "mypoll.h"

#include <errno.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/poll.h>
#include <unistd.h>

uint8_t CheckReady(struct pollfd *fds, nfds_t nfds, int timeout) {
    int ready = poll(fds, nfds, timeout);
retry:
    if (ready < 0) {
        if (errno == EINTR) {
            goto retry;
        }
        perror("CheckReady: poll return error");
        goto err;
    } else if (ready == 0) {
        goto out;
    }
    for (uint8_t i = 0; ready != 0 || i < nfds; i++) {
        if (fds[i].revents == 0) {
            continue;
        }
        if (fds[i].revents & POLLIN) {
            MyRead();
            ready--;
        }
        if (fds[i].revents & POLLOUT) {
            MyWrite();
            ready--;
        }
        if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            DelPollfd(fds, &nfds, i);
        }
    }
out:
    return 0;
err:
    return 1;
}

int AddPollfd(struct pollfd *fds, nfds_t *nfds, nfds_t max_nfds, int fd,
              short events) {
    if ((*nfds + 1) >= max_nfds) {
        return -1;
    }
    fds[*nfds].fd = fd;
    fds[*nfds].events = events;
    (*nfds)++;
    return 0;
}

int DelPollfd(struct pollfd *fds, nfds_t *nfds, uint8_t del_size) {
    (*nfds)--;
    if (*nfds < del_size) {
        (*nfds)++;
        return -1;
    }
    fds[del_size] = fds[*nfds];
    fds[*nfds].fd = -1;
    fds[*nfds].revents = 0;
    return 0;
}

void MyRead() {}
void MyWrite() {}
