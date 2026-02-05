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

        if (header_bytes == 0) break; 
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

        ByteBuf req(buffer);
        try {
            if (buffer.size() < 12) continue;

            // --- Parse Header (v2) ---
            int32_t req_msg_size = req.readInt32(); 
            int16_t api_key = req.readInt16();      
            int16_t api_version = req.readInt16();  
            int32_t correlation_id = req.readInt32(); 
            
            // Skip Client ID (Nullable String)
            req.skipString(); 
            // Skip Tag Buffer (Unsigned Varint)
            req.readUnsignedVarint(); 

            // --- Logic Selection ---
            ByteBuf resp_body;
            bool use_header_v1 = false; // Default v0 (only correlation_id)

            if (api_key == static_cast<int16_t>(ApiKeys::API_VERSIONS)) {
                // ... Existing ApiVersions Logic ...
                int16_t error_code = 0;
                if (api_version < 0 || api_version > 4) {
                    error_code = 35;
                }
                
                resp_body.writeInt16(error_code);
                resp_body.writeUnsignedVarint(3); // Array Len 3 (2 items)
                
                resp_body.writeInt16(18); // ApiVersions
                resp_body.writeInt16(0);  
                resp_body.writeInt16(4);  
                resp_body.writeUnsignedVarint(0);

                resp_body.writeInt16(75); // DescribeTopicPartitions
                resp_body.writeInt16(0);  
                resp_body.writeInt16(0);  
                resp_body.writeUnsignedVarint(0);

                resp_body.writeInt32(0);          
                resp_body.writeUnsignedVarint(0); 

            } else if (api_key == static_cast<int16_t>(ApiKeys::DESCRIBE_TOPIC_PARTITIONS)) {
                use_header_v1 = true;
                
                // Parse Request Body
                // topics array (Compact Array)
                uint32_t topics_len = req.readUnsignedVarint();
                std::vector<std::string> requested_topics;
                
                // Compact array length includes N + 1
                if (topics_len > 0) {
                    size_t count = topics_len - 1;
                    for (size_t i = 0; i < count; ++i) {
                        requested_topics.push_back(req.readCompactString());
                        req.readUnsignedVarint(); // Skip topic tags
                    }
                }
                // (Ignore remaining fields: partition limit, cursor, etc.)

                // Construct Response Body
                resp_body.writeInt32(0); // throttle_time_ms
                
                // topics array
                resp_body.writeUnsignedVarint(requested_topics.size() + 1);
                
                uint8_t zero_uuid[16] = {0};

                for (const auto& topic : requested_topics) {
                    resp_body.writeInt16(3); // error_code: UNKNOWN_TOPIC_OR_PARTITION
                    resp_body.writeCompactString(topic);
                    resp_body.writeUUID(zero_uuid); // topic_id
                    resp_body.writeBool(false);     // is_internal
                    resp_body.writeUnsignedVarint(1); // partitions array (empty, len=0+1)
                    resp_body.writeInt32(0);        // topic_authorized_operations
                    resp_body.writeUnsignedVarint(0); // Tag Buffer
                }

                resp_body.writeInt8(-1); // next_cursor (0xff)
                resp_body.writeUnsignedVarint(0); // Tag Buffer
            }

            // 5. Send Response
            ByteBuf final_resp;
            int32_t body_size = resp_body.data().size();
            
            // Header Size:
            // v0: Correlation ID (4)
            // v1: Correlation ID (4) + Tag Buffer (Varint 0 -> 1 byte) = 5
            int32_t header_size = use_header_v1 ? 5 : 4;

            final_resp.writeInt32(header_size + body_size); 
            final_resp.writeInt32(correlation_id);          
            
            if (use_header_v1) {
                final_resp.writeUnsignedVarint(0); // Header Tag Buffer (Empty)
            }
            
            write(client_fd, final_resp.data().data(), final_resp.data().size());
            write(client_fd, resp_body.data().data(), resp_body.data().size());

        } catch (const std::exception& e) {
            std::cerr << "Error processing request: " << e.what() << "\n";
            break;
        }
    }
}