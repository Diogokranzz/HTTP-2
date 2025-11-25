#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace http3 {

enum class FrameType : uint64_t {
    DATA = 0x00,
    HEADERS = 0x01,
    CANCEL_PUSH = 0x03,
    SETTINGS = 0x04,
    PUSH_PROMISE = 0x05,
    GOAWAY = 0x07,
    MAX_PUSH_ID = 0x0D
};

struct FrameHeader {
    FrameType type;
    uint64_t length;

    
    static uint64_t read_varint(const uint8_t* data, size_t len, size_t& offset) {
        if (offset >= len) throw std::out_of_range("Buffer underflow");
        
        uint8_t first = data[offset];
        uint8_t prefix = first >> 6;
        uint64_t value = 0;

        if (prefix == 0x00) {
            value = first & 0x3F;
            offset += 1;
        } else if (prefix == 0x01) {
            if (offset + 2 > len) throw std::out_of_range("Buffer underflow");
            value = ((first & 0x3F) << 8) | data[offset+1];
            offset += 2;
        } else if (prefix == 0x02) {
            if (offset + 4 > len) throw std::out_of_range("Buffer underflow");
            value = ((first & 0x3F) << 24) | (data[offset+1] << 16) | (data[offset+2] << 8) | data[offset+3];
            offset += 4;
        } else {
            if (offset + 8 > len) throw std::out_of_range("Buffer underflow");
            value = ((uint64_t)(first & 0x3F) << 56);
            for(int i=1; i<8; ++i) value |= ((uint64_t)data[offset+i] << (56 - i*8));
            offset += 8;
        }
        return value;
    }

    static FrameHeader parse(const uint8_t* data, size_t len, size_t& offset) {
        FrameHeader h;
        h.type = static_cast<FrameType>(read_varint(data, len, offset));
        h.length = read_varint(data, len, offset);
        return h;
    }
};

}
