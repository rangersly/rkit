#include "mypoll.h"

#include <stdio.h>
#include <poll.h>
#include <unistd.h>


#define MAX_POLL 100

int main() {
    printf("mypoll demo run!\n");
    PollerCreate(MAX_POLL);
    PollerAdd(STDIN_FILENO, POLLIN, NULL);

    return 0;
}
