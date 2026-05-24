/* プロセスの実行環境を整える */

#pragma once

/* 子プロセスの後始末とクライアント切断の扱いを設定する。失敗したらプロセスを終了する */
void setup_signals(void);

/*
 * chroot_dir に隔離してから user/group の権限へ落とす
 * 一度落とすと戻せない。失敗したらプロセスを終了する
 */
void drop_privileges(const char *chroot_dir, const char *user, const char *group);

/*
 * 端末から切り離してバックグラウンドで動かす。標準入出力は /dev/null へ向ける
 * /dev/null を開くので drop_privileges より前に呼ぶ。失敗したらプロセスを終了する
 */
void become_daemon(void);
