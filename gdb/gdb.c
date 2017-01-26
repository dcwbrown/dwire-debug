#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "rsp.h"
#include "network.h"
#include "target.h"

#define LISTEN_PORT 4444

int GDB_RSP()
{
    int connfd;

    if (target_reset()) {
        fprintf(stderr, "Can't reset target!\n");
        return -1;
    }

    printf("Target ready, waiting for GDB connection\n");
    printf("Use 'target remote :%d'\n", LISTEN_PORT);

    connfd = listen_sock(LISTEN_PORT);
    printf("Connection accepted\n");

    handle_client(connfd);

    return 0;
}
