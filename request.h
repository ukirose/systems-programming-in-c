/* HTTP リクエストの読み取りと解析 */

#pragma once

#include <stddef.h>
#include <sys/types.h>

/*
 * ヘッダ全体をこのバッファ1つに収める。上限が無いとメモリを使い切られる
 * 普通のリクエストは 1KB 未満。メモリ 1ページに収まる大きさで、巨大な Cookie だけが 400 になる
 */
#define MAX_REQUEST_HEADER_SIZE 4096

/* 受け付けるヘッダ行の数。1行が 128バイトより短ければ、バッファより先にこちらが効く */
#define MAX_HEADER_FIELDS 32

enum HTTPMethod {
    METHOD_GET,
    METHOD_HEAD,
    METHOD_POST,
    METHOD_UNKNOWN,  /* 解析はできたが対応していないメソッド */
};

struct HTTPHeaderField {
    char *name;   /* 読み込んだバッファの中を指す */
    char *value;  /* 同上。値の前の空白は読み飛ばしてある */
};

struct HTTPRequest {
    enum HTTPMethod method;
    char *path;                                   /* 読み込んだバッファの中を指す */
    char *query;                                  /* '?' 以降。無ければ NULL。%XX は復元しない */
    struct HTTPHeaderField fields[MAX_HEADER_FIELDS];
    int field_count;
};

/*
 * fd からヘッダの終わり (空行) までを buf へ読み、'\0' を置いて読めた長さを返す
 * 同じ read で届いたボディの先頭も buf に入り、長さに含まれる
 * 終端に 1バイト使うので、読めるのは size - 1 まで。上限超過や切断は -1
 */
ssize_t read_request_header(int fd, char *buf, size_t size);

/* 列挙を CGI へ渡す文字列に戻す。METHOD_UNKNOWN は ""。返る領域は静的で、解放しない */
const char *method_name(enum HTTPMethod method);

/*
 * buf を '\0' で区切りながら解析する。不正な書式や、ボディの上限超過も -1
 * req の各ポインタは buf の中を指すので、buf より長生きさせてはいけない
 */
int parse_http_request(char *buf, struct HTTPRequest *req);
