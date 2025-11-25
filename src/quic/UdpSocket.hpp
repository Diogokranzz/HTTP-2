#pragma once
#include "../sys/Platform.hpp"
#include "../core/Ring.hpp"
#include "../coro/IOAwaitable.hpp"
#include <iostream>

namespace quic {

class UdpSocket {
public:
    sys::native_handle_t fd = sys::INVALID_HANDLE_VALUE_NET;
    
    void init(int port) {
        #ifdef PLATFORM_WINDOWS
        fd = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (fd == INVALID_SOCKET) {
            throw std::runtime_error("Failed to create UDP socket");
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            throw std::runtime_error("UDP Bind failed");
        }
        std::cout << "[QUIC] UDP Listener active on port " << port << "\n";
        #endif
    }

  
};

}


