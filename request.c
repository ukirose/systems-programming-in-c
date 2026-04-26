/* HTTP リクエストの読み取りと解析 */

#include "request.h"
#include <string.h>
#include <unistd.h>

#define HTTP_VERSION_PREFIX "HTTP/1."

static char *take_line(char **rest);
static int parse_request_line(char *line, struct HTTPRequest *req);
static int parse_header_field(char *line, struct HTTPHeaderField *field);
static enum HTTPMethod parse_method(const char *s);
static char *split_at(char *s, char delim);

ssize_t read_request_header(int fd, char *buf, size_t size) {
    size_t filled = 0;

    /* 終端を置く1バイトを残して読む */
    while (filled + 1 < size) {
        ssize_t bytes_read = read(fd, buf + filled, size - filled - 1);

        /* 0 は切断。空行より前に切れたなら不完全なリクエスト */
        if (bytes_read <= 0) return -1;

        filled += (size_t)bytes_read;
        buf[filled] = '\0';

        /* ヘッダの終わりは空行、つまり CRLF が2つ続く位置 */
        if (strstr(buf, "\r\n\r\n")) return (ssize_t)filled;
    }

    return -1;
}

int parse_http_request(char *buf, struct HTTPRequest *req) {
    req->field_count = 0;

    char *rest = buf;
    char *line = take_line(&rest);
    if (!line || parse_request_line(line, req) < 0) return -1;

    for (;;) {
        line = take_line(&rest);
        if (!line) return -1;
        if (*line == '\0') return 0;  /* 空行に達した */

        if (req->field_count >= MAX_HEADER_FIELDS) return -1;
        if (parse_header_field(line, &req->fields[req->field_count]) < 0) return -1;
        req->field_count++;
    }
}

/* rest の先頭1行を切り出す。'\r' を '\0' に替えて終端し、rest を CRLF の次へ進める */
static char *take_line(char **rest) {
    char *line = *rest;

    char *line_end = strstr(line, "\r\n");
    if (!line_end) return NULL;

    *line_end = '\0';
    *rest = line_end + 2;
    return line;
}

/* "GET /index.html HTTP/1.0" を3つに割る */
static int parse_request_line(char *line, struct HTTPRequest *req) {
    req->path = split_at(line, ' ');
    if (!req->path) return -1;

    req->method = parse_method(line);

    char *version = split_at(req->path, ' ');
    if (!version) return -1;

    /* HTTP/1.x 以外は、解析できても応答の形が違うので断る */
    if (strncmp(version, HTTP_VERSION_PREFIX, sizeof HTTP_VERSION_PREFIX - 1) != 0) return -1;

    return 0;
}

/* "Host: example.com" を名前と値に割る。値の前の空白は飛ばす */
static int parse_header_field(char *line, struct HTTPHeaderField *field) {
    char *value = split_at(line, ':');
    if (!value) return -1;

    field->name  = line;
    field->value = value + strspn(value, " \t");
    return 0;
}

/* メソッドは大文字小文字を区別する (RFC 9110 9.1)。小文字の get は GET ではない */
static enum HTTPMethod parse_method(const char *s) {
    if (strcmp(s, "GET")  == 0) return METHOD_GET;
    if (strcmp(s, "HEAD") == 0) return METHOD_HEAD;
    if (strcmp(s, "POST") == 0) return METHOD_POST;

    return METHOD_UNKNOWN;
}

/* delim を最初に見つけた位置で s を切り、後ろ半分の先頭を返す */
static char *split_at(char *s, char delim) {
    char *p = strchr(s, delim);
    if (!p) return NULL;

    *p = '\0';
    return p + 1;
}
