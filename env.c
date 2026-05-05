/* プロセスの実行環境を整える */

#include "env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

static void set_signal_action(int sig, void (*handler)(int), int flags);
static void noop_handler(int sig);

void setup_signals(void) {
    /* 無視しておくと、相手が切った後の write が EPIPE で戻る。既定では子プロセスが終了させられる */
    set_signal_action(SIGPIPE, SIG_IGN, 0);

    /* 放っておくとゾンビが溜まるので、終了した子プロセスを待たずに捨てる */
    set_signal_action(SIGCHLD, noop_handler, SA_NOCLDWAIT);
}

static void set_signal_action(int sig, void (*handler)(int), int flags) {
    struct sigaction act;
    act.sa_handler = handler;
    act.sa_flags   = flags;
    sigemptyset(&act.sa_mask);

    if (sigaction(sig, &act, NULL) < 0) {
        fprintf(stderr, "sigaction(%d) failed: %s\n", sig, strerror(errno));
        exit(1);
    }
}

/* SA_NOCLDWAIT が効くのはハンドラが SIG_IGN でないときなので、何もしない関数を置く */
static void noop_handler(int sig) {
    (void)sig;
}
