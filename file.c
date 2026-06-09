/* URL パスから配信するファイルを解決し、その内容を送り出す */

#include "file.h"
#include "my_string.h"
#include "io.h"
#include <stdio.h>
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

    /*
     * 調べるだけだと、送る時に開き直した先が同じ実体という保証が無い
     * 開いてから fstat で調べ、送信もこの fd から行う
     * O_NOFOLLOW は末尾の要素がリンクなら開かない (途中の要素は辿る)
     * O_NONBLOCK は FIFO で書き込む側を待たないため。通常ファイルの read には効かない
     */
    int fd = open(info->path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return -1;

    struct stat st;
    if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        return -1;
    }

    info->fd            = fd;
    info->size          = st.st_size;
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
    const char *ext = my_strrchr(info->path, '.');

    if (ext) {
        for (size_t i = 0; i < sizeof content_types / sizeof content_types[0]; i++) {
            if (strcasecmp(ext, content_types[i].ext) == 0) return content_types[i].type;
        }
    }

    /* 分からないものをブラウザに解釈させると、意図しない実行につながる */
    return "application/octet-stream";
}

void send_file_body(int fd, const struct FileInfo *info) {
    char buf[COPY_BUF_SIZE];
    for (;;) {
        ssize_t bytes_read = read(info->fd, buf, sizeof buf);
        if (bytes_read <= 0) break;

        /* 相手が切ったあとも読み続けないよう、書けなくなったら止める */
        if (write_all(fd, buf, (size_t)bytes_read) < 0) break;
    }
}
