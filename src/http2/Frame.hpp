#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <stdexcept>

namespace http2 {

enum class FrameType : uint8_t {
    DATA = 0x0,
    HEADERS = 0x1,
    PRIORITY = 0x2,
    RST_STREAM = 0x3,
    SETTINGS = 0x4,
    PUSH_PROMISE = 0x5,
    PING = 0x6,
    GOAWAY = 0x7,
    WINDOW_UPDATE = 0x8,
    CONTINUATION = 0x9
};

enum class SettingsId : uint16_t {
    HEADER_TABLE_SIZE = 0x1,
    ENABLE_PUSH = 0x2,
    MAX_CONCURRENT_STREAMS = 0x3,
    INITIAL_WINDOW_SIZE = 0x4,
    MAX_FRAME_SIZE = 0x5,
    MAX_HEADER_LIST_SIZE = 0x6
};

enum Flags : uint8_t {
    ACK = 0x1,
    END_STREAM = 0x1,
    END_HEADERS = 0x4,
    PADDED = 0x8,
    PRIORITY = 0x20
};

#pragma pack(push, 1)
struct FrameHeader {
    uint8_t length_h;
    uint16_t length_l;
    FrameType type;
    uint8_t flags;
    uint32_t stream_id; 

    static constexpr size_t SIZE = 9;

    uint32_t get_length() const {
        return (length_h << 16) | length_l;
    }

    void set_length(uint32_t len) {
        length_h = (len >> 16) & 0xFF;
        length_l = (len & 0xFFFF); 
    }

    uint32_t get_stream_id() const {
        return stream_id & 0x7FFFFFFF; 
    }

    void set_stream_id(uint32_t id) {
        stream_id = id & 0x7FFFFFFF;
    }

    static FrameHeader parse(const uint8_t* data) {
        FrameHeader h;
        
        h.length_h = data[0];
        h.length_l = (data[1] << 8) | data[2]; 
        h.type = static_cast<FrameType>(data[3]);
        h.flags = data[4];
        
        uint32_t sid = ((data[5] & 0x7F) << 24) | (data[6] << 16) | (data[7] << 8) | data[8];
      
        
        h.stream_id = (data[5] << 24) | (data[6] << 16) | (data[7] << 8) | data[8];
        return h;
    }
    
    void encode(uint8_t* buffer) const {
        buffer[0] = length_h;
        buffer[1] = (length_l >> 8) & 0xFF;
        buffer[2] = length_l & 0xFF;
        buffer[3] = static_cast<uint8_t>(type);
        buffer[4] = flags;
        
        
        
        buffer[5] = (stream_id >> 24) & 0xFF;
        buffer[6] = (stream_id >> 16) & 0xFF;
        buffer[7] = (stream_id >> 8) & 0xFF;
        buffer[8] = stream_id & 0xFF;
    }
};
#pragma pack(pop)

}
