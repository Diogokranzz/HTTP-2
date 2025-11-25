#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include "../http2/Hpack.hpp" 

namespace http3 {

using Header = http2::Header;

class Qpack {
public:
    
    static const std::vector<Header>& static_table() {
        static std::vector<Header> table = {
            {":authority", ""},
            {":path", "/"},
            {"age", "0"},
            {"content-disposition", ""},
            {"content-length", "0"},
            {"cookie", ""},
            {"date", ""},
            {"etag", ""},
            {"if-modified-since", ""},
            {"if-none-match", ""},
            {"last-modified", ""},
            {"link", ""},
            {"location", ""},
            {"referer", ""},
            {"set-cookie", ""},
            {":method", "CONNECT"},
            {":method", "DELETE"},
            {":method", "GET"},
            {":method", "HEAD"},
            {":method", "OPTIONS"},
            {":method", "POST"},
            {":method", "PUT"},
            {":scheme", "http"},
            {":scheme", "https"},
            {":status", "103"},
            {":status", "200"},
            {":status", "304"},
            {":status", "404"},
            {":status", "503"},
            {"accept", "*/*"},
            {"accept-encoding", "gzip, deflate, br"},
            {"accept-language", ""},
            {"cache-control", ""},
            {"content-type", "application/json"},
            {"user-agent", ""}
        };
        return table;
    }


    static std::vector<Header> decode(const uint8_t* data, size_t len) {
        std::vector<Header> headers;
        size_t offset = 0;

        
        
        if (len > 2) offset += 2;

        while (offset < len) {
            uint8_t b = data[offset];

            if (b & 0x80) {
              
                bool is_static = (b & 0x40);
                size_t index = decode_integer(data, len, offset, 6);
                
                if (is_static) {
                    if (index < static_table().size()) {
                        headers.push_back(static_table()[index]);
                    }
                } else {
                    
                }
            } 
            else if (b & 0x40) {
               
                offset++; 
                
            }
            else if (b & 0x20) {
                 
                 offset++;
            }
            else {
               
                offset++;
            }
        }
        return headers;
    }

    
    static size_t decode_integer(const uint8_t* data, size_t len, size_t& offset, uint8_t prefix_bits) {
        if (offset >= len) return 0;
        
        uint8_t mask = (1 << prefix_bits) - 1;
        size_t value = data[offset] & mask;
        offset++;

        if (value < mask) {
            return value;
        }

        size_t shift = 0;
        while (offset < len) {
            uint8_t b = data[offset];
            offset++;
            value += (b & 0x7F) << shift;
            if ((b & 0x80) == 0) {
                return value;
            }
            shift += 7;
        }
        return value;
    }
};

}
