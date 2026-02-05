#include "Connection.hpp"
#include "../utils/ByteBuf.hpp"
#include "../protocol/ApiKeys.hpp"
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdint>
#include <vector>
#include <iostream>
#include <cstring>

Connection::Connection(int fd) : client_fd(fd) {}

Connection::~Connection() {
    close(client_fd);
}

void Connection::handle() {
    while (true) {
        // 1. Read Message Size (4 bytes) to handle framing
        uint32_t message_size_be = 0;
        ssize_t header_bytes = read(client_fd, &message_size_be, sizeof(message_size_be));

        if (header_bytes == 0) {
            // Client closed connection gracefully
            break;
        }
        if (header_bytes < 0) {
            std::cerr << "Read error on socket\n";
            break;
        }

        // Convert big-endian size to host order
        int32_t message_size = ntohl(message_size_be);
        
        // 2. Read the specific amount of data expected for this request
        // We create a buffer large enough for Size (4) + Body (message_size)
        // This preserves the format expected by ByteBuf (which reads the size first)
        std::vector<uint8_t> buffer(4 + message_size);
        std::memcpy(buffer.data(), &message_size_be, 4);

        size_t bytes_to_read = message_size;
        size_t offset = 4;
        
        while (bytes_to_read > 0) {
            ssize_t res = read(client_fd, buffer.data() + offset, bytes_to_read);
            if (res <= 0) {
                std::cerr << "Incomplete message or read error\n";
                return;
            }
            bytes_to_read -= res;
            offset += res;
        }

        // 3. Process Request using existing logic
        ByteBuf req(buffer);

        try {
            if (buffer.size() < 12) {
                std::cerr << "Request too short\n";
                continue;
            }

            int32_t req_msg_size = req.readInt32(); // Redundant but consistent with ByteBuf
            int16_t api_key = req.readInt16();      
            int16_t api_version = req.readInt16();  
            int32_t correlation_id = req.readInt32(); 

            int16_t error_code = 0;
            bool is_api_versions = (api_key == static_cast<int16_t>(ApiKeys::API_VERSIONS));
            
            if (is_api_versions) {
                if (api_version < 0 || api_version > 4) {
                    error_code = 35; // UNSUPPORTED_VERSION
                }
            }

            // 4. Serialize Response
            ByteBuf resp_body;

            // ApiVersions Response Body v4
            resp_body.writeInt16(error_code);
            resp_body.writeUnsignedVarint(2); // Array Length (N+1)
            resp_body.writeInt16(18);         // ApiKey
            resp_body.writeInt16(0);          // Min Version
            resp_body.writeInt16(4);          // Max Version
            resp_body.writeUnsignedVarint(0); // Tagged Fields
            resp_body.writeInt32(0);          // Throttle time
            resp_body.writeUnsignedVarint(0); // Tagged Fields

            // 5. Send Response
            ByteBuf final_resp;
            int32_t body_size = resp_body.data().size();
            int32_t header_size = 4; // Correlation ID
            
            final_resp.writeInt32(header_size + body_size); 
            final_resp.writeInt32(correlation_id);          
            
            const auto& body_bytes = resp_body.data();
            
            // Write Header
            write(client_fd, final_resp.data().data(), final_resp.data().size());
            // Write Body
            write(client_fd, body_bytes.data(), body_bytes.size());

        } catch (const std::exception& e) {
            std::cerr << "Error processing request: " << e.what() << "\n";
            break;
        }
    }
}