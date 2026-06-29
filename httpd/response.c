/* HTTP レスポンスを組み立てて書き出す */

#include "response.h"
#include "my_string.h"
#include "my_stdio.h"
#include "io.h"

#define SERVER_NAME "httpd/0.1"

/* ステータス行と共通ヘッダが収まる大きさ。実測の最長は 405 応答の 137バイト */
#define HEADER_BUF_SIZE 512

static void respond_with_message(int fd, const char *status, const char *extra_headers, const char *message);
static void write_headers(int fd, const char *status, const char *extra_headers,
                          const char *content_type, off_t content_length);

void respond_with_file(int fd, const struct FileInfo *info) {
    respond_with_file_header(fd, info);
    send_file_body(fd, info);
}

void respond_with_file_header(int fd, const struct FileInfo *info) {
    write_headers(fd, "200 OK", "", guess_content_type(info), info->size);
}

void respond_not_found(int fd) {
    respond_with_message(fd, "404 Not Found", "", "Not Found\n");
}

void respond_bad_request(int fd) {
    respond_with_message(fd, "400 Bad Request", "", "Bad Request\n");
}

void respond_method_not_allowed(int fd) {
    /* 405 には Allow が要る (RFC 9110 15.5.6) */
    respond_with_message(fd, "405 Method Not Allowed", "Allow: GET, HEAD\r\n", "Method Not Allowed\n");
}

void respond_not_implemented(int fd) {
    respond_with_message(fd, "501 Not Implemented", "", "Not Implemented\n");
}

void respond_internal_error(int fd) {
    respond_with_message(fd, "500 Internal Server Error", "", "Internal Server Error\n");
}

void respond_cgi_header(int fd) {
    static const char header[] =
        "HTTP/1.0 200 OK\r\n"
        "Server: " SERVER_NAME "\r\n"
        "Connection: close\r\n";

    write_all(fd, header, sizeof header - 1);
}

/* 短い本文をその場で組み立てて送る */
static void respond_with_message(int fd, const char *status, const char *extra_headers, const char *message) {
    size_t length = my_strlen(message);

    write_headers(fd, status, extra_headers, "text/plain", (off_t)length);
    write_all(fd, message, length);
}

/* extra_headers は「名前: 値\r\n」を並べた塊。無ければ "" */
static void write_headers(int fd, const char *status, const char *extra_headers,
                          const char *content_type, off_t content_length) {
    char header[HEADER_BUF_SIZE];

    /* off_t の幅は環境によって違うので、書式を固定できる long long に寄せる */
    int len = my_snprintf(header, sizeof header,
        "HTTP/1.0 %s\r\n"
        "Server: %s\r\n"
        "Connection: close\r\n"
        "Content-Length: %lld\r\n"
        "Content-Type: %s\r\n"
        "%s"
        "\r\n",
        status, SERVER_NAME, (long long)content_length, content_type, extra_headers);

    /* 収まらなければ my_snprintf は「書きたかった長さ」を返す。そのまま渡すと配列の外を読む */
    if (len < 0 || (size_t)len >= sizeof header) return;

    write_all(fd, header, (size_t)len);
}
