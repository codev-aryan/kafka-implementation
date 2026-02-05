#include "TcpServer.hpp"
#include "Connection.hpp"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>

TcpServer::TcpServer(int port) : port(port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("Failed to create server socket");
    }

    // Set SO_REUSEADDR to avoid 'Address already in use' errors during restarts
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(server_fd);
        throw std::runtime_error("setsockopt failed");
    }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
        close(server_fd);
        throw std::runtime_error("Failed to bind to port");
    }

    if (listen(server_fd, 5) != 0) {
        close(server_fd);
        throw std::runtime_error("listen failed");
    }
}

TcpServer::~TcpServer() {
    close(server_fd);
}

void TcpServer::start() {
    std::cout << "Listening on port " << port << "...\n";

    while (true) {
        struct sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);

        if (client_fd < 0) {
            std::cerr << "Accept failed\n";
            continue;
        }

        std::cout << "Client connected\n";
        
        // In a real server, we might spawn a thread here. 
        // For now, we handle it synchronously.
        Connection connection(client_fd);
        connection.handle();
    }
}