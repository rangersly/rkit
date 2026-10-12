#include <stdint.h>
#include <poll.h>
#include <sys/poll.h>

#ifndef POLL_MYPOLL_H_
#define POLL_MYPOLL_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

struct pollfd *fds;
nfds_t 

uint8_t CheckReady(struct pollfd *fds, nfds_t nfds, int timeout);
int AddPollfd(struct pollfd *fds, nfds_t *nfds, nfds_t max_nfds, int fd, short events);
int DelPollfd(struct pollfd *fds, nfds_t *nfds, uint8_t del_size);

void MyRead();
void MyWrite();

#ifdef __cplusplus
}
#endif

#endif
