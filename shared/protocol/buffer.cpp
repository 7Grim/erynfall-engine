#include "protocol/buffer.h"

namespace erynfall {

// --- Writing ---

void ByteBuffer::writeU8(uint8_t val) {
    data_.push_back(val);
}

void ByteBuffer::writeU16(uint16_t val) {
    data_.push_back(static_cast<uint8_t>(val & 0xFF));
    data_.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

void ByteBuffer::writeU32(uint32_t val) {
    data_.push_back(static_cast<uint8_t>(val & 0xFF));
    data_.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    data_.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    data_.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

void ByteBuffer::writeI8(int8_t val) {
    data_.push_back(static_cast<uint8_t>(val));
}

void ByteBuffer::writeI16(int16_t val) {
    writeU16(static_cast<uint16_t>(val));
}

void ByteBuffer::writeI32(int32_t val) {
    writeU32(static_cast<uint32_t>(val));
}

void ByteBuffer::writeString(const char* str) {
    size_t len = std::strlen(str);
    writeU16(static_cast<uint16_t>(len));
    for (size_t i = 0; i < len; ++i) {
        data_.push_back(static_cast<uint8_t>(str[i]));
    }
}

void ByteBuffer::writeBytes(const uint8_t* data, size_t len) {
    data_.insert(data_.end(), data, data + len);
}

// --- Reading ---

void ByteBuffer::checkRead(size_t bytes) const {
    if (readableBytes() < bytes) {
        throw std::runtime_error("ByteBuffer underflow");
    }
}

uint8_t ByteBuffer::readU8() {
    checkRead(1);
    return data_[read_pos_++];
}

uint16_t ByteBuffer::readU16() {
    checkRead(2);
    uint16_t val = static_cast<uint16_t>(data_[read_pos_]) |
                   (static_cast<uint16_t>(data_[read_pos_ + 1]) << 8);
    read_pos_ += 2;
    return val;
}

uint32_t ByteBuffer::readU32() {
    checkRead(4);
    uint32_t val = static_cast<uint32_t>(data_[read_pos_]) |
                   (static_cast<uint32_t>(data_[read_pos_ + 1]) << 8) |
                   (static_cast<uint32_t>(data_[read_pos_ + 2]) << 16) |
                   (static_cast<uint32_t>(data_[read_pos_ + 3]) << 24);
    read_pos_ += 4;
    return val;
}

int8_t ByteBuffer::readI8() {
    return static_cast<int8_t>(readU8());
}

int16_t ByteBuffer::readI16() {
    return static_cast<int16_t>(readU16());
}

int32_t ByteBuffer::readI32() {
    return static_cast<int32_t>(readU32());
}

std::string ByteBuffer::readString() {
    uint16_t len = readU16();
    checkRead(len);
    std::string str(data_.begin() + read_pos_, data_.begin() + read_pos_ + len);
    read_pos_ += len;
    return str;
}

void ByteBuffer::readBytes(uint8_t* buf, size_t len) {
    checkRead(len);
    std::memcpy(buf, data_.data() + read_pos_, len);
    read_pos_ += len;
}

// --- State ---

void ByteBuffer::append(const uint8_t* data, size_t len) {
    data_.insert(data_.end(), data, data + len);
}

} // namespace erynfall
