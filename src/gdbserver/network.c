#define BUFSZ 1024

int listen_sock(int port)
{
    int listenfd;
    int yes;
    struct sockaddr_in serv_addr = {0};

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    //listenfd = WSASocket(AF_INET, SOCK_STREAM, 0,0,0,0);

    if (listenfd < 0) {
        PrintLastError("listenfd");
        return -1;
    }

    yes = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(int)) < 0) {
        PrintLastError("setsockopt");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        PrintLastError("bind");
        return -1;
    }

    if (listen(listenfd, 10) < 0) {
        PrintLastError("listen");
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
            Ws("Error reading command. Error code: "); Wd(r,1); Fail("!");
        }
        Ws("Got: "); Ws(cmd); Wl();

        if (cmd[0] == 'k') break;  // gdb quitting

        handle_command(connfd, cmd);
    }

    Close((FileHandle)connfd);

    return;
}

