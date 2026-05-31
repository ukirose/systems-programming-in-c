/* 受け取った環境変数と、標準入力に来たバイト数を返す確認用のプログラム */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    printf("Content-Type: text/plain\r\n\r\n");

    const char *method = getenv("REQUEST_METHOD");
    const char *query  = getenv("QUERY_STRING");
    printf("method=%s\n", method ? method : "");
    printf("query=%s\n", query ? query : "");

    /* 標準入力に来たものをそのまま数えて返す */
    fflush(stdout);
    char buf[4096];
    long total = 0;
    ssize_t n;
    while ((n = read(STDIN_FILENO, buf, sizeof buf)) > 0) total += n;
    printf("body=%ld\n", total);

    return 0;
}
