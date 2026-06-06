/* 標準ライブラリの文字列関数を自分で書いたもの。契約は標準に合わせる */

#pragma once

#include <stddef.h>

/* 終端の '\0' を含まない長さ */
size_t my_strlen(const char *s);

/* a が小さければ負、大きければ正、等しければ 0 */
int my_strcmp(const char *a, const char *b);

/* 先頭 n 文字までで比べる。ほかは my_strcmp と同じ */
int my_strncmp(const char *a, const char *b, size_t n);

/* 最初に c が現れた位置。無ければ NULL */
char *my_strchr(const char *s, int c);

/* 最後に c が現れた位置。無ければ NULL */
char *my_strrchr(const char *s, int c);

/* needle が最初に現れた位置。無ければ NULL */
char *my_strstr(const char *haystack, const char *needle);

/* 先頭から accept に含まれる文字が続く長さ */
size_t my_strspn(const char *s, const char *accept);
