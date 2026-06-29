/* 書式付きで文字列を組み立てる */

#include "my_stdio.h"

/*
 * 書き出し先と、書きたかった長さ
 * 呼出側が切り詰めを検出できるよう、size を超えても written は数え続ける
 */
struct Sink {
    char *buf;
    size_t size;
    size_t written;
};

static void put(struct Sink *sink, char c);
static void put_string(struct Sink *sink, const char *s);
static void put_signed(struct Sink *sink, long long value);
static void put_unsigned(struct Sink *sink, unsigned long long value);

int my_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
    struct Sink sink = { buf, size, 0 };

    for (const char *p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            put(&sink, *p);
            continue;
        }

        /* 末尾が '%' 1つなら、書式指定子が無いのでそのまま出して終わる */
        if (p[1] == '\0') {
            put(&sink, '%');
            break;
        }

        if (p[1] == 's') {
            put_string(&sink, va_arg(ap, const char *));
            p += 1;
        } else if (p[1] == 'd') {
            /* 可変引数では int より小さい型は int に格上げされている */
            put_signed(&sink, va_arg(ap, int));
            p += 1;
        } else if (p[1] == 'l' && p[2] == 'l' && p[3] == 'd') {
            put_signed(&sink, va_arg(ap, long long));
            p += 3;
        } else if (p[1] == '%') {
            put(&sink, '%');
            p += 1;
        } else {
            /* 対応していない指定子。読み飛ばすと引数とずれるので、そのまま出して見えるようにする */
            put(&sink, '%');
        }
    }

    /* 収まらなかったときは最後の 1バイトを終端にする */
    if (size > 0) {
        buf[sink.written < size ? sink.written : size - 1] = '\0';
    }

    return (int)sink.written;
}

int my_snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int length = my_vsnprintf(buf, size, fmt, ap);
    va_end(ap);

    return length;
}

/* 終端の 1バイトを残せるときだけ書く。書けなくても written は進める */
static void put(struct Sink *sink, char c) {
    if (sink->written + 1 < sink->size) sink->buf[sink->written] = c;
    sink->written++;
}

static void put_string(struct Sink *sink, const char *s) {
    /* glibc は NULL を "(null)" と出すが、規格は未定義。落とさないために合わせる */
    if (s == NULL) s = "(null)";

    for (; *s != '\0'; s++) put(sink, *s);
}

static void put_signed(struct Sink *sink, long long value) {
    if (value >= 0) {
        put_unsigned(sink, (unsigned long long)value);
        return;
    }

    put(sink, '-');

    /*
     * 最小値は符号を反転すると表現できないので、1 足してから反転し、符号なしで 1 を戻す
     * -value と書くとそこで未定義動作になる
     */
    put_unsigned(sink, (unsigned long long)(-(value + 1)) + 1);
}

static void put_unsigned(struct Sink *sink, unsigned long long value) {
    /* 64ビットの符号なし十進数は最大 20桁 */
    char digits[20];
    int count = 0;

    do {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);

    while (count > 0) put(sink, digits[--count]);
}
