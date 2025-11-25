#pragma once
#include "Packet.hpp"
#include <unordered_map>
#include <iostream>
#include <string>
#include "../http3/Frame.hpp"
#include "../http3/Qpack.hpp"

namespace quic {

struct QuicStream {
    uint64_t id;
    std::vector<uint8_t> data;
};

class QuicSession {
public:
    ConnectionId cid;
    
    void on_packet(const Packet& p) {
        std::cout << "[QUIC] Received Packet Number: " << p.packet_number << std::endl;
        
        if (p.flags & LONG_HEADER) {
            
            std::cout << "[QUIC] Handshake Packet" << std::endl;
        } else {
            
            std::cout << "[QUIC] 1-RTT Data Packet" << std::endl;
            
            
            try {
                size_t offset = 0;
                const uint8_t* data = p.payload.data();
                size_t len = p.payload.size();
                
                while (offset < len) {
                    http3::FrameHeader h3 = http3::FrameHeader::parse(data, len, offset);
                    std::cout << "[HTTP3] Frame Type: " << (int)h3.type << " Len: " << h3.length << std::endl;
                    
                    if (offset + h3.length > len) break;

                    if (h3.type == http3::FrameType::HEADERS) {
                         auto headers = http3::Qpack::decode(data + offset, h3.length);
                         for(const auto& h : headers) {
                             std::cout << "  " << h.name << ": " << h.value << std::endl;
                         }
                    }
                    
                    offset += h3.length;
                }
            } catch (const std::exception& e) {
                std::cout << "[HTTP3] Parse Error: " << e.what() << std::endl;
            }
        }
    }
};

class Engine {
public:
    void on_packet(const uint8_t* data, size_t len, const sockaddr* sender) {
        Packet p = Packet::parse(data, len);
        
        
        std::cout << "[QUIC] Packet from " << sender << " Flags: " << (int)p.flags << std::endl;
        
       
        QuicSession session;
        session.on_packet(p);
    }
};

}
