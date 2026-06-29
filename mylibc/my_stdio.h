/* 書式付きで文字列を組み立てる */

#pragma once

#include <stdarg.h>
#include <stddef.h>

/*
 * 扱う書式は %s %d %lld %% の 4つだけ。それ以外は '%' ごとそのまま出す
 * 幅・精度・フラグは解釈しない
 *
 * 返すのは「書きたかった長さ」で、終端の '\0' は含まない
 * size に収まらなくても切り詰めた長さではなく、必要だった長さを返す (C99 の snprintf と同じ)
 * この契約に呼出側が依存しているので、書けた長さを返す実装に変えてはいけない
 *
 * size が 0 でなければ、必ず終端を置く。size が 0 なら buf に一切触らない
 */
int my_snprintf(char *buf, size_t size, const char *fmt, ...);
int my_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
