/* 外部プログラムを起動し、その標準出力をレスポンスとして流す */

#include "cgi.h"
#include "io.h"
#include "response.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

/*
 *  [親プロセス (httpd)]                [子プロセス (CGI)]
 *   書く to_cgi[1]   ==============>  to_cgi[0]   読む (標準入力)
 *   読む from_cgi[0] <==============  from_cgi[1] 書く (標準出力)
 */

/* パイプから1回に読む量 */
#define RELAY_BUF_SIZE 4096

static void run_child(int to_cgi[2], int from_cgi[2], const char *path, const struct HTTPRequest *req);
static void relay_output(int client_fd, int from_cgi_read);
static void export_cgi_env(const struct HTTPRequest *req);

void run_cgi(int client_fd, const struct HTTPRequest *req, const char *path) {
    int to_cgi[2], from_cgi[2];
    if (pipe(to_cgi) < 0) {
        respond_internal_error(client_fd);
        return;
    }
    if (pipe(from_cgi) < 0) {
        close(to_cgi[0]);
        close(to_cgi[1]);
        respond_internal_error(client_fd);
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(to_cgi[0]);
        close(to_cgi[1]);
        close(from_cgi[0]);
        close(from_cgi[1]);
        respond_internal_error(client_fd);
        return;
    }

    if (pid == 0) run_child(to_cgi, from_cgi, path, req);

    /*
     * 子プロセスが使う側は fork で親プロセスにも複製されている。使わないので閉じる
     * 特に to_cgi の読み端は、握ったままだと CGI の標準入力に終端が届かない
     */
    close(to_cgi[0]);
    close(from_cgi[1]);

    close(to_cgi[1]);
    relay_output(client_fd, from_cgi[0]);
    close(from_cgi[0]);
}

/* 標準入出力をパイプに繋ぎ替えてから外部プログラムになる。戻らない */
static void run_child(int to_cgi[2], int from_cgi[2], const char *path, const struct HTTPRequest *req) {
    if (dup2(to_cgi[0], STDIN_FILENO) < 0 || dup2(from_cgi[1], STDOUT_FILENO) < 0) _exit(1);

    close(to_cgi[0]);
    close(to_cgi[1]);
    close(from_cgi[0]);
    close(from_cgi[1]);

    /* SIG_IGN は exec をまたいで残る。出力先が閉じたことを CGI が検知できなくなる */
    signal(SIGPIPE, SIG_DFL);

    export_cgi_env(req);

    execl(path, path, (char *)NULL);
    _exit(1);
}

/* Content-Type などは CGI 自身が出力するので、こちらはステータス行だけ足す */
static void relay_output(int client_fd, int from_cgi_read) {
    respond_cgi_header(client_fd);

    char buf[RELAY_BUF_SIZE];
    for (;;) {
        ssize_t bytes_read = read(from_cgi_read, buf, sizeof buf);
        if (bytes_read == 0) break;
        if (bytes_read < 0) {
            /* 子プロセスの終了による SIGCHLD で中断される。終端と混同しない */
            if (errno == EINTR) continue;
            break;
        }

        if (write_all(client_fd, buf, (size_t)bytes_read) < 0) break;
    }
}

/* RFC 3875 が定める変数のうち、実行に要るものだけ渡す */
static void export_cgi_env(const struct HTTPRequest *req) {
    setenv("REQUEST_METHOD", method_name(req->method), 1);

    /* 区切り文字の判定は復元より前にしかできないので、エンコードのまま渡す */
    setenv("QUERY_STRING", req->query ? req->query : "", 1);
}
