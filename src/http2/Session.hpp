#pragma once
#include "Frame.hpp"
#include "Hpack.hpp"
#include <unordered_map>
#include <vector>
#include <iostream>
#include <string>

namespace http2 {

struct Stream {
    uint32_t id;
    enum State { IDLE, OPEN, HALF_CLOSED_REMOTE, HALF_CLOSED_LOCAL, CLOSED } state = IDLE;
    std::vector<uint8_t> recv_buffer; 
};

class Session {
public:
    Session() = default;

    
    bool on_data(const uint8_t* data, size_t len) {
        pending_buffer_.insert(pending_buffer_.end(), data, data + len);
        
        size_t offset = 0;
        
        if (!preface_received_) {
            if (pending_buffer_.size() < 24) return true; 
            
            preface_received_ = true;
            offset += 24;
            std::cout << "[HTTP2] Connection Preface Received" << std::endl;
        }

        while (offset + FrameHeader::SIZE <= pending_buffer_.size()) {
            FrameHeader header = FrameHeader::parse(pending_buffer_.data() + offset);
            uint32_t length = header.get_length();
            
            if (offset + FrameHeader::SIZE + length > pending_buffer_.size()) {
                break; 
            }

            const uint8_t* payload = pending_buffer_.data() + offset + FrameHeader::SIZE;
            process_frame(header, payload);
            
            offset += FrameHeader::SIZE + length;
        }
        
        if (offset > 0) {
            pending_buffer_.erase(pending_buffer_.begin(), pending_buffer_.begin() + offset);
        }
        
        return true;
    }

    void consume_output(std::vector<char>& out) {
        if (!output_buffer_.empty()) {
            out.insert(out.end(), output_buffer_.begin(), output_buffer_.end());
            output_buffer_.clear();
        }
    }

    void send_settings() {
        
        FrameHeader h;
        h.set_length(0);
        h.type = FrameType::SETTINGS;
        h.flags = 0;
        h.set_stream_id(0);
        write_frame(h, nullptr);
    }

private:
   
    void process_frame(const FrameHeader& header, const uint8_t* payload);
    void handle_data(const FrameHeader& header, const uint8_t* payload);
    void handle_headers(const FrameHeader& header, const uint8_t* payload);
    void handle_settings(const FrameHeader& header, const uint8_t* payload);
    void handle_ping(const FrameHeader& header, const uint8_t* payload);
    void handle_rst_stream(const FrameHeader& header, const uint8_t* payload);
    void handle_goaway(const FrameHeader& header, const uint8_t* payload);
    void send_simple_response(uint32_t stream_id);
    void write_frame(const FrameHeader& header, const uint8_t* payload);
    Stream& get_or_create_stream(uint32_t id);

    
    bool preface_received_ = false;
    std::unordered_map<uint32_t, Stream> streams_;
    std::vector<char> output_buffer_;
    std::vector<uint8_t> pending_buffer_;
};


inline void Session::process_frame(const FrameHeader& header, const uint8_t* payload) {
    uint32_t stream_id = header.get_stream_id();
    uint32_t length = header.get_length();
    
    std::cout << "[HTTP2] Recv Frame: Type=" << (int)header.type 
              << " Len=" << length 
              << " Flags=" << (int)header.flags 
              << " Stream=" << stream_id << "\n";

    if (stream_id > 0) {
        get_or_create_stream(stream_id);
    }

    switch (header.type) {
        case FrameType::DATA:
            handle_data(header, payload);
            break;
        case FrameType::HEADERS:
            handle_headers(header, payload);
            break;
        case FrameType::SETTINGS:
            handle_settings(header, payload);
            break;
        case FrameType::PING:
            handle_ping(header, payload);
            break;
        case FrameType::RST_STREAM:
            handle_rst_stream(header, payload);
            break;
        case FrameType::GOAWAY:
            handle_goaway(header, payload);
            break;
        default:
            std::cout << "[HTTP2] Unknown/Ignored Frame Type: " << (int)header.type << "\n";
            break;
    }
}

inline void Session::handle_settings(const FrameHeader& header, const uint8_t* payload) {
    if (header.flags & Flags::ACK) return;
    
    FrameHeader ack;
    ack.set_length(0);
    ack.type = FrameType::SETTINGS;
    ack.flags = Flags::ACK;
    ack.set_stream_id(0);
    write_frame(ack, nullptr);
}

inline void Session::handle_ping(const FrameHeader& header, const uint8_t* payload) {
    if (header.flags & Flags::ACK) return;
   
    FrameHeader pong = header;
    pong.flags = Flags::ACK;
    write_frame(pong, payload);
}

inline void Session::handle_headers(const FrameHeader& header, const uint8_t* payload) {
    uint32_t stream_id = header.get_stream_id();
    Stream& stream = get_or_create_stream(stream_id);
    stream.state = Stream::OPEN;
    
    send_simple_response(stream_id);
}

inline void Session::handle_data(const FrameHeader& header, const uint8_t* payload) {
    uint32_t stream_id = header.get_stream_id();
    uint32_t length = header.get_length();
    Stream& stream = get_or_create_stream(stream_id);
    if (length > 0) {
        stream.recv_buffer.insert(stream.recv_buffer.end(), payload, payload + length);
    }
    
    if (header.flags & Flags::END_STREAM) {
        stream.state = Stream::HALF_CLOSED_REMOTE;
    }
}

inline void Session::handle_rst_stream(const FrameHeader& header, const uint8_t* payload) {
    uint32_t stream_id = header.get_stream_id();
    uint32_t error_code = 0;
    if (header.get_length() >= 4) {
        error_code = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
    }
    
    std::cout << "[HTTP2] RST_STREAM Stream=" << stream_id << " ErrorCode=" << error_code << "\n";
    
    if (streams_.contains(stream_id)) {
        streams_[stream_id].state = Stream::CLOSED;
        streams_.erase(stream_id);
    }
}

inline void Session::send_simple_response(uint32_t stream_id) {
    std::string body = "Hello from HTTP/2 Stream " + std::to_string(stream_id);
    
    std::vector<Header> headers = {
        {":status", "200"},
        {"content-type", "text/plain"},
        {"content-length", std::to_string(body.size())},
        {"server", "DK-Server/1.0"}
    };
    
    std::vector<uint8_t> hpack_block;
    Hpack::encode(headers, hpack_block);
    
    FrameHeader h;
    h.set_length((uint32_t)hpack_block.size());
    h.type = FrameType::HEADERS;
    h.flags = Flags::END_HEADERS; 
    h.set_stream_id(stream_id);
    write_frame(h, hpack_block.data());
    
    FrameHeader d;
    d.set_length((uint32_t)body.size());
    d.type = FrameType::DATA;
    d.flags = Flags::END_STREAM;
    d.set_stream_id(stream_id);
    write_frame(d, (const uint8_t*)body.data());
}

inline void Session::handle_goaway(const FrameHeader& header, const uint8_t* payload) {
   
}

inline void Session::write_frame(const FrameHeader& header, const uint8_t* payload) {
    uint8_t buf[9];
    header.encode(buf); 
    
    for (size_t i = 0; i < 9; ++i) {
        output_buffer_.push_back(static_cast<char>(buf[i]));
    }
    
    uint32_t length = header.get_length();
    if (length > 0 && payload) {
        for (size_t i = 0; i < length; ++i) {
            output_buffer_.push_back(static_cast<char>(payload[i]));
        }
    }
}

inline Stream& Session::get_or_create_stream(uint32_t id) {
    if (!streams_.contains(id)) {
        streams_[id] = Stream{id};
    }
    return streams_[id];
}

}
