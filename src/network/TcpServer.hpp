#pragma once
#include <netinet/in.h>
#include <sys/socket.h>

class TcpServer {
public:
    explicit TcpServer(int port);
    ~TcpServer();
    void start();

private:
    int server_fd;
    int port;
};