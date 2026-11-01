#include "rio.h"

#include <errno.h>
#include <unistd.h>

ssize_t IoRead(int fd, void *buf, size_t len) {
    ssize_t n;
    do {
        n = read(fd, buf, len);
    } while (n < 0 && errno == EINTR);
    return n;
}

ssize_t IoWrite(int fd, const void *buf, size_t len) {
    ssize_t n;
    do {
        n = write(fd, buf, len);
    } while (n < 0 && errno == EINTR);
    return n;
}
