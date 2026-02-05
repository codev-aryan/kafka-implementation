#include <iostream>
#include "network/TcpServer.hpp"

int main(int argc, char* argv[]) {
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    try {
        // Port 9092 is the standard Kafka port
        TcpServer server(9092);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Server failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}