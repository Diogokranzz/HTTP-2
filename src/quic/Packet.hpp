#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include <iostream>

namespace quic {


enum HeaderForm : uint8_t {
    SHORT_HEADER = 0x00,
    LONG_HEADER = 0x80
};


enum PacketType : uint8_t {
    INITIAL = 0x00,
    ZERO_RTT = 0x10,
    HANDSHAKE = 0x20,
    RETRY = 0x30
};

struct ConnectionId {
    static constexpr size_t MAX_LEN = 20;
    uint8_t len = 0;
    std::array<uint8_t, MAX_LEN> data;

    bool operator==(const ConnectionId& other) const {
        if (len != other.len) return false;
        for (size_t i = 0; i < len; ++i) if (data[i] != other.data[i]) return false;
        return true;
    }
};

struct Packet {
    uint8_t flags;
    ConnectionId dest_cid;
    ConnectionId src_cid;
    uint32_t version;
    uint64_t packet_number;
    std::vector<uint8_t> payload;

    static Packet parse(const uint8_t* data, size_t len) {
        Packet p;
        if (len < 1) return p;
        
        size_t offset = 0;
        p.flags = data[offset++];

        if (p.flags & LONG_HEADER) {
            
            if (len < offset + 4) return p;
            
            p.version = (data[offset] << 24) | (data[offset+1] << 16) | (data[offset+2] << 8) | data[offset+3];
            offset += 4;

            
            uint8_t dcid_len = data[offset++];
            p.dest_cid.len = dcid_len;
            for(int i=0; i<dcid_len; ++i) p.dest_cid.data[i] = data[offset++];

            
            uint8_t scid_len = data[offset++];
            p.src_cid.len = scid_len;
            for(int i=0; i<scid_len; ++i) p.src_cid.data[i] = data[offset++];
            
          
            while(offset < len) p.payload.push_back(data[offset++]);
            
        } else {
           
            p.dest_cid.len = 8;
            for(int i=0; i<8; ++i) p.dest_cid.data[i] = data[offset++];
            
            while(offset < len) p.payload.push_back(data[offset++]);
        }
        return p;
    }
};

}
