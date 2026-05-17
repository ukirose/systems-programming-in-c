/* 起動時の引数を見てサーバーを立ち上げる */

#include "server.h"
#include "env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PORT "80"
#define USAGE "Usage: %s [--port=n] [--chroot --user=u --group=g] <docroot>\n"

struct Options {
    const char *port;
    const char *docroot;
    const char *user;
    const char *group;
    int do_chroot;
};

static void parse_options(int argc, char *argv[], struct Options *opts);
static const char *option_value(const char *arg, const char *name);

int main(int argc, char *argv[]) {
    struct Options opts = { .port = DEFAULT_PORT };
    parse_options(argc, argv, &opts);

    setup_signals();

    /* 1024 番未満の bind には特権が要る。権限を落とす前に済ませる */
    int server_fd = create_server_socket(opts.port);

    const char *docroot = opts.docroot;
    if (opts.do_chroot) {
        drop_privileges(opts.docroot, opts.user, opts.group);

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
}

/* "--port=8080" が name と一致すれば "8080" を返す。値を空けて書く形は受けない */
static const char *option_value(const char *arg, const char *name) {
    size_t name_len = strlen(name);

    if (strncmp(arg, name, name_len) != 0 || arg[name_len] != '=') return NULL;

    return arg + name_len + 1;
}
