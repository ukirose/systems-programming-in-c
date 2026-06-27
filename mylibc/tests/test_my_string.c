/*
 * my_string.c の 8関数を標準ライブラリと突き合わせる差分テスト
 * 標準ライブラリを正解役にする
 */

#include "../my_string.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/*
 * 0x80(非英数字バイト)以上のバイトを含む入力を混ぜている。
 * char が符号付きの環境では、比較を unsigned char に直さないとここで順序が逆転する
 */
static const char *const INPUTS[] = {
    "", "a", "abc", "abcabc", "\x80\xff", "a\x80", "Hello, World",
    "GET / HTTP/1.0", "  \t x", "aaaaab", "\xff\xff\xff",
    "Content-Length", "content-length", "CONTENT-LENGTH", "keep-alive", "KEEP-ALIVE",
};
#define INPUT_COUNT (sizeof INPUTS / sizeof INPUTS[0])

static int checks;
static int mismatches;

static void check_one_string(void);
static void check_two_strings(void);
static void compare(const char *name, bool matched, const char *where_fmt, ...);
static bool same_sign(int a, int b);

int main(void) {
    check_one_string();
    check_two_strings();

    printf("%d件を比較、不一致 %d件\n", checks, mismatches);
    return mismatches != 0;
}

/* strlen / strchr / strrchr を標準と突き合わせるテスト。文字列 1本で答えが決まる組 */
static void check_one_string(void) {
    for (size_t i = 0; i < INPUT_COUNT; i++) {
        const char *s = INPUTS[i];

        compare("my_strlen", my_strlen(s) == strlen(s), "INPUTS[%zu]", i);

        /* 探す文字は int で受け取る仕様なので、0 から 255 まで全部試す */
        for (int c = 0; c < 256; c++) {
            compare("my_strchr",  my_strchr(s, c)  == strchr(s, c),  "INPUTS[%zu], c=%d", i, c);
            compare("my_strrchr", my_strrchr(s, c) == strrchr(s, c), "INPUTS[%zu], c=%d", i, c);
        }
    }
}

/* strcmp / strncmp / strcasecmp / strstr / strspn を標準と突き合わせるテスト */
static void check_two_strings(void) {
    for (size_t i = 0; i < INPUT_COUNT; i++) {
        const char *s = INPUTS[i];

        for (size_t j = 0; j < INPUT_COUNT; j++) {
            const char *t = INPUTS[j];

            /* 比較の 3つは、返る値そのものではなく符号だけを突き合わせる */
            compare("my_strcmp", same_sign(my_strcmp(s, t), strcmp(s, t)),
                    "INPUTS[%zu], INPUTS[%zu]", i, j);

            /* n は最長の入力 (14バイト) を超えるところまで取り、途中で切れる場合と切れない場合の両方を通す */
            for (size_t n = 0; n <= 16; n++) {
                compare("my_strncmp", same_sign(my_strncmp(s, t, n), strncmp(s, t, n)),
                        "INPUTS[%zu], INPUTS[%zu], n=%zu", i, j, n);
            }

            compare("my_strcasecmp", same_sign(my_strcasecmp(s, t), strcasecmp(s, t)),
                    "INPUTS[%zu], INPUTS[%zu]", i, j);

            /* 探索の 2つは、返る位置と長さがそのまま一致していなければならない */
            compare("my_strstr", my_strstr(s, t) == strstr(s, t),
                    "INPUTS[%zu], INPUTS[%zu]", i, j);

            compare("my_strspn", my_strspn(s, t) == strspn(s, t),
                    "INPUTS[%zu], INPUTS[%zu]", i, j);
        }
    }
}

/* テスト結果を判定・カウントする */
static void compare(const char *name, bool matched, const char *where_fmt, ...) {
    checks++;
    if (matched) return;

    mismatches++;
    printf("NG  %s  ", name);

    va_list args;
    va_start(args, where_fmt);
    vprintf(where_fmt, args);
    va_end(args);

    printf("\n");
}

/* strcmp 系の戻り値は、差の大きさではなく符号だけが規定されている */
static bool same_sign(int a, int b) {
    return (a > 0) == (b > 0) && (a < 0) == (b < 0);
}
