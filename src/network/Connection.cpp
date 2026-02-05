#include "Connection.hpp"
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdint>
#include <vector>

Connection::Connection(int fd) : client_fd(fd) {}

Connection::~Connection() {
    close(client_fd);
}

void Connection::handle() {
    // Basic buffer to drain the read queue (ignoring request content for now)
    char buffer[1024];
    read(client_fd, buffer, sizeof(buffer));

    // Construct the response
    // message_size: 4 bytes (int32)
    // correlation_id: 4 bytes (int32)
    
    // Using htonl to ensure Big-Endian byte order
    int32_t message_size = 0; // Per stage instructions, can be 0
    int32_t correlation_id = 7; // Hardcoded per stage instructions
    
    int32_t payload_size_be = htonl(message_size);
    int32_t correlation_id_be = htonl(correlation_id);

    write(client_fd, &payload_size_be, sizeof(payload_size_be));
    write(client_fd, &correlation_id_be, sizeof(correlation_id_be));
}