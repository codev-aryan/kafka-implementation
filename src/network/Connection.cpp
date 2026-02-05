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

    if (bytes_read <= 0) return;

    buffer.resize(bytes_read);
    ByteBuf req(buffer);

    try {
        // 2. Parse Request Header (v2)
        if (buffer.size() < 12) {
            std::cerr << "Request too short\n";
            return;
        }

        int32_t message_size = req.readInt32(); 
        int16_t api_key = req.readInt16();      
        int16_t api_version = req.readInt16();  
        int32_t correlation_id = req.readInt32(); 

        // 3. Process Request Logic
        int16_t error_code = 0;
        bool is_api_versions = (api_key == static_cast<int16_t>(ApiKeys::API_VERSIONS));
        
        if (is_api_versions) {
            if (api_version < 0 || api_version > 4) {
                error_code = 35; // UNSUPPORTED_VERSION
            }
        }

        // 4. Serialize Response Body
        ByteBuf resp_body;

        // ApiVersions Response Body v4
        // Field: error_code (int16)
        resp_body.writeInt16(error_code);

        // Field: api_keys (Compact Array)
        // Length = N + 1. We have 1 item, so length is 2.
        resp_body.writeUnsignedVarint(2); 

        // Item 1: ApiKey=18, Min=0, Max=4, TaggedFields=0
        resp_body.writeInt16(18); // ApiKey: ApiVersions
        resp_body.writeInt16(0);  // Min Version
        resp_body.writeInt16(4);  // Max Version
        resp_body.writeUnsignedVarint(0); // Tagged Fields (Empty)

        // Field: throttle_time_ms (int32)
        resp_body.writeInt32(0);

        // Field: TAG_BUFFER (Tagged Fields)
        resp_body.writeUnsignedVarint(0); // Empty

        // 5. Finalize Response Packet
        ByteBuf final_resp;
        
        // Total Size = Header (4 bytes) + Body Size
        int32_t body_size = resp_body.data().size();
        int32_t header_size = 4; // Correlation ID
        
        final_resp.writeInt32(header_size + body_size); // Message Size
        final_resp.writeInt32(correlation_id);          // Header
        
        // Append Body
        const auto& body_bytes = resp_body.data();
        write(client_fd, final_resp.data().data(), final_resp.data().size());
        write(client_fd, body_bytes.data(), body_bytes.size());

    } catch (const std::exception& e) {
        std::cerr << "Error processing request: " << e.what() << "\n";
    }
}