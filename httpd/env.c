/* プロセスの実行環境を整える */

#include "env.h"
#include "log.h"
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>

static void set_signal_action(int sig, void (*handler)(int), int flags);
static void noop_handler(int sig);
static void redirect_stdio_to_null(void);

void setup_signals(void) {
    /* 無視しておくと、相手が切った後の write が EPIPE で戻る。既定では子プロセスが終了させられる */
    set_signal_action(SIGPIPE, SIG_IGN, 0);

    /* 放っておくとゾンビが溜まるので、終了した子プロセスを待たずに捨てる */
    set_signal_action(SIGCHLD, noop_handler, SA_NOCLDWAIT);
}

void drop_privileges(const char *chroot_dir, const char *user, const char *group) {
    if (!user || !group) {
        log_error_and_exit("--chroot needs both --user and --group");
    }

    /* 隔離すると /etc/passwd も /etc/group も見えなくなるので、先に調べておく */
    struct group *gr = getgrnam(group);
    if (!gr) log_error_and_exit("no such group: %s", group);

    struct passwd *pw = getpwnam(user);
    if (!pw) log_error_and_exit("no such user: %s", user);

    if (setgid(gr->gr_gid) < 0) log_error_and_exit("setgid(2) failed: %s", strerror(errno));

    /* setgid は補足グループを触らない。root の補足グループが残ったままになる */
    if (initgroups(user, gr->gr_gid) < 0) log_error_and_exit("initgroups(3) failed: %s", strerror(errno));

    if (chroot(chroot_dir) < 0) log_error_and_exit("chroot(2) failed: %s", strerror(errno));
    if (chdir("/") < 0) log_error_and_exit("chdir(2) failed: %s", strerror(errno));

    /* 手放すと chroot も setgid もできなくなるので最後 */
    if (setuid(pw->pw_uid) < 0) log_error_and_exit("setuid(2) failed: %s", strerror(errno));
}

static void set_signal_action(int sig, void (*handler)(int), int flags) {
    struct sigaction act;
    act.sa_handler = handler;
    act.sa_flags   = flags;
    sigemptyset(&act.sa_mask);

    if (sigaction(sig, &act, NULL) < 0) {
        log_error_and_exit("sigaction(%d) failed: %s", sig, strerror(errno));
    }
}

void become_daemon(void) {
    /* 親プロセスが終わるとシェルに制御が戻り、残った子プロセスは init に引き取られる */
    pid_t pid = fork();
    if (pid < 0) log_error_and_exit("fork(2) failed: %s", strerror(errno));
    if (pid > 0) _exit(0);

    /* 新しいセッションのリーダーになり、制御端末から切り離される */
    if (setsid() < 0) log_error_and_exit("setsid(2) failed: %s", strerror(errno));

    /* カレントディレクトリがそこを指したままだと、そのファイルシステムを外せない */
    if (chdir("/") < 0) log_error_and_exit("chdir(2) failed: %s", strerror(errno));

    redirect_stdio_to_null();
}

/* SA_NOCLDWAIT が効くのはハンドラが SIG_IGN でないときなので、何もしない関数を置く */
static void noop_handler(int sig) {
    (void)sig;
}

/* 端末が無いのに書き込むと、繋ぎ直された先を壊しかねない */
static void redirect_stdio_to_null(void) {
    int fd = open("/dev/null", O_RDWR);
    if (fd < 0) log_error_and_exit("open(/dev/null) failed: %s", strerror(errno));

    if (dup2(fd, STDIN_FILENO) < 0 || dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0) {
        log_error_and_exit("dup2(2) failed: %s", strerror(errno));
    }

    /* fd 自身が 0,1,2 のどれかなら、閉じると今繋いだ先が消える */
    if (fd > STDERR_FILENO) close(fd);
}
