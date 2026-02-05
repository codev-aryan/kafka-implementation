#include "Connection.hpp"
#include "../utils/ByteBuf.hpp"
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdint>
#include <vector>
#include <iostream>

Connection::Connection(int fd) : client_fd(fd) {}

Connection::~Connection() {
    close(client_fd);
}

void Connection::handle() {
    // 1. Read data from the socket
    // We use a reasonably large buffer for this stage. 
    // In production, we would handle partial reads/loops.
    std::vector<uint8_t> buffer(1024);
    ssize_t bytes_read = read(client_fd, buffer.data(), buffer.size());

    if (bytes_read <= 0) {
        return; 
    }

    // Resize vector to actual data read to avoid reading garbage
    buffer.resize(bytes_read);
    
    // 2. Wrap in ByteBuf for parsing
    ByteBuf buf(buffer);

    try {
        // 3. Parse Header v2 (Partial)
        // Structure: 
        // Message Size (Int32)
        // API Key (Int16)
        // API Version (Int16)
        // Correlation ID (Int32)
        
        int32_t message_size = buf.readInt32(); // Offset 0
        int16_t api_key = buf.readInt16();      // Offset 4
        int16_t api_version = buf.readInt16();  // Offset 6
        int32_t correlation_id = buf.readInt32(); // Offset 8

        // 4. Construct Response
        // message_size: 4 bytes (int32)
        // correlation_id: 4 bytes (int32)
        
        int32_t resp_message_size = 0; // Still 0 for this stage
        
        int32_t payload_size_be = htonl(resp_message_size);
        int32_t correlation_id_be = htonl(correlation_id);

        write(client_fd, &payload_size_be, sizeof(payload_size_be));
        write(client_fd, &correlation_id_be, sizeof(correlation_id_be));

    } catch (const std::exception& e) {
        std::cerr << "Error processing request: " << e.what() << "\n";
    }
}