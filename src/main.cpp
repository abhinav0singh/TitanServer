#include "Logger.h"
#include "TcpServer.h"

int main() {
    Logger::instance().info("TitanServer starting...");

    try {
        TcpServer server(8080);
        server.start();
    }
    catch (const std::exception& ex) {
        Logger::instance().error(ex.what());
    }

    return 0;
}
