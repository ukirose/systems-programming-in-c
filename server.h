/* 待ち受けソケットを作り、接続を受ける */

#pragma once

/* 指定ポートで待ち受けるソケットを返す。開けない場合はプロセスを終了する */
int create_server_socket(const char *port);

/* 接続を受け入れ続ける。戻らない */
void accept_client_connections(int server_fd);
