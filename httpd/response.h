/* HTTP レスポンスを組み立てて書き出す */

#pragma once

#include "file.h"

/* 200 とヘッダを書き、続けてファイルの中身を送る */
void respond_with_file(int fd, const struct FileInfo *info);

/* 200 とヘッダだけを書く。HEAD は GET と同じヘッダを返し、本文だけ省く */
void respond_with_file_header(int fd, const struct FileInfo *info);

void respond_not_found(int fd);
void respond_bad_request(int fd);
void respond_method_not_allowed(int fd);
void respond_not_implemented(int fd);
void respond_internal_error(int fd);

/*
 * ステータス行と Server と Connection だけを書く。ヘッダを閉じる空行は書かない
 * Content-Type と終端の空行は CGI 自身の出力が担う
 */
void respond_cgi_header(int fd);
