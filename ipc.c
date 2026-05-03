#include <errno.h>
#include <unistd.h>
#include "stock.h"

// read exactly count bytes unless end of file or error happens
int read_full(int fd, void *buf, size_t count) {
    size_t total = 0;
    char *p = buf;  // byte pointer for filling the buffer

    while (total < count) {
        ssize_t n = read(fd, p + total, count - total);

        if (n == 0) {
            if (total == 0) {
                return 0;   // clean end of file before reading a message 
            }
            return -1;  // truncated message 
        }

        if (n < 0) {
            if (errno == EINTR) {
                continue;   // retry if interrupted
            }
            return -1;
        }

        total += (size_t)n;    // add bytes read so far
    }

    return 1;   // full message was read
}

// write exactly count bytes unless error happens
int write_full(int fd, const void *buf, size_t count) {
    size_t total = 0;
    const char *p = buf;    // byte pointer for reading from buffer

    while (total < count) {
        ssize_t n = write(fd, p + total, count - total);

        if (n < 0) {
            if (errno == EINTR) {
                continue;   // retry if interrupted
            }
            return -1;
        }

        total += (size_t)n;    // add bytes written so far
    }

    return 0;   // full message was written
}

// close a file descriptor and retry if interrupted
int close_fd(int fd) {
    int result;

    do {
        result = close(fd);
    } while (result == -1 && errno == EINTR);   // retry if interrupted

    return result;
}