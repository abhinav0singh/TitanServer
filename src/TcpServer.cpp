#include "TcpServer.h"
#include "Logger.h"
#include "HttpParser.h"
#include "HttpHandler.h"

#include <string>

TcpServer::TcpServer(int port)
    : serverSocket_(INVALID_SOCKET), port_(port) {

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }

    serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket_ == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Socket creation failed");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverSocket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(serverSocket_);
        WSACleanup();
        throw std::runtime_error("Bind failed");
    }

    if (listen(serverSocket_, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(serverSocket_);
        WSACleanup();
        throw std::runtime_error("Listen failed");
    }

    Logger::instance().info("Server listening on port " + std::to_string(port_));
}

TcpServer::~TcpServer() {
    closesocket(serverSocket_);
    WSACleanup();
}

void TcpServer::start() {
    while (true) {
        SOCKET clientSocket = accept(serverSocket_, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            Logger::instance().error("Accept failed");
            continue;
        }

        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';

            std::string rawRequest(buffer);

            HttpRequest request = HttpParser::parse(rawRequest);
            HttpResponse response = HttpHandler::handle(request);

            std::string rawResponse = response.toString();
            send(clientSocket, rawResponse.c_str(),
                static_cast<int>(rawResponse.size()), 0);
        }

        closesocket(clientSocket);
    }
}
