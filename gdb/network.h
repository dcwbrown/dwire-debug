#pragma once

int listen_sock(int port);
void handle_client(int connfd);
