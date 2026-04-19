/* HTTP レスポンスのステータス行とヘッダを書く */

#include "response.h"
#include "io.h"
#include <stdio.h>
#include <string.h>

#define SERVER_NAME "httpd/0.1"

/* ステータス行と共通ヘッダが収まる大きさ */
#define HEADER_BUF_SIZE 512

static void respond_with_message(int fd, const char *status, const char *message);
static void write_headers(int fd, const char *status, const char *content_type, off_t content_length);

void respond_with_file(int fd, const struct FileInfo *info) {
    write_headers(fd, "200 OK", guess_content_type(info), info->size);
    send_file_body(info, fd);
}

void respond_not_found(int fd) {
    respond_with_message(fd, "404 Not Found", "Not Found\n");
}

void respond_bad_request(int fd) {
    respond_with_message(fd, "400 Bad Request", "Bad Request\n");
}

/* 短い本文をその場で組み立てて送る */
static void respond_with_message(int fd, const char *status, const char *message) {
    size_t length = strlen(message);

    write_headers(fd, status, "text/plain", (off_t)length);
    write_all(fd, message, length);
}

static void write_headers(int fd, const char *status, const char *content_type, off_t content_length) {
    char header[HEADER_BUF_SIZE];

    /* off_t の幅は環境によって違うので、書式を固定できる long long に寄せる */
    int len = snprintf(header, sizeof header,
        "HTTP/1.0 %s\r\n"
        "Server: %s\r\n"
        "Connection: close\r\n"
        "Content-Length: %lld\r\n"
        "Content-Type: %s\r\n"
        "\r\n",
        status, SERVER_NAME, (long long)content_length, content_type);

    /* 収まらなければ snprintf は「書きたかった長さ」を返す。そのまま渡すと配列の外を読む */
    if (len < 0 || (size_t)len >= sizeof header) return;

    write_all(fd, header, (size_t)len);
}
