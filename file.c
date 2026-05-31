/* URL パスから配信するファイルを解決し、その内容を送り出す */

#include "file.h"
#include "io.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* 1回の read(2) で読む量。小さいほどシステムコールの回数が増える */
#define COPY_BUF_SIZE 4096

static int is_safe_path(const char *url_path);

int resolve_file(const char *docroot, const char *url_path, struct FileInfo *info) {
    if (!is_safe_path(url_path)) return -1;

    /* url_path は '/' で始まるので、間に区切りを挟まない */
    int len = snprintf(info->path, sizeof info->path, "%s%s", docroot, url_path);
    if (len < 0 || (size_t)len >= sizeof info->path) return -1;

    /* 末尾の要素がリンクなら lstat は辿らないので S_ISREG が偽になる。途中の要素は辿る */
    struct stat st;
    if (lstat(info->path, &st) < 0) return -1;

    /* FIFO は書き込む側が現れるまで open が返らず、ディレクトリは read が EISDIR で落ちる */
    if (!S_ISREG(st.st_mode)) return -1;

    info->size = st.st_size;
    info->is_executable = (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0;
    return 0;
}

/* 絶対パスであること、経路の要素に ".." が無いことを確かめる */
static int is_safe_path(const char *url_path) {
    if (url_path[0] != '/') return 0;

    /* 経路の要素は必ず '/' の後から始まる。終端に当たれば && が短絡するので範囲外は読まない */
    for (const char *p = url_path; *p; p++) {
        if (*p != '/') continue;
        if (p[1] == '.' && p[2] == '.' && (p[3] == '/' || p[3] == '\0')) return 0;
    }

    return 1;
}

static const struct {
    const char *ext;
    const char *type;
} content_types[] = {
    { ".html", "text/html"  },
    { ".htm",  "text/html"  },
    { ".css",  "text/css"   },
    { ".txt",  "text/plain" },
    { ".png",  "image/png"  },
    { ".jpg",  "image/jpeg" },
    { ".gif",  "image/gif"  },
};

const char *guess_content_type(const struct FileInfo *info) {
    const char *ext = strrchr(info->path, '.');

    if (ext) {
        for (size_t i = 0; i < sizeof content_types / sizeof content_types[0]; i++) {
            if (strcasecmp(ext, content_types[i].ext) == 0) return content_types[i].type;
        }
    }

    /* 分からないものをブラウザに解釈させると、意図しない実行につながる */
    return "application/octet-stream";
}

void send_file_body(const struct FileInfo *info, int fd) {
    int file_fd = open(info->path, O_RDONLY);
    if (file_fd < 0) return;

    char buf[COPY_BUF_SIZE];
    for (;;) {
        ssize_t bytes_read = read(file_fd, buf, sizeof buf);
        if (bytes_read <= 0) break;

        /* 相手が切ったあとも読み続けないよう、書けなくなったら止める */
        if (write_all(fd, buf, (size_t)bytes_read) < 0) break;
    }

    close(file_fd);
}
