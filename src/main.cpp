#include "Logger.h"
#include "TcpServer.h"

int main() {
    Logger::instance().setLevel(LogLevel::Info); 
    Logger::instance().info("TitanServer starting...");

    try {
        // Thread count = hardware concurrency
        size_t threads = std::thread::hardware_concurrency();
        if (threads == 0) threads = 4;

        TcpServer server(8080, threads);
        server.start();
    }
    catch (const std::exception& ex) {
        Logger::instance().error(ex.what());
    }

    return 0;
}
