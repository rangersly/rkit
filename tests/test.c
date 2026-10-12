#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>

#include "mypoll.h"

void ShowTest(int test);

int main() {
    time_t t1, dt;
    printf("==================\n");
    printf("===Test running===\n");
    printf("==================\n\n");

    printf("---mypoll.h test---\n");
    do {
        const nfds_t fds_size = 100;
        struct pollfd fds[fds_size];
        nfds_t nfds = 0;
        do {
            printf("AddPollfd: ");
            t1 = clock();
            AddPollfd(fds, &nfds, fds_size,  STDIN_FILENO, POLLIN);
            dt = (clock() - t1) * 1000 / CLOCKS_PER_SEC;
            ShowTest((fds[0].fd == STDIN_FILENO) && (fds[0].events == POLLIN) &&
                     (nfds == 1));
            printf("    Use time: %ld\n", dt);
        } while (false);
        printf("CheckReady: ");

    } while (false);
}

void ShowTest(int test) {
    if (test) {
        printf("\e[32mOK!\e[0m\n");
    } else {
        printf("\e[31mERROR!\e[0m\n");
    }
}
