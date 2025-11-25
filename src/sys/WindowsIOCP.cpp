#include "../core/Ring.hpp"
#include <iostream>
#include <stdexcept>

#ifdef PLATFORM_WINDOWS

namespace core {

Ring::Ring(unsigned entries) : m_iocp(NULL) {
    (void)entries;
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
}

Ring::~Ring() {
    if (m_iocp) {
        CloseHandle(m_iocp);
    }
    WSACleanup();
}

void Ring::init() {
    m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (!m_iocp) {
        throw std::runtime_error("CreateIoCompletionPort failed");
    }
}

void Ring::attach(sys::native_handle_t fd) {
    HANDLE res = CreateIoCompletionPort((HANDLE)fd, m_iocp, (ULONG_PTR)fd, 0);
    if (!res) {
        
        
    }
}

void Ring::submit_read(sys::native_handle_t fd, void* buf, size_t len, sys::NativeOverlapped* ov) {
    ZeroMemory(&ov->ol, sizeof(WSAOVERLAPPED));
    
    WSABUF wsaBuf;
    wsaBuf.buf = static_cast<char*>(buf);
    wsaBuf.len = static_cast<ULONG>(len);
    
    DWORD flags = 0;
    DWORD bytesReceived = 0;
    
    int result = WSARecv(fd, &wsaBuf, 1, &bytesReceived, &flags, &ov->ol, NULL);
    
    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
          
        }
    }
}

void Ring::submit_write(sys::native_handle_t fd, const void* buf, size_t len, sys::NativeOverlapped* ov) {
    ZeroMemory(&ov->ol, sizeof(WSAOVERLAPPED));
   
    WSABUF wsaBuf;
    wsaBuf.buf = static_cast<char*>(const_cast<void*>(buf));
    wsaBuf.len = static_cast<ULONG>(len);
    
    DWORD bytesSent = 0;
    
    int result = WSASend(fd, &wsaBuf, 1, &bytesSent, 0, &ov->ol, NULL);
    
    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            // std::cerr << "WSASend failed: " << err << "\n";
        }
    }
}

void Ring::submit_accept(sys::native_handle_t server_fd, void* output_buffer, int* client_len, sys::NativeOverlapped* ov) {
    (void)client_len;
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        return; 
    }
    
    
    
    ov->client_socket = client_socket; 

    static LPFN_ACCEPTEX lpAcceptEx = NULL;
    if (!lpAcceptEx) {
        GUID GuidAcceptEx = WSAID_ACCEPTEX;
        DWORD dwBytes;
        WSAIoctl(server_fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
                 &GuidAcceptEx, sizeof(GuidAcceptEx),
                 &lpAcceptEx, sizeof(lpAcceptEx),
                 &dwBytes, NULL, NULL);
    }
    
    ZeroMemory(&ov->ol, sizeof(WSAOVERLAPPED));
    DWORD bytesReceived = 0;
    
    BOOL res = lpAcceptEx(server_fd, client_socket, 
                          output_buffer, 0, 
                          sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, 
                          &bytesReceived, &ov->ol);
                          
    if (res == FALSE) {
        int err = WSAGetLastError();
        if (err != ERROR_IO_PENDING) {
             // std::cerr << "AcceptEx failed: " << err << "\n";
             closesocket(client_socket);
        }
    }
}

void Ring::submit_sendfile(sys::os_fd_t file_fd, sys::native_handle_t socket_fd, size_t offset, size_t count, sys::NativeOverlapped* ov) {
    ZeroMemory(&ov->ol, sizeof(WSAOVERLAPPED));
    ov->ol.Offset = static_cast<DWORD>(offset);
    ov->ol.OffsetHigh = static_cast<DWORD>(offset >> 32);
    
    BOOL res = TransmitFile(socket_fd, file_fd, static_cast<DWORD>(count), 0, &ov->ol, NULL, 0);
    
    if (res == FALSE) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            
        }
    }
}

void Ring::submit_recvfrom(sys::native_handle_t fd, void* buffer, size_t len, struct sockaddr* addr, int* addr_len, sys::NativeOverlapped* ov) {
    ZeroMemory(&ov->ol, sizeof(WSAOVERLAPPED));
    
    WSABUF wsaBuf;
    wsaBuf.buf = (char*)buffer;
    wsaBuf.len = (ULONG)len;
    
    DWORD flags = 0;
    
    int res = WSARecvFrom((SOCKET)fd, &wsaBuf, 1, NULL, &flags, addr, addr_len, &ov->ol, NULL);
    
    if (res == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            
        }
    }
}

int Ring::process_completions(bool wait_for_completion) {
    LPOVERLAPPED out_ov = NULL;
    ULONG_PTR key = 0;
    DWORD bytes_transferred = 0;
    
    BOOL res = GetQueuedCompletionStatus(m_iocp, &bytes_transferred, &key, &out_ov, wait_for_completion ? INFINITE : 0);
    
    if (out_ov) {
        sys::NativeOverlapped* ov = reinterpret_cast<sys::NativeOverlapped*>(out_ov);
        
        if (ov->user_data) {
             ov->result = static_cast<int>(bytes_transferred);
             std::coroutine_handle<>::from_address(ov->user_data).resume();
        }
        return 1;
    }
    
    return 0;
}

}

#endif
