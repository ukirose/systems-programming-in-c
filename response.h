/* HTTP レスポンスのステータス行とヘッダを書く */

#pragma once

#include "file.h"

/* 200 とヘッダを書き、続けてファイルの中身を送る */
void respond_with_file(int fd, const struct FileInfo *info);

void respond_not_found(int fd);
void respond_bad_request(int fd);
