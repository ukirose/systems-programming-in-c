/*
 * my_malloc.c の性質テスト
 * 返るアドレスは実装ごとに違い標準の malloc と突き合わせられないので、
 * どのアロケータでも成り立つ性質を確かめる
 */

#include "../my_stdlib.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int checks;
static int failures;

static void check_merge(void);
static void check_exact_fit(void);
static void check_zero_and_null(void);
static void check_write_and_read_back(void);
static void check_alignment(void);
static void check_blocks_are_independent(void);
static void check_reuse(void);
static void check(const char *name, bool ok);
static bool has_overlap(char **blocks, size_t *sizes, int count);
static bool has_broken_content(char **blocks, size_t *sizes, int count);
static bool already_seen(char **seen, int count, char *p);

int main(void) {
    /*
     * 壊れたアロケータは NG を出した後で落ちたり止まったりする
     * 端末以外へ出すと既定では溜め込まれ、そこまでの NG が消えるので、都度書き出す
     */
    setvbuf(stdout, NULL, _IONBF, 0);

    check_merge();
    check_exact_fit();
    check_zero_and_null();
    check_write_and_read_back();
    check_alignment();
    check_blocks_are_independent();
    check_reuse();

    printf("%d件を確認、不合格 %d件\n", checks, failures);
    return failures != 0;
}

/*
 * 隣り合う空きが 1つに繋がることをテスト
 * 断片化していない状態でしかチェックできないので、最初に呼ぶ
 */
static void check_merge(void) {
    enum { PIECES = 32, PIECE_SIZE = 1000 };

    /* 32個に刻んで、隣り合うブロックが並んだ状態を作る */
    char *pieces[PIECES];
    bool all_allocated = true;
    for (int i = 0; i < PIECES; i++) {
        pieces[i] = my_malloc(PIECE_SIZE);
        if (pieces[i] == NULL) all_allocated = false;
    }
    /* NULL が混じると範囲の下端が NULL まで広がり、この先の検査が素通りする */
    check("刻むための 32個を確保できる", all_allocated);
    if (!all_allocated) return;

    /* 32個のブロックが占めるアドレス範囲を記録 */
    char *lowest  = pieces[0];
    char *highest = pieces[0];
    for (int i = 0; i < PIECES; i++) {
        if (pieces[i] < lowest)  lowest  = pieces[i];
        if (pieces[i] > highest) highest = pieces[i];
    }

    /* 32個すべてを解放してフリーリストに戻す */
    for (int i = 0; i < PIECES; i++) my_free(pieces[i]);

    /*
     * 検証: 16000バイト (32個ぶんの半分) を要求してみる
     * マージされていれば元の範囲内から割り当てられる
     * マージされていなければ、外側の新しいアドレスから割り当てられる
     */
    char *merged = my_malloc(PIECES * PIECE_SIZE / 2);
    bool within_original_range = merged != NULL && merged >= lowest && merged <= highest;
    check("解放した隣同士が繋がって大きい要求に応えられる", within_original_range);

    my_free(merged);
}

/* 要求サイズと「ピッタリ同じ大きさ」の空きブロックを再利用できるかテスト */
static void check_exact_fit(void) {
    enum { SIZE = 64 };

    /*
     * 前後を確保したまま中央だけ解放する
     * 前後が埋まっているためマージが起きず、ピッタリ同じ大きさの空きブロックが作られる
     */
    char *before = my_malloc(SIZE);
    char *target = my_malloc(SIZE);
    char *after  = my_malloc(SIZE);
    my_free(target);

    /* 解放した空きがリストから正常に除去・再利用されるか検証 */
    char *again = my_malloc(SIZE);
    char *other = my_malloc(SIZE);
    bool exact_fit_reused    = again == target;
    bool not_double_allocated = other != NULL && other != again;
    check("ぴったり合う空きがそのまま配り直され、二重には配られない",
          exact_fit_reused && not_double_allocated);

    my_free(other);
    my_free(again);
    my_free(before);
    my_free(after);
}

/* 境界値 (サイズ 0、NULL) に対する動作テスト */
static void check_zero_and_null(void) {
    /* malloc(0) は NULL を返す。規格では有効なポインタを返してもよい */
    check("0バイトの要求は NULL", my_malloc(0) == NULL);

    /* free(NULL) は何もせず安全に終了するかチェック。落ちればこの先の行に来られない */
    my_free(NULL);
    check("NULL の解放で落ちない", true);
}

/* 確保したメモリ領域に対する正常な書き込み・読み戻しテスト */
static void check_write_and_read_back(void) {
    /* 1回の arena (64KB) を超えるサイズを指定し、新規に OS から領域を取得する経路を通す */
    enum { LARGE_SIZE = 200000 };
    const char MARK = 'S';  /* メモリ領域に書き込むテスト用の値 */

    char *large = my_malloc(LARGE_SIZE);
    check("1回の arena を超える確保が成功する", large != NULL);
    if (large == NULL) return;

    /* 端から端まで正しく書けて読み戻せるか検証。領域が足りなければ memset が落ちる */
    memset(large, MARK, LARGE_SIZE);
    bool first_byte_ok = large[0] == MARK;
    bool last_byte_ok  = large[LARGE_SIZE - 1] == MARK;
    check("大きい領域も端まで書ける", first_byte_ok && last_byte_ok);

    my_free(large);
}

