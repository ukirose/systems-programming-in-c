/* 標準ライブラリの汎用機能を自分で書いたもの。契約は標準に合わせる */

#pragma once

#include <stddef.h>

/* 確保した領域を返す。足りなければ NULL。size が 0 でも NULL */
void *my_malloc(size_t size);

/* my_malloc が返した領域を返却する。NULL は無視する */
void my_free(void *ptr);
