#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>

namespace http2 {

struct Header {
    std::string name;
    std::string value;
};

class Hpack {
public:
    static const std::vector<Header>& static_table() {
        static std::vector<Header> table = {
            {":authority", ""},
            {":method", "GET"},
            {":method", "POST"},
            {":path", "/"},
            {":path", "/index.html"},
            {":scheme", "http"},
            {":scheme", "https"},
            {":status", "200"},
            {":status", "404"},
            {":status", "500"},
            {"accept-encoding", ""},
            {"content-length", ""},
            {"content-type", ""},
            {"date", ""},
            {"user-agent", ""}
        };
        return table;
    }

    static std::vector<Header> decode(const uint8_t* data, size_t len) {
        std::vector<Header> headers;
        size_t offset = 0;

        while (offset < len) {
            uint8_t b = data[offset];

            if (b & 0x80) {
               
                size_t index = decode_integer(data, len, offset, 7);
                if (index == 0) throw std::runtime_error("HPACK Index 0");
                
                if (index <= static_table().size()) {
                    headers.push_back(static_table()[index - 1]);
                } else {
                    
                }
            } 
            else if (b & 0x40) {
                
                size_t index = decode_integer(data, len, offset, 6);
                Header h;
                if (index == 0) {
                    h.name = decode_string(data, len, offset);
                } else if (index <= static_table().size()) {
                    h.name = static_table()[index - 1].name;
                }
                h.value = decode_string(data, len, offset);
                headers.push_back(h);
                
            }
            else if ((b & 0xF0) == 0x00) {
                
                size_t index = decode_integer(data, len, offset, 4);
                Header h;
                if (index == 0) {
                    h.name = decode_string(data, len, offset);
                } else if (index <= static_table().size()) {
                    h.name = static_table()[index - 1].name;
                }
                h.value = decode_string(data, len, offset);
                headers.push_back(h);
            }
            else {
                 
                 if ((b & 0xE0) == 0x20) {
                   
                     decode_integer(data, len, offset, 5);
                 } else {
                    
                     size_t index = decode_integer(data, len, offset, 4);
                     Header h;
                     if (index == 0) {
                         h.name = decode_string(data, len, offset);
                     } else if (index <= static_table().size()) {
                         h.name = static_table()[index - 1].name;
                     }
                     h.value = decode_string(data, len, offset);
                     headers.push_back(h);
                 }
            }
        }
        return headers;
    }

    static void encode(const std::vector<Header>& headers, std::vector<uint8_t>& out) {
        for (const auto& h : headers) {
           
            bool found = false;
            for (size_t i = 0; i < static_table().size(); ++i) {
                if (static_table()[i].name == h.name && static_table()[i].value == h.value) {
                   
                    encode_integer(i + 1, 7, 0x80, out);
                    found = true;
                    break;
                }
            }
            if (found) continue;

            
            size_t name_index = 0;
           

            if (name_index > 0) {
                
                encode_integer(name_index, 4, 0x00, out);
                encode_string(h.value, out);
            } else {
                
                out.push_back(0x00);
                encode_string(h.name, out);
                encode_string(h.value, out);
            }
        }
    }

    static std::string decode_string(const uint8_t* data, size_t len, size_t& offset) {
        if (offset >= len) throw std::out_of_range("Buffer overflow");
        
        uint8_t b = data[offset];
        bool huffman = (b & 0x80) != 0;
        size_t str_len = decode_integer(data, len, offset, 7);
        
        if (offset + str_len > len) throw std::out_of_range("String length overflow");
        
        std::string s;
        if (huffman) {
           
            s.assign((const char*)(data + offset), str_len);
        } else {
            s.assign((const char*)(data + offset), str_len);
        }
        offset += str_len;
        return s;
    }

    static void encode_string(const std::string& s, std::vector<uint8_t>& out) {
        
        encode_integer(s.size(), 7, 0x00, out);
        out.insert(out.end(), s.begin(), s.end());
    }

    static void encode_integer(size_t value, uint8_t prefix_bits, uint8_t flags, std::vector<uint8_t>& out) {
        uint8_t mask = (1 << prefix_bits) - 1;
        if (value < mask) {
            out.push_back(flags | value);
        } else {
            out.push_back(flags | mask);
            value -= mask;
            while (value >= 128) {
                out.push_back((value & 0x7F) | 0x80);
                value >>= 7;
            }
            out.push_back(value);
        }
    }

    static size_t decode_integer(const uint8_t* data, size_t len, size_t& offset, uint8_t prefix_bits) {
        if (offset >= len) throw std::out_of_range("Buffer overflow");
        
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
        throw std::out_of_range("Integer decoding failed");
    }
};

}
