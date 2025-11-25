#pragma once
#include "../sys/Platform.hpp"
#include "../core/BufferPool.hpp"

namespace net {

struct Connection {
    sys::native_handle_t fd;
    void* buffer;
    size_t buffer_len;
    sys::NativeOverlapped ov; 

    Connection() : fd(sys::INVALID_HANDLE_VALUE_NET), buffer(nullptr), buffer_len(0) {
        ov.user_data = nullptr;
    }

    void reset() {
        if (fd != sys::INVALID_HANDLE_VALUE_NET) {
#ifdef PLATFORM_WINDOWS
            closesocket(fd);
#else
            close(fd);
#endif
            fd = sys::INVALID_HANDLE_VALUE_NET;
        }
       
        buffer = nullptr;
        buffer_len = 0;
    }
};

}
