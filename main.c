/* 起動時の引数を見てサーバーを立ち上げる */

#include "server.h"
#include <stdio.h>
#include <stdlib.h>

#define USAGE "Usage: %s <port>\n"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, USAGE, argv[0]);
        exit(1);
    }

    int server_fd = create_server_socket(argv[1]);
    accept_client_connections(server_fd);

    return 0;
}
