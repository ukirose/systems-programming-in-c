/* エラーを記録してプロセスを終了する */

#pragma once

/*
 * 以降の記録を syslog へ向ける
 * chroot すると /dev/log が見えなくなるので、隔離より前に呼ぶ
 */
void log_to_syslog(const char *ident);

/* 記録してプロセスを終了する。戻らない */
void log_error_and_exit(const char *fmt, ...);
