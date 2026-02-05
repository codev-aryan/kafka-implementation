#include "Connection.hpp"
#include "../utils/ByteBuf.hpp"
#include "../protocol/ApiKeys.hpp"
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
    std::vector<uint8_t> buffer(1024);
    ssize_t bytes_read = read(client_fd, buffer.data(), buffer.size());

    if (bytes_read <= 0) {
        return; 
    }

    buffer.resize(bytes_read);
    ByteBuf buf(buffer);

    try {
        // 2. Parse Request Header
        // Ensure we have enough data for the mandatory header fields
        // Size (4) + ApiKey (2) + ApiVersion (2) + CorrelationId (4) = 12 bytes
        if (buffer.size() < 12) {
            std::cerr << "Request too short\n";
            return;
        }

        int32_t message_size = buf.readInt32(); 
        int16_t api_key = buf.readInt16();      
        int16_t api_version = buf.readInt16();  
        int32_t correlation_id = buf.readInt32(); 

        // 3. Process Request
        int16_t error_code = 0;

        // Check for ApiVersions request (Key 18)
        if (api_key == static_cast<int16_t>(ApiKeys::API_VERSIONS)) {
            // Check if version is supported (0-4)
            if (api_version < 0 || api_version > 4) {
                error_code = 35; // UNSUPPORTED_VERSION
            }
        }

        // 4. Construct Response
        // Structure:
        // - Message Size (4 bytes)
        // - Correlation ID (4 bytes) - Response Header v0
        // - Error Code (2 bytes) - Start of ApiVersions Response Body
        
        int32_t resp_message_size = 0; // Placeholder
        
        int32_t payload_size_be = htonl(resp_message_size);
        int32_t correlation_id_be = htonl(correlation_id);
        int16_t error_code_be = htons(error_code);

        write(client_fd, &payload_size_be, sizeof(payload_size_be));
        write(client_fd, &correlation_id_be, sizeof(correlation_id_be));
        write(client_fd, &error_code_be, sizeof(error_code_be));

    } catch (const std::exception& e) {
        std::cerr << "Error processing request: " << e.what() << "\n";
    }
}