/*
 * my_snprintf.c を、標準ライブラリと突き合わせる差分テスト
 * 実際のHTTPヘッダと同じ形の書式を流す
 */

#include "../my_stdio.h"
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* バッファ不足時の挙動を検証するため、0〜十分な大きさまで用意 */
static const size_t SIZES[] = { 0, 1, 2, 3, 8, 16, 64 };
#define SIZE_COUNT (sizeof SIZES / sizeof SIZES[0])

/*
 * 渡した範囲の外を書いていないか見るため、書き込み領域の前後を番兵で挟む
 * 0x7f は %s にも %d にも %% にも現れないので、書き換わったら溢れたといえる
 */
#define GUARD_SIZE  8
#define GUARD_VALUE '\x7f'

/* テスト用バッファのサイズ (最大サイズ 64 と、後ろの番兵が十分収まる大きさ) */
#define AREA_SIZE 128

static int checks;
static int mismatches;

static void compare_all_sizes(const char *label, const char *fmt, ...);
static bool guards_intact(const char *area, size_t size);

int main(void) {
    /* --- 書式なし --- */
    compare_all_sizes("空文字列", "%s", "");
    compare_all_sizes("文字だけ", "hello");

    /* --- %s --- */
    compare_all_sizes("%s 1つ", "%s", "abc");
    compare_all_sizes("%s 3つ", "%s%s%s", "/docroot", "/index", ".html");
    compare_all_sizes("%s と地の文", "Content-Type: %s\r\n", "text/html");
    compare_all_sizes("長い %s", "%s", "0123456789012345678901234567890123456789");

    /* --- %d --- */
    compare_all_sizes("%d 正", "sigaction(%d) failed", 15);
    compare_all_sizes("%d 0", "%d", 0);
    compare_all_sizes("%d 負", "%d", -42);
    compare_all_sizes("%d 最小", "%d", INT_MIN);
    compare_all_sizes("%d 最大", "%d", INT_MAX);

    /* --- %lld --- */
    compare_all_sizes("%lld", "Content-Length: %lld\r\n", (long long)58);
    compare_all_sizes("%lld 0", "%lld", (long long)0);
    compare_all_sizes("%lld 負", "%lld", (long long)-1);
    compare_all_sizes("%lld 最小", "%lld", LLONG_MIN);
    compare_all_sizes("%lld 最大", "%lld", LLONG_MAX);

    /* --- 混在。実際の応答ヘッダと同じ形 --- */
    compare_all_sizes("応答ヘッダ",
        "HTTP/1.0 %s\r\nServer: %s\r\nContent-Length: %lld\r\nContent-Type: %s\r\n%s\r\n",
        "200 OK", "httpd/0.1", (long long)204800, "text/html", "");

    /* --- %% --- */
    compare_all_sizes("%% だけ", "%%");
    compare_all_sizes("%% と地の文", "100%% done");

    printf("%d件を比較、不一致 %d件\n", checks, mismatches);
    return mismatches != 0;
}

/*
 * 1つの書式を、渡せる大きさを変えながら標準と突き合わせるテスト
 * 可変長引数はそのまま他の関数へ渡せないので、va_list で受けて vsnprintf 系に流す
 */
static void compare_all_sizes(const char *label, const char *fmt, ...) {
    for (size_t i = 0; i < SIZE_COUNT; i++) {
        size_t size = SIZES[i];

        /* 書き込み領域の前後を番兵で埋め、はみ出しを検出できるようにする */
        char mine[GUARD_SIZE + AREA_SIZE];    /* 自作関数 */
        char theirs[GUARD_SIZE + AREA_SIZE];  /* 標準関数 */
        memset(mine,   GUARD_VALUE, sizeof mine);
        memset(theirs, GUARD_VALUE, sizeof theirs);

        va_list args;
        /* 自作の実装で処理 */
        va_start(args, fmt);
        int mine_len = my_vsnprintf(mine + GUARD_SIZE, size, fmt, args);
        va_end(args);

        /* 標準の実装で処理 */
        va_start(args, fmt);
        int theirs_len = vsnprintf(theirs + GUARD_SIZE, size, fmt, args);
        va_end(args);

        /* 戻り値 (書きたかった長さ) が標準と一致しているか確認 */
        bool lengths_matched = (mine_len == theirs_len);

        /* 書かれた文字列の中身が標準と一致しているか確認 */
        bool contents_matched;
        if (size == 0) {
            /* size が 0 の時はバッファに何も書かれず終端文字もないため、無条件で合格とする */
            contents_matched = true;
        } else {
            contents_matched = (strcmp(mine + GUARD_SIZE, theirs + GUARD_SIZE) == 0);
        }

        checks++;
        if (!lengths_matched || !contents_matched) {
            mismatches++;
            printf("NG  %s (size=%zu)\n", label, size);

            /* デバッグ表示 */
            const char *expected_str;
            const char *actual_str;
            if (size == 0) {
                /* size が 0 の時はバッファに何も書かれないことを表示 */
                expected_str = "(書き込みなし)";
                actual_str   = "(書き込みなし)";
            } else {
                expected_str = theirs + GUARD_SIZE;
                actual_str   = mine + GUARD_SIZE;
            }
            printf("      期待: %d %s\n", theirs_len, expected_str);
            printf("      実際: %d %s\n", mine_len,   actual_str);
        }

        /* 渡した size の外へはみ出していないかを検証 */
        checks++;
        if (!guards_intact(mine, size)) {
            mismatches++;
            printf("NG  %s (size=%zu) が確保した範囲の外を書いた\n", label, size);
        }
    }
}

/*
 * 書き込み領域の前後に置いた番兵が両方とも元のまま残っているか確認
 * 1バイトでも書き換わっていれば、渡した size の外に書いているということ
 */
static bool guards_intact(const char *area, size_t size) {
    for (int i = 0; i < GUARD_SIZE; i++) {
        if (area[i] != GUARD_VALUE) return false;                      /* 前 */
        if (area[GUARD_SIZE + size + i] != GUARD_VALUE) return false;  /* 後ろ */
    }
    return true;
}
