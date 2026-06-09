/* エラーを記録してプロセスを終了する */

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>

/* 1行のエラーが収まる大きさ。溢れたら切り詰める */
#define LOG_BUF_SIZE 1024

/* デーモン化すると標準エラー出力が /dev/null へ向くので、記録先を切り替える */
static int use_syslog = 0;

void redirect_log_to_syslog(const char *ident) {
    /* chroot 後ではソケットを開けないので、LOG_NDELAY で今すぐ開かせる */
    openlog(ident, LOG_PID | LOG_NDELAY, LOG_DAEMON);
    use_syslog = 1;
}

void log_error_and_exit(const char *fmt, ...) {
    char message[LOG_BUF_SIZE];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(message, sizeof message, fmt, ap);
    va_end(ap);

    if (use_syslog) syslog(LOG_ERR, "%s", message);
    else            fprintf(stderr, "%s\n", message);

    exit(1);
}
