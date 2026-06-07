/* URL パスから配信するファイルを解決し、その内容を送り出す */

#pragma once

#include <limits.h>
#include <sys/types.h>

struct FileInfo {
    char path[PATH_MAX];  /* docroot と URL パスを繋いだ実際のパス */
    int fd;               /* 開いた読み取り専用の fd。呼出側が close する */
    off_t size;           /* 本文の長さ */
    int is_executable;    /* 立っていれば配信せずに実行する */
};

/*
 * docroot 配下の url_path を開いて調べる。配信できないなら -1
 * 成功したら info->fd が開いたままになる。呼出側が close する
 */
int resolve_file(const char *docroot, const char *url_path, struct FileInfo *info);

/* 拡張子から推測する。分からなければ application/octet-stream */
const char *guess_content_type(const struct FileInfo *info);

/* resolve_file が開いた中身を fd へ書き出す */
void send_file_body(const struct FileInfo *info, int fd);
