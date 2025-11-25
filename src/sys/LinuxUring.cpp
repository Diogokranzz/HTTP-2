#include "../core/Ring.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>

#ifdef PLATFORM_LINUX

namespace core {

Ring::Ring(unsigned entries) {
    if (io_uring_queue_init(entries, &m_ring, 0) < 0) {
        throw std::runtime_error("io_uring_queue_init failed");
    }
}

Ring::~Ring() {
    io_uring_queue_exit(&m_ring);
}

void Ring::init() {
   
}

void Ring::attach(sys::native_handle_t fd) {
   
}

void Ring::submit_read(sys::native_handle_t fd, void* buf, size_t len, sys::NativeOverlapped* ov) {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);
    if (!sqe) {
        std::cerr << "Ring full in submit_read\n";
        return;
    }
    
    io_uring_prep_read(sqe, fd, buf, len, 0);
    io_uring_sqe_set_data(sqe, ov);
}

void Ring::submit_write(sys::native_handle_t fd, const void* buf, size_t len, sys::NativeOverlapped* ov) {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);
    if (!sqe) {
        std::cerr << "Ring full in submit_write\n";
        return;
    }
    
    io_uring_prep_write(sqe, fd, buf, len, 0);
    io_uring_sqe_set_data(sqe, ov);
}

void Ring::submit_accept(sys::native_handle_t server_fd, struct sockaddr* client_addr, int* client_len, sys::NativeOverlapped* ov) {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);
    if (!sqe) {
        std::cerr << "Ring full in submit_accept\n";
        return;
    }
    
    // io_uring_prep_accept takes socklen_t*
    io_uring_prep_accept(sqe, server_fd, client_addr, (socklen_t*)client_len, 0);
    io_uring_sqe_set_data(sqe, ov);
}

void Ring::submit_sendfile(sys::native_handle_t socket_fd, sys::os_fd_t file_fd, size_t offset, size_t count, sys::NativeOverlapped* ov) {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);
    if (!sqe) {
        std::cerr << "Ring full in submit_sendfile\n";
        return;
    }
    
   
    loff_t off = offset;
    io_uring_prep_splice(sqe, file_fd, &off, socket_fd, -1, count, 0);
    io_uring_sqe_set_data(sqe, ov);
}

void Ring::submit_recvfrom(sys::native_handle_t fd, void* buf, size_t len, struct sockaddr* addr, int* addr_len, sys::NativeOverlapped* ov) {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);
    if (!sqe) {
        std::cerr << "Ring full in submit_recvfrom\n";
        return;
    }

  
    
    io_uring_prep_recv(sqe, fd, buf, len, 0);
    io_uring_sqe_set_data(sqe, ov);
}

int Ring::process_completions(bool wait_for_completion) {
    struct io_uring_cqe* cqe;
    int ret;
    
    if (wait_for_completion) {
        ret = io_uring_wait_cqe(&m_ring, &cqe);
    } else {
        ret = io_uring_peek_cqe(&m_ring, &cqe);
    }
    
    if (ret < 0) {
        if (ret == -EAGAIN) return 0;
        return 0; // Error
    }
    
    if (cqe) {
        sys::NativeOverlapped* ov = (sys::NativeOverlapped*)io_uring_cqe_get_data(cqe);
        if (ov && ov->user_data) {
            ov->result = cqe->res;
            std::coroutine_handle<>::from_address(ov->user_data).resume();
        }
        io_uring_cqe_seen(&m_ring, cqe);
        return 1;
    }
    
    return 0;
}

}

#endif
