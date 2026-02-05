#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <arpa/inet.h>

class ByteBuf {
public:
    explicit ByteBuf(const std::vector<uint8_t>& data) : data_(data), reader_index_(0) {}

    // Read a 16-bit signed integer (Big Endian)
    int16_t readInt16() {
        if (reader_index_ + 2 > data_.size()) {
            throw std::out_of_range("Not enough bytes to read Int16");
        }
        uint16_t val;
        std::memcpy(&val, &data_[reader_index_], 2);
        reader_index_ += 2;
        return static_cast<int16_t>(ntohs(val));
    }

    // Read a 32-bit signed integer (Big Endian)
    int32_t readInt32() {
        if (reader_index_ + 4 > data_.size()) {
            throw std::out_of_range("Not enough bytes to read Int32");
        }
        uint32_t val;
        std::memcpy(&val, &data_[reader_index_], 4);
        reader_index_ += 4;
        return static_cast<int32_t>(ntohl(val));
    }

private:
    std::vector<uint8_t> data_;
    size_t reader_index_;
};