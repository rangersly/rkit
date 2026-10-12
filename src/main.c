#include "mypoll.h"

#include <stdio.h>
#include <poll.h>
#include <unistd.h>


#define MAX_POLL 100

int main() {
    printf("POLL test app run!\n");
    struct pollfd fds[MAX_POLL];
    nfds_t nfds = 0;
    AddPollfd(fds, &nfds, MAX_POLL, STDIN_FILENO, POLLIN);
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    return 0;
}
