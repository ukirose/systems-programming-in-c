/* 空きリストをアドレス順に並べて管理するメモリ確保 */

#include "my_stdlib.h"
#include <sys/mman.h>

/*
 * 確保した領域の直前に置く管理情報
 * 共用体にしているのは、どんな型を置いても境界が合うよう大きさを底上げするため
 * 中身は Block 何個分かで数える。バイト数で持つと割り算が散らばる
 */
union Block {
    struct {
        union Block *next;  /* 空きリストの次。アドレスの小さい順に並ぶ */
        size_t units;       /* この Block 自身を含めた大きさ */
    } info;
    max_align_t align;  /* どの型でも通用する最も厳しい境界を持つ型 */
};

/*
 * mmap 1回で取る大きさ。4096 個 = 64KB
 * 小さく刻むとシステムコールが増え、大きく取ると使わない領域が残る
 */
#define ARENA_UNITS 4096

/* 大きさ 0 の番兵。空きが尽きてもリストが空にならないので、場合分けが減る */
static union Block base;

/* アドレス順の循環リスト。直前に見た位置を覚えて、次はそこから探し始める */
static union Block *free_list;

static union Block *request_more_memory(size_t units);

void *my_malloc(size_t size) {
    if (size == 0) return NULL;

    /* 管理情報 1個分を足して、Block 何個分になるかを切り上げる */
    size_t units = (size + sizeof(union Block) - 1) / sizeof(union Block) + 1;

    if (free_list == NULL) {
        base.info.next  = &base;
        base.info.units = 0;
        free_list       = &base;
    }

    union Block *prev = free_list;
    for (union Block *p = prev->info.next; ; prev = p, p = p->info.next) {
        if (p->info.units >= units) {
            if (p->info.units == units) {
                prev->info.next = p->info.next;
            } else {
                /* 後ろ側を切り出す。前側は空きのまま残るので、繋ぎ替えが要らない */
                p->info.units -= units;
                p += p->info.units;
                p->info.units = units;
            }

            free_list = prev;

            /* 管理情報の 1個分先が、呼出側に渡す領域の先頭 */
            return p + 1;
        }

        /* 一周して戻ってきた。足りる空きがどこにも無い */
        if (p == free_list) {
            p = request_more_memory(units);
            if (p == NULL) return NULL;
        }
    }
}

void my_free(void *ptr) {
    if (ptr == NULL) return;

    /* my_malloc が管理情報の 1個分先を返しているので、1個分戻せば元の Block */
    union Block *block = (union Block *)ptr - 1;

    /*
     * アドレス順を保ったまま挿す位置を探す
     * 順に並べておかないと、隣り合う空き同士を見つけられない
     */
    union Block *p = free_list;
    while (!(block > p && block < p->info.next)) {
        /* リストは循環しているので、アドレスが折り返す 1箇所だけ大小関係が逆になる */
        if (p >= p->info.next && (block > p || block < p->info.next)) break;

        p = p->info.next;
    }

    /* 直後の空きと地続きなら 1つに繋げる */
    if (block + block->info.units == p->info.next) {
        block->info.units += p->info.next->info.units;
        block->info.next   = p->info.next->info.next;
    } else {
        block->info.next = p->info.next;
    }

    /* 直前の空きと地続きなら 1つに繋げる */
    if (p + p->info.units == block) {
        p->info.units += block->info.units;
        p->info.next   = block->info.next;
    } else {
        p->info.next = block;
    }

    free_list = p;
}

/*
 * OS から領域を取り、空きリストへ入れる。取れなければ NULL
 * sbrk ではなく mmap を使う。man 2 sbrk が「避けよ」と書いており、POSIX にも無い
 * 決め手は返し方で、sbrk は break の先端しか縮められない
 * 自分の領域より上に glibc の malloc が確保した時点で、こちらは OS へ返せなくなる
 */
static union Block *request_more_memory(size_t units) {
    if (units < ARENA_UNITS) units = ARENA_UNITS;

    /* MAP_ANONYMOUS はファイルではなく素の領域を求める指定。末尾の fd と offset は使わない */
    void *arena = mmap(NULL, units * sizeof(union Block), PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arena == MAP_FAILED) return NULL;

    /*
     * 確保済みの体裁を整えてから my_free に渡すと、繋ぎ込みを 1箇所に寄せられる
     * mmap で取った領域同士は地続きとは限らないが、
     * my_free はアドレスが隣り合うときしか繋げないので、別々のままになる
     */
    union Block *block = arena;
    block->info.units = units;
    my_free(block + 1);

    return free_list;
}
