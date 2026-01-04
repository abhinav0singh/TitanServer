#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

class TcpServer {
public:
    TcpServer(int port);
    ~TcpServer();

    void start();

private:
    SOCKET serverSocket_;
    int port_;
};
