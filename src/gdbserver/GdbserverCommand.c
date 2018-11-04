#define LISTEN_PORT 4444

void GdbserverCommand(void)
{
    int connfd;

#ifdef windows
    WSADATA WinSocketData = {0};
    int err = WSAStartup(0x0202, &WinSocketData);
    if (err != 0) {Ws("Could not start Windows sockets, error code "); Wd(err,1); Fail(".");}
#endif

    if (target_reset()) {
        Fail("Can't reset target!\n");
    }

    Wsl("Target ready, waiting for GDB connection.");
    fprintf(stderr, "%s", "\nInfo : avrchip: hardware has something\n");    //vscode check stderr for start, mimic a openocd process
    Ws("Use 'target remote :"); Wd(LISTEN_PORT,1); Wsl("'");

    connfd = listen_sock(LISTEN_PORT);
    if (connfd < 0) Fail("Listen failed.");

    Wsl("Connection accepted.");

    handle_client(connfd);
}
