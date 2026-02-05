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
        // 1. Read Message Size
        uint32_t message_size_be = 0;
        ssize_t header_bytes = read(client_fd, &message_size_be, sizeof(message_size_be));

        if (header_bytes == 0) break; // Graceful close
        if (header_bytes < 0) {
            std::cerr << "Read error on socket\n";
            break;
        }

        int32_t message_size = ntohl(message_size_be);
        
        // 2. Read Request Body
        std::vector<uint8_t> buffer(4 + message_size);
        std::memcpy(buffer.data(), &message_size_be, 4);

        size_t bytes_to_read = message_size;
        size_t offset = 4;
        
        while (bytes_to_read > 0) {
            ssize_t res = read(client_fd, buffer.data() + offset, bytes_to_read);
            if (res <= 0) return;
            bytes_to_read -= res;
            offset += res;
        }

        // 3. Process Request
        ByteBuf req(buffer);
        try {
            if (buffer.size() < 12) {
                continue;
            }

            int32_t req_msg_size = req.readInt32(); 
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
            
            // Array Length: We now have 2 items.
            // Compact Array Length = N + 1 = 3
            resp_body.writeUnsignedVarint(3); 

            // Entry 1: ApiVersions (Key 18)
            resp_body.writeInt16(18); 
            resp_body.writeInt16(0);  
            resp_body.writeInt16(4);  
            resp_body.writeUnsignedVarint(0); // Tagged Fields

            // Entry 2: DescribeTopicPartitions (Key 75)
            resp_body.writeInt16(75); 
            resp_body.writeInt16(0);  
            resp_body.writeInt16(0);  
            resp_body.writeUnsignedVarint(0); // Tagged Fields

            // Throttle Time
            resp_body.writeInt32(0);
            // Global Tagged Fields
            resp_body.writeUnsignedVarint(0); 

            // 5. Send Response
            ByteBuf final_resp;
            int32_t body_size = resp_body.data().size();
            int32_t header_size = 4; // Correlation ID only for v0 header
            
            final_resp.writeInt32(header_size + body_size); 
            final_resp.writeInt32(correlation_id);          
            
            write(client_fd, final_resp.data().data(), final_resp.data().size());
            write(client_fd, resp_body.data().data(), resp_body.data().size());

        } catch (const std::exception& e) {
            std::cerr << "Error processing request: " << e.what() << "\n";
            break;
        }
    }
}