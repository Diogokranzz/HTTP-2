#pragma once
#include "../core/Ring.hpp"
#include "../sys/Platform.hpp"
#include <coroutine>

namespace coro {

struct IOAwaitable {
    core::Ring& ring;
    sys::native_handle_t fd;
    void* buf;
    size_t len;
   
    int op_type; 
    sys::NativeOverlapped* ov;

  
    struct sockaddr* client_addr;
    int* client_len;

   
    sys::os_fd_t file_fd;
    size_t offset;

   
    int result;

    IOAwaitable(core::Ring& r, sys::native_handle_t f, void* b, size_t l, sys::NativeOverlapped* o)
        : ring(r), fd(f), buf(b), len(l), op_type(0), ov(o), server_fd(0), client_addr(nullptr), client_len(nullptr), file_fd(0), offset(0), result(0) {}

   
    sys::native_handle_t server_fd; 

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> h) {
        ov->user_data = h.address();
        
        if (op_type == 0) { 
            ring.submit_read(fd, buf, len, ov);
        } else if (op_type == 1) { 
            ring.submit_write(fd, buf, len, ov);
        } else if (op_type == 2) { 
            ring.submit_accept(fd, client_addr, client_len, ov);
        } else if (op_type == 3) { 
            ring.submit_sendfile(file_fd, fd, offset, len, ov);
        } else if (op_type == 4) {
            ring.submit_recvfrom(fd, buf, len, client_addr, client_len, ov);
        }
    }

    int await_resume() {
        return ov->result;
    }
};

}
