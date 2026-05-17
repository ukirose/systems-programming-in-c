/* プロセスの実行環境を整える */

#include "env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

static void set_signal_action(int sig, void (*handler)(int), int flags);
static void noop_handler(int sig);
static void fail(const char *what);

void setup_signals(void) {
    /* 無視しておくと、相手が切った後の write が EPIPE で戻る。既定では子プロセスが終了させられる */
    set_signal_action(SIGPIPE, SIG_IGN, 0);

    /* 放っておくとゾンビが溜まるので、終了した子プロセスを待たずに捨てる */
    set_signal_action(SIGCHLD, noop_handler, SA_NOCLDWAIT);
}

void drop_privileges(const char *chroot_dir, const char *user, const char *group) {
    if (!user || !group) {
        fprintf(stderr, "--chroot needs both --user and --group\n");
        exit(1);
    }

    /* 隔離すると /etc/passwd も /etc/group も見えなくなるので、先に調べておく */
    struct group *gr = getgrnam(group);
    if (!gr) {
        fprintf(stderr, "no such group: %s\n", group);
        exit(1);
    }

    struct passwd *pw = getpwnam(user);
    if (!pw) {
        fprintf(stderr, "no such user: %s\n", user);
        exit(1);
    }

    if (setgid(gr->gr_gid) < 0) fail("setgid(2)");

    /* setgid は補足グループを触らない。root の補足グループが残ったままになる */
    if (initgroups(user, gr->gr_gid) < 0) fail("initgroups(3)");

    if (chroot(chroot_dir) < 0) fail("chroot(2)");
    if (chdir("/") < 0) fail("chdir(2)");

    /* 手放すと chroot も setgid もできなくなるので最後 */
    if (setuid(pw->pw_uid) < 0) fail("setuid(2)");
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

static void fail(const char *what) {
    fprintf(stderr, "%s failed: %s\n", what, strerror(errno));
    exit(1);
}
