/* URL パスから配信するファイルを解決し、その内容を送り出す */

#pragma once

#include <limits.h>
#include <sys/types.h>

struct FileInfo {
    char path[PATH_MAX];  /* docroot と URL パスを繋いだ実際のパス */
    off_t size;           /* 本文の長さ */
    int is_executable;    /* 立っていれば配信せずに実行する */
};

/* docroot 配下の url_path を調べる。配信できないなら -1 */
int resolve_file(const char *docroot, const char *url_path, struct FileInfo *info);

/* 拡張子から推測する。分からなければ application/octet-stream */
const char *guess_content_type(const struct FileInfo *info);

/* 中身を fd へ書き出す。開けなかった場合は何も書かない */
void send_file_body(const struct FileInfo *info, int fd);
