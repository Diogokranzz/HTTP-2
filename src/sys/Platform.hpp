#pragma once

#include <cstdint>
#include <cstddef>
#include <coroutine>
#include <optional>
#include <span>

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <mswsock.h>
#elif defined(__linux__)
    #define PLATFORM_LINUX
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <liburing.h>
#else
    #error "Unsupported Platform"
#endif

namespace sys {

#if defined(PLATFORM_WINDOWS)
    using native_handle_t = SOCKET;
    using os_fd_t = HANDLE;
    constexpr native_handle_t INVALID_HANDLE_VALUE_NET = INVALID_SOCKET;

    struct NativeOverlapped {
        WSAOVERLAPPED ol;
        void* user_data;
        SOCKET client_socket; 
        int result; 
    };

#elif defined(PLATFORM_LINUX)
    using native_handle_t = int;
    using os_fd_t = int;
    constexpr native_handle_t INVALID_HANDLE_VALUE_NET = -1;

    struct NativeOverlapped {
        void* user_data;
        int result;
    };
#endif

    struct IOCompletion {
        int result; 
        void* user_data;
    };

}
