/* 書き切るまで繰り返す write */

#pragma once

#include <stddef.h>

/*
 * size バイト書き切るまで繰り返す。成功なら 0
 * 書けなくなったら -1。そのときまでに一部が相手へ届いていることがある
 */
int write_all(int fd, const void *buf, size_t size);
