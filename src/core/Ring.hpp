#pragma once
#include "../sys/Platform.hpp"

namespace core {

class Ring {
public:
    explicit Ring(unsigned entries = 4096);
    ~Ring();

    Ring(const Ring&) = delete;
    Ring& operator=(const Ring&) = delete;

    void init();

    
    void attach(sys::native_handle_t fd);

    
    void submit_read(sys::native_handle_t fd, void* buf, size_t len, sys::NativeOverlapped* ov);

    // Submit a write request
    void submit_write(sys::native_handle_t fd, const void* buf, size_t len, sys::NativeOverlapped* ov);

    // Submit an accept request
    void submit_accept(sys::native_handle_t server_fd, void* output_buffer, int* client_len, sys::NativeOverlapped* ov);

    
    void submit_sendfile(sys::os_fd_t file_fd, sys::native_handle_t socket_fd, size_t offset, size_t count, sys::NativeOverlapped* ov);

    
    void submit_recvfrom(sys::native_handle_t fd, void* buffer, size_t len, struct sockaddr* addr, int* addr_len, sys::NativeOverlapped* ov);

    
    int process_completions(bool wait_for_completion = true);

private:
#ifdef PLATFORM_LINUX
    struct io_uring m_ring;
#elif defined(PLATFORM_WINDOWS)
    HANDLE m_iocp;
#endif
};

}
