#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include "ThreadPool.h"

#pragma comment(lib, "Ws2_32.lib")

class TcpServer {
public:
    TcpServer(int port, size_t threadCount);
    ~TcpServer();

    void start();

private:
    SOCKET serverSocket_;
    int port_;
    ThreadPool pool_;
};

