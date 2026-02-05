#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <arpa/inet.h>

class ByteBuf {
public:
    // Constructor for Reading
    explicit ByteBuf(const std::vector<uint8_t>& data) : data_(data), reader_index_(0) {}
    
    // Constructor for Writing
    ByteBuf() : reader_index_(0) {}

    // --- Read Methods ---

    int16_t readInt16() {
        if (reader_index_ + 2 > data_.size()) {
            throw std::out_of_range("Not enough bytes to read Int16");
        }
        uint16_t val;
        std::memcpy(&val, &data_[reader_index_], 2);
        reader_index_ += 2;
        return static_cast<int16_t>(ntohs(val));
    }

    int32_t readInt32() {
        if (reader_index_ + 4 > data_.size()) {
            throw std::out_of_range("Not enough bytes to read Int32");
        }
        uint32_t val;
        std::memcpy(&val, &data_[reader_index_], 4);
        reader_index_ += 4;
        return static_cast<int32_t>(ntohl(val));
    }

    // --- Write Methods ---

    void writeInt8(int8_t val) {
        data_.push_back(static_cast<uint8_t>(val));
    }

    void writeInt16(int16_t val) {
        uint16_t be_val = htons(static_cast<uint16_t>(val));
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&be_val);
        data_.insert(data_.end(), ptr, ptr + 2);
    }

    void writeInt32(int32_t val) {
        uint32_t be_val = htonl(static_cast<uint32_t>(val));
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&be_val);
        data_.insert(data_.end(), ptr, ptr + 4);
    }
    
    // For this stage, we only need to write simple Unsigned Varints (0-127)
    // which map 1:1 to a single byte.
    void writeUnsignedVarint(uint32_t val) {
         // Full LEB128 implementation not needed for small constants (0, 2)
         // but good to be safe for small values.
         while ((val & 0xffffff80) != 0L) {
             data_.push_back(static_cast<uint8_t>((val & 0x7f) | 0x80));
             val >>= 7;
         }
         data_.push_back(static_cast<uint8_t>(val & 0x7f));
    }

    const std::vector<uint8_t>& data() const {
        return data_;
    }

private:
    std::vector<uint8_t> data_;
    size_t reader_index_;
};