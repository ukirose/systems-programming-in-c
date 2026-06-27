/* 外部プログラムを起動し、その標準出力をレスポンスとして流す */

#pragma once

#include "request.h"

/*
 * path を実行し、出力を fd へ中継する
 * pipe(2) か fork(2) に失敗したら 500。exec の失敗はステータス行を書いた後なので伝えられない
 */
void run_cgi(int fd, const struct HTTPRequest *req, const char *path);
