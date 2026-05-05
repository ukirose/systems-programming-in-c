/* プロセスの実行環境を整える */

#pragma once

/* 子プロセスの後始末とクライアント切断の扱いを設定する。失敗したらプロセスを終了する */
void setup_signals(void);