/* メモリ領域のアライメントテスト */
static void check_alignment(void) {
    enum { MAX_SIZE = 64 };

    /*
     * 検証を終えるまで解放しない
     * 1個ずつ解放すると空きが元に戻り、毎回同じアドレスが配られてしまう
     */
    void *blocks[MAX_SIZE];
    bool aligned = true;

    for (size_t size = 1; size <= MAX_SIZE; size++) {
        void *p = my_malloc(size);
        blocks[size - 1] = p;

        /* 返されたアドレスが最も厳しい配置基準 max_align_t の倍数になっているかチェック */
        bool at_boundary = (uintptr_t)p % sizeof(max_align_t) == 0;
        if (p == NULL || !at_boundary) aligned = false;
    }
    check("どの大きさでも max_align_t の境界に合う", aligned);

    for (int i = 0; i < MAX_SIZE; i++) my_free(blocks[i]);
}

/* 同時に確保した領域同士が、互いに干渉しないことをテスト */
static void check_blocks_are_independent(void) {
    /* 多彩な境界位置を試すためバラバラな要求サイズを定義 */
    static const size_t SIZES[] = { 1, 15, 17, 31, 33, 63, 65, 97 };
    enum { SIZE_COUNT = sizeof SIZES / sizeof SIZES[0] };

    /* 各ブロックに書き込む値が 1バイトに収まるよう 200個に設定 */
    enum { COUNT = 200 };
    char *blocks[COUNT];
    size_t block_sizes[COUNT];
    bool all_allocated = true;

    /* ブロック番号をそのまま値として敷き詰め、誰が書いたかを後から追えるようにする */
    for (int i = 0; i < COUNT; i++) {
        block_sizes[i] = SIZES[i % SIZE_COUNT];
        blocks[i]      = my_malloc(block_sizes[i]);
        if (blocks[i] == NULL) {
            all_allocated = false;
            break;
        }
        memset(blocks[i], (unsigned char)i, block_sizes[i]);
    }
    check("200個 まとめて確保できる", all_allocated);
    if (!all_allocated) return;

    check("確保した領域同士が重ならない", !has_overlap(blocks, block_sizes, COUNT));
    check("他を確保しても内容が壊れない", !has_broken_content(blocks, block_sizes, COUNT));

    for (int i = 0; i < COUNT; i++) my_free(blocks[i]);
}

/* メモリ解放後の再割り当てテスト */
static void check_reuse(void) {
    enum { ROUNDS = 5000, BLOCK_SIZE = 4096 };

    /* あらかじめフリーリストを拡張し、十分な空き領域を確保しておく */
    for (int i = 0; i < 100; i++) my_free(my_malloc(BLOCK_SIZE));

    /*
     * 返された一意なアドレスの種類数を記録する
     * 距離ではなく種類で見るのは、arena が複数あるとその間隔が mmap 任せで数MB 開き、
     * 距離が「新しく取ったかどうか」を表さないため
     */
    enum { MAX_DISTINCT = 16 };
    char *seen[MAX_DISTINCT];
    int seen_count = 0;
    bool all_allocated = true;

    /* 種類数が上限に届いた時点で不合格が決まるので、5000回を待たずに抜ける */
    for (int i = 0; i < ROUNDS && seen_count < MAX_DISTINCT; i++) {
        char *p = my_malloc(BLOCK_SIZE);
        if (p == NULL) {
            all_allocated = false;
            break;
        }

        if (!already_seen(seen, seen_count, p)) seen[seen_count++] = p;

        /* 次の繰り返しで再利用できるよう即座に解放 */
        my_free(p);
    }

    /* 5000回繰り返してもアドレスの種類数が少量 (16未満) に収まり、再利用されているか判定 */
    check("確保と解放を繰り返しても同じ領域が配り直される",
          all_allocated && seen_count < MAX_DISTINCT);
}

static void check(const char *name, bool ok) {
    checks++;
    if (ok) return;

    failures++;
    printf("NG  %s\n", name);
}

/* 確保した全メモリブロック同士が重複しているか総当たりで検証する */
static bool has_overlap(char **blocks, size_t *sizes, int count) {
    for (int i = 0; i < count; i++) {
        char *start_a = blocks[i], *end_a = blocks[i] + sizes[i];
        for (int j = i + 1; j < count; j++) {
            char *start_b = blocks[j], *end_b = blocks[j] + sizes[j];

            /* ブロック A と ブロック B の領域 (範囲) が重なっているか判定 */
            bool is_overlapping = start_a < end_b && start_b < end_a;
            if (is_overlapping) return true;
        }
    }
    return false;
}

/*
 * 各ブロックが、確保したときに敷き詰めた値をそのまま保っているか検証する
 * 範囲が重なっていなくても、管理情報の書き込み位置がずれれば隣の中身を潰せる
 */
static bool has_broken_content(char **blocks, size_t *sizes, int count) {
    for (int i = 0; i < count; i++) {
        for (size_t k = 0; k < sizes[i]; k++) {
            /* char の符号有無による値の化けを防ぐため unsigned char にキャストして比較 */
            if ((unsigned char)blocks[i][k] != (unsigned char)i) return true;
        }
    }
    return false;
}

/* 配列 seen に p が既に登録されているか */
static bool already_seen(char **seen, int count, char *p) {
    for (int i = 0; i < count; i++) {
        if (seen[i] == p) return true;
    }
    return false;
}
