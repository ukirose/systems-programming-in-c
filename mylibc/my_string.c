/* 標準ライブラリの文字列関数を自分で書いたもの。契約は標準に合わせる */

#include "my_string.h"

size_t my_strlen(const char *s) {
    const char *head = s;
    while (*s) s++;

    return (size_t)(s - head);
}

int my_strcmp(const char *a, const char *b) {
    while (*a == *b) {
        if (*a == '\0') return 0;
        a++;
        b++;
    }

    /* char が符号付きの環境で 0x80 以上を負と見ないよう、符号なしで比べる */
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int my_strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];

        /* 片方が終端なら、そこまで一致しているので残りは見なくてよい */
        if (a[i] == '\0') return 0;
    }

    return 0;
}

char *my_strchr(const char *s, int c) {
    for (;; s++) {
        if (*s == (char)c) return (char *)s;
        if (*s == '\0') return NULL;
    }
}

char *my_strrchr(const char *s, int c) {
    const char *found = NULL;

    for (;; s++) {
        if (*s == (char)c) found = s;
        if (*s == '\0') return (char *)found;
    }
}

/*
 * 一致しなかったら1文字ずらして先頭からやり直す総当たり
 * 探す needle は "\r\n" と "\r\n\r\n" だけなので、KMP のような前処理は割に合わない
 */
char *my_strstr(const char *haystack, const char *needle) {
    if (*needle == '\0') return (char *)haystack;

    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;

        while (*n && *h == *n) { h++; n++; }
        if (*n == '\0') return (char *)haystack;
    }

    return NULL;
}

/*
 * 大文字だけを小文字へ倒す。tolower を使わないのはロケールに左右されないため
 * ヘッダ名と拡張子の照合は ASCII だけ見れば足りる
 */
static int fold_case(unsigned char c) {
    return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
}

int my_strcasecmp(const char *a, const char *b) {
    while (fold_case((unsigned char)*a) == fold_case((unsigned char)*b)) {
        if (*a == '\0') return 0;
        a++;
        b++;
    }

    return fold_case((unsigned char)*a) - fold_case((unsigned char)*b);
}

size_t my_strspn(const char *s, const char *accept) {
    const char *head = s;

    while (*s && my_strchr(accept, *s)) s++;

    return (size_t)(s - head);
}
