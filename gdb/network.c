#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

#include "rsp.h"

#define BUFSZ 1024

int listen_sock(int port)
{
    int listenfd;
    int yes;
    struct sockaddr_in serv_addr = {0};

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd < 0) {
        perror("listenfd");
        return -1;
    }

    yes = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        perror("setsockopt");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(listenfd, 10) < 0) {
        perror("listen");
        return -1;
    }

    return accept(listenfd, (struct sockaddr *)NULL, NULL);
}

void handle_client(int connfd)
{
    char cmd[BUFSZ];
    ssize_t r;

    while (1) {
        r = read_command(connfd, cmd, sizeof(cmd));
        if (r < 1) {
            fprintf(stderr, "Error reading command '%zd'!", r);
        }
        printf("Got: %zu '%s'\n", r, cmd);

        handle_command(connfd, cmd);
    }

    close(connfd);

    return;
}

