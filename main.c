/* 起動時の引数を見てサーバーを立ち上げる */

#include "server.h"
#include "env.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#define SERVER_IDENT "httpd"
#define DEFAULT_PORT "80"
#define USAGE "Usage: %s [--port=n] [--debug] [--chroot --user=u --group=g] <docroot>\n"

struct Options {
    const char *port;
    const char *docroot;
    const char *user;
    const char *group;
    int do_chroot;
    int debug;      /* 立っていればデーモン化せず、記録も端末へ出す */
};

static void parse_options(int argc, char *argv[], struct Options *opts);
static const char *option_value(const char *arg, const char *name);
static int is_valid_port(const char *port);

int main(int argc, char *argv[]) {
    struct Options opts = { .port = DEFAULT_PORT };
    parse_options(argc, argv, &opts);

    /* デーモン化で chdir("/") するので、相対パスのまま持ち回れない */
    char resolved[PATH_MAX];
    if (!realpath(opts.docroot, resolved)) {
        log_error_and_exit("realpath(%s) failed: %s", opts.docroot, strerror(errno));
    }

    /*
     * 以降の失敗はすべてここへ記録する
     * chroot すると /dev/log が見えなくなるので、隔離より前に開いておく
     */
    if (!opts.debug) log_to_syslog(SERVER_IDENT);

    setup_signals();

    /* 1024 番未満の bind には特権が要る。権限を落とす前に済ませる */
    int server_fd = create_server_socket(opts.port);

    /* 標準入出力を /dev/null へ向けるので、隔離より前でないと開けない */
    if (!opts.debug) become_daemon();

    const char *docroot = resolved;
    if (opts.do_chroot) {
        drop_privileges(resolved, opts.user, opts.group);

        /* 隔離後はドキュメントルートが '/' になる */
        docroot = "";
    }

    accept_client_connections(server_fd, docroot);

    return 0;
}

static void parse_options(int argc, char *argv[], struct Options *opts) {
    int i;
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        const char *value;

        if      ((value = option_value(argv[i], "--port")))  opts->port  = value;
        else if ((value = option_value(argv[i], "--user")))  opts->user  = value;
        else if ((value = option_value(argv[i], "--group"))) opts->group = value;
        else if (strcmp(argv[i], "--chroot") == 0)           opts->do_chroot = 1;
        else if (strcmp(argv[i], "--debug") == 0)            opts->debug = 1;
        else if (strcmp(argv[i], "--help") == 0) {
            fprintf(stdout, USAGE, argv[0]);
            exit(0);
        }
        else {
            fprintf(stderr, USAGE, argv[0]);
            exit(1);
        }
    }

    /* オプションの後に残るのはドキュメントルート1つだけ */
    if (i != argc - 1) {
        fprintf(stderr, USAGE, argv[0]);
        exit(1);
    }

    opts->docroot = argv[i];

    /* getaddrinfo は範囲外のポートの下位 16ビットしか見ない。99999 は 34463 になる */
    if (!is_valid_port(opts->port)) {
        fprintf(stderr, "port must be 1..65535: %s\n", opts->port);
        exit(1);
    }
}

/* 十進数だけで、1 から 65535 に収まるか */
static int is_valid_port(const char *port) {
    if (*port == '\0') return 0;

    long value = 0;
    for (const char *p = port; *p; p++) {
        if (*p < '0' || *p > '9') return 0;

        value = value * 10 + (*p - '0');
        if (value > 65535) return 0;
    }

    return value >= 1;
}

/* "--port=8080" が name と一致すれば "8080" を返す。値を空けて書く形は受けない */
static const char *option_value(const char *arg, const char *name) {
    size_t name_len = strlen(name);

    if (strncmp(arg, name, name_len) != 0 || arg[name_len] != '=') return NULL;

    return arg + name_len + 1;
}
