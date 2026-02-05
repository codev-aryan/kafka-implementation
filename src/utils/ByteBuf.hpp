#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <arpa/inet.h>
#include <string>

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
    
    int8_t readInt8() {
         if (reader_index_ + 1 > data_.size()) {
            throw std::out_of_range("Not enough bytes to read Int8");
        }
        return static_cast<int8_t>(data_[reader_index_++]);
    }

    uint32_t readUnsignedVarint() {
        uint32_t value = 0;
        int shift = 0;
        while (true) {
            if (reader_index_ >= data_.size()) {
                throw std::out_of_range("Not enough bytes to read Varint");
            }
            uint8_t b = data_[reader_index_++];
            value |= (static_cast<uint32_t>(b & 0x7F) << shift);
            if ((b & 0x80) == 0) break;
            shift += 7;
        }
        return value;
    }

    // Standard string (int16 length prefix)
    void skipString() {
        int16_t len = readInt16();
        if (len > 0) {
            if (reader_index_ + len > data_.size()) throw std::out_of_range("String buffer overflow");
            reader_index_ += len;
        }
    }

    std::string readCompactString() {
        uint32_t len = readUnsignedVarint();
        if (len == 0) return ""; 
        // Compact string length is N + 1
        uint32_t str_len = len - 1;
        if (reader_index_ + str_len > data_.size()) {
             throw std::out_of_range("Compact String buffer overflow");
        }
        std::string s(reinterpret_cast<const char*>(&data_[reader_index_]), str_len);
        reader_index_ += str_len;
        return s;
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
    
    void writeBool(bool val) {
        writeInt8(val ? 1 : 0);
    }
    
    void writeUnsignedVarint(uint32_t val) {
         while ((val & 0xffffff80) != 0L) {
             data_.push_back(static_cast<uint8_t>((val & 0x7f) | 0x80));
             val >>= 7;
         }
         data_.push_back(static_cast<uint8_t>(val & 0x7f));
    }

    void writeCompactString(const std::string& str) {
        writeUnsignedVarint(str.length() + 1);
        data_.insert(data_.end(), str.begin(), str.end());
    }

    void writeUUID(const uint8_t* uuid) {
        data_.insert(data_.end(), uuid, uuid + 16);
    }

    const std::vector<uint8_t>& data() const {
        return data_;
    }

private:
    std::vector<uint8_t> data_;
    size_t reader_index_;
};