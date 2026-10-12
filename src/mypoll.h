#include <stdint.h>
#include <poll.h>

#ifndef POLL_MYPOLL_H_
#define POLL_MYPOLL_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

uint8_t CheckReady(struct pollfd *fds, nfds_t nfds, int timeout);
void AddPollfd(struct pollfd *fds, nfds_t *nfds, int fd, short events);
void DelPollfd(struct pollfd *fds, nfds_t *nfds, uint8_t del_size);

void MyRead();
void MyWrite();

#ifdef __cplusplus
}
#endif

#endif
