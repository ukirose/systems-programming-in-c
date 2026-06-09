/* 待ち受けソケットを作り、接続を受ける */

#include "server.h"
#include "request.h"
#include "response.h"
#include "file.h"
#include "log.h"
#include "cgi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>

/* accept を待つ接続を並べておける数。溢れた接続は拒否される */
#define MAX_BACKLOG 5

static int try_listen(struct addrinfo *candidates, int family);
static void serve_client(int client_fd, const char *docroot);

int create_server_socket(const char *port) {
    /* 指定しなかったメンバは 0 になる。getaddrinfo はそれを「指定なし」と読む */
    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,    /* IPv4 / IPv6 の両方に対応 */
        .ai_socktype = SOCK_STREAM,  /* TCP */
        .ai_flags    = AI_PASSIVE,   /* bind 用のアドレスを要求 */
    };

    /* 第1引数の NULL は、このホストの全てのアドレスで待ち受ける指定 */
    struct addrinfo *candidates;
    int err = getaddrinfo(NULL, port, &hints, &candidates);
    /* getaddrinfo は errno を使わず独自のコードを返す */
    if (err != 0) log_error_and_exit("getaddrinfo(): %s", gai_strerror(err));

    /*
     * 候補は複数返る。IPv6 を先に試すのは、その1本で IPv4 の接続も受けられるため
     * 返る順は決まっていないので、選ぶ側で順序を決める
     */
    int server_fd = try_listen(candidates, AF_INET6);
    if (server_fd < 0) server_fd = try_listen(candidates, AF_UNSPEC);

    freeaddrinfo(candidates);

    if (server_fd < 0) log_error_and_exit("failed to listen on port %s", port);

    return server_fd;
}

/* family に合う候補を順に試し、最初に listen まで通った fd を返す。AF_UNSPEC はすべてを試す */
static int try_listen(struct addrinfo *candidates, int family) {
    for (struct addrinfo *ai = candidates; ai != NULL; ai = ai->ai_next) {
        if (family != AF_UNSPEC && ai->ai_family != family) continue;

        int fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;

        /*
         * 直前の接続が TIME-WAIT に残っていると bind が弾かれる
         * この指定が無いと、止めてから数十秒は同じポートで起動し直せない
         */
        int on = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);

        int bind_result = bind(fd, ai->ai_addr, ai->ai_addrlen);
        if (bind_result < 0) {
            close(fd);
            continue;
        }

        int listen_result = listen(fd, MAX_BACKLOG);
        if (listen_result < 0) {
            close(fd);
            continue;
        }

        return fd;
    }

    return -1;
}

void accept_client_connections(int server_fd, const char *docroot) {
    for (;;) {
        /* accept は接続ごとに新しい fd を返す。server_fd は待ち受け専用のまま残る */
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            /* シグナルで中断されただけなので、待ち受けに戻る */
            if (errno == EINTR) continue;

            log_error_and_exit("accept(2) failed: %s", strerror(errno));
        }

        pid_t pid = fork();
        if (pid < 0) {
            log_error_and_exit("fork(2) failed: %s", strerror(errno));
        }

        if (pid == 0) {
            /* 子プロセスが待ち受けソケットを握ったままだと、親プロセスを止めてもポートが解放されない */
            close(server_fd);

            serve_client(client_fd, docroot);
            close(client_fd);
            exit(0);
        }

        /* 応答は子プロセスに任せてあり、親プロセスが持ち続けると接続ごとに fd が漏れる */
        close(client_fd);
    }
}

static void serve_client(int client_fd, const char *docroot) {
    char buf[MAX_REQUEST_HEADER_SIZE];
    struct HTTPRequest req;

    if (read_request_header(client_fd, buf, sizeof buf) < 0
        || parse_http_request(buf, &req) < 0) {
        respond_bad_request(client_fd);
        return;
    }

    if (req.method == METHOD_UNKNOWN) {
        respond_not_implemented(client_fd);
        return;
    }

    struct FileInfo info;
    if (resolve_file(docroot, req.path, &info) < 0) {
        respond_not_found(client_fd);
        return;
    }

    if (info.is_executable) {
        /* 実行するのは path のほう。中身は読まないので先に閉じる */
        close(info.fd);
        run_cgi(client_fd, &req, info.path);
        return;
    }

    /* 静的ファイルは送られてきたデータを処理できないので POST を断る */
    if      (req.method == METHOD_POST) respond_method_not_allowed(client_fd);
    else if (req.method == METHOD_HEAD) respond_with_file_header(client_fd, &info);
    else                                respond_with_file(client_fd, &info);

    close(info.fd);
}
