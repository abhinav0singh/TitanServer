#include "TcpServer.h"
#include "Logger.h"
#include "HttpParser.h"
#include "HttpHandler.h"

#include <string>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace {

    // Header lookup that does not care about capitalisation.
    std::string headerValue(const HttpRequest& req, const std::string& name) {
        for (const auto& [k, v] : req.headers) {
            if (k.size() != name.size()) continue;
            bool same = true;
            for (size_t i = 0; i < k.size(); ++i) {
                if (std::tolower((unsigned char)k[i]) !=
                    std::tolower((unsigned char)name[i])) { same = false; break; }
            }
            if (same) return v;
        }
        return "";
    }

    bool wantsKeepAlive(const HttpRequest& req) {
        std::string conn = headerValue(req, "Connection");
        std::transform(conn.begin(), conn.end(), conn.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });

        // HTTP/1.1 keeps the connection open unless told otherwise.
        // HTTP/1.0 closes it unless told otherwise.
        if (req.version == "HTTP/1.1") return conn != "close";
        return conn == "keep-alive";
    }

    constexpr int  IDLE_TIMEOUT_MS = 2000;  // free the worker if client goes quiet
    constexpr int  MAX_REQUESTS_PER_CONN = 1000;  // recycle threads eventually

} // namespace

TcpServer::TcpServer(int port, size_t threadCount)
    : serverSocket_(INVALID_SOCKET),
    port_(port),
    pool_(threadCount) {

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

    Logger::instance().info(
        "Server listening on port " + std::to_string(port_) +
        " with thread pool"
    );
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

        // An idle client must not pin a worker thread forever.
        DWORD timeout = IDLE_TIMEOUT_MS;
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,
            (const char*)&timeout, sizeof(timeout));

        // One complete response per send() - Nagle only adds latency.
        BOOL noDelay = TRUE;
        setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY,
            (const char*)&noDelay, sizeof(noDelay));

        pool_.enqueue([clientSocket]() {

            char buffer[8192];
            int served = 0;
            bool keepAlive = true;

            // Serve requests on this ONE connection until the client
            // disconnects, asks to close, or goes idle. No new TCP
            // handshake, no new socket, no TIME_WAIT per request.
            while (keepAlive && served < MAX_REQUESTS_PER_CONN) {

                int bytesReceived = recv(clientSocket, buffer,
                    sizeof(buffer) - 1, 0);

                // 0 = client closed cleanly. <0 = error or idle timeout.
                if (bytesReceived <= 0) break;

                buffer[bytesReceived] = '\0';
                HttpRequest request = HttpParser::parse(std::string(buffer));

                keepAlive = wantsKeepAlive(request);

                HttpResponse response = HttpHandler::handle(request);
                response.setHeader("Connection", keepAlive ? "keep-alive" : "close");

                std::string raw = response.toString();

                // send() may write fewer bytes than asked. Loop until done.
                size_t sent = 0;
                while (sent < raw.size()) {
                    int n = send(clientSocket, raw.data() + sent,
                        static_cast<int>(raw.size() - sent), 0);
                    if (n <= 0) { keepAlive = false; break; }
                    sent += n;
                }

                ++served;
            }

            // Graceful close: signal we're done writing, drain whatever the
            // client already sent, then close. closesocket() with unread data
            // pending makes Windows send an RST instead of a FIN.
            shutdown(clientSocket, SD_SEND);

            DWORD drainTimeout = 200;
            setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,
                (const char*)&drainTimeout, sizeof(drainTimeout));

            char drain[512];
            while (recv(clientSocket, drain, sizeof(drain), 0) > 0) { }

            closesocket(clientSocket);
        });
    }
}