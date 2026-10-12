#include "mypoll.h"

#include <stdbool.h>
#include <stdio.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>

void ShowTest(bool test);

int main() {
    time_t t1, dt;
    printf("==================\n");
    printf("===Test running===\n");
    printf("==================\n\n");

    printf("---mypoll.h test---\n");
    do {
        struct pollfd fds[10];
        nfds_t nfds = 0;
        do {
            printf("AddPollfd: ");
            t1 = clock();
            AddPollfd(fds, &nfds, STDIN_FILENO, POLLIN);
            dt = (clock() - t1) * 1000 / CLOCKS_PER_SEC;
            ShowTest(nfds == 1 && fds[0].fd == STDIN_FILENO &&
                     fds[0].events == POLLIN);
            printf("    Use time: %ld\n", dt);
        } while (false);
        printf("CheckReady: ");

    } while (false);
}

void ShowTest(bool test) {
    if (test) {
        printf("\e[32mOK!\e[0m\n");
    } else {
        printf("\e[31mERROR!\e[0m\n");
    }
}
