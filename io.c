/* 書き切るまで繰り返す write */

#include "io.h"
#include <unistd.h>
#include <errno.h>

int write_all(int fd, const void *buf, size_t size) {
    const char *head = buf;
    size_t written = 0;

    /* write(2) は要求より少ないバイト数で戻ることがある */
    while (written < size) {
        ssize_t n = write(fd, head + written, size - written);
        if (n < 0) {
            /* 割り込まれただけなら同じ位置から続ける */
            if (errno == EINTR) continue;
            return -1;
        }

        written += (size_t)n;
    }

    return 0;
}
