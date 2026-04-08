/* 書き切るまで繰り返す write */

#pragma once

#include <stddef.h>

/* size バイト書き切るまで繰り返す。書けなくなったら -1 */
int write_all(int fd, const void *buf, size_t size);
