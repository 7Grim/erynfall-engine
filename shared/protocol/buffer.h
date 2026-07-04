#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>

namespace erynfall {

// Binary packet buffer for the protocol.
// Supports reading and writing primitive types in little-endian byte order.
class ByteBuffer {
public:
    ByteBuffer() = default;
    explicit ByteBuffer(size_t capacity) { data_.reserve(capacity); }

    // --- Writing ---
    void writeU8(uint8_t val);
    void writeU16(uint16_t val);
    void writeU32(uint32_t val);
    void writeI8(int8_t val);
    void writeI16(int16_t val);
    void writeI32(int32_t val);
    void writeString(const char* str);      // Null-terminated
    void writeBytes(const uint8_t* data, size_t len);

    // --- Reading ---
    uint8_t  readU8();
    uint16_t readU16();
    uint32_t readU32();
    int8_t   readI8();
    int16_t  readI16();
    int32_t  readI32();
    std::string readString();                 // Null-terminated
    void readBytes(uint8_t* buf, size_t len);

    // --- State ---
    size_t readableBytes() const { return data_.size() - read_pos_; }
    size_t size() const { return data_.size(); }
    const uint8_t* data() const { return data_.data(); }
    uint8_t* data() { return data_.data(); }
    void clear() { data_.clear(); read_pos_ = 0; }

    // Reset for reading (keep data, reset read position)
    void resetRead() { read_pos_ = 0; }

    // Copy raw data in (from network read)
    void append(const uint8_t* data, size_t len);

private:
    void checkRead(size_t bytes) const;

    std::vector<uint8_t> data_;
    size_t read_pos_ = 0;
};

} // namespace erynfall
