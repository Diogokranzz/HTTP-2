#pragma once
#include "Ring.hpp"
#include "BufferPool.hpp"
#include "../http/Router.hpp"
#include "../http/Parser.hpp"
#include "../http2/Session.hpp"
#include "../quic/UdpSocket.hpp"
#include "../quic/QuicSession.hpp"
#include "../coro/Task.hpp"
#include "../api/UserController.hpp"
#include "../tls/TlsContext.hpp"
#include "../tls/TlsSession.hpp"
#include <iostream>
#include <vector>
#include <fstream>

namespace core {

class Server {
public:
    Server(int port = 8080, const std::string& cert_file = "", const std::string& key_file = "") 
        : m_port(port), m_ring(4096), m_pool(10000) {
        m_ring.init();
        
        if (!cert_file.empty() && !key_file.empty()) {
            try {
                m_tls_ctx.init(cert_file, key_file);
                m_use_tls = true;
                std::cout << "[Server] TLS Enabled (Port " << port << ")\n";
            } catch (const std::exception& e) {
                std::cerr << "[Server] TLS Init Failed: " << e.what() << "\n";
                exit(1);
            }
        }
        
       
        {
            std::ofstream ofs("index.html");
            ofs << "<html><body><h1>DK Server Online</h1><p>Powered by C++23 & IOCP</p></body></html>";
        }

        
       
        #ifdef PLATFORM_WINDOWS
        m_file_handle = CreateFileA("index.html", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
        if (m_file_handle != INVALID_HANDLE_VALUE) {
            GetFileSizeEx(m_file_handle, &m_file_size);
        }
        #endif

        
        m_router.add(http::Method::HTTP_GET, "/api/hello", [](const http::Request& req) {
            http::Response res;
            res.content_type = "application/json";
            res.body = R"({"message": "Hello from DK API", "status": "fast"})";
            return res;
        });
        
        api::UserController::register_routes(m_router);
    }

    void run() {
        std::cout << "Server starting on port " << m_port << "...\n";
        
        
        auto tcp_task = accept_loop();
        auto udp_task = udp_listener();

        std::cout << "Engine Running. Press Ctrl+C to stop.\n";
        
        while (true) {
            m_ring.process_completions(true);
        }
    }

    http::Router& router() { return m_router; }

private:
    int m_port;
    core::Ring m_ring;
    core::BufferPool m_pool;
    http::Router m_router;
    tls::TlsContext m_tls_ctx;
    bool m_use_tls = false;
    
    #ifdef PLATFORM_WINDOWS
    HANDLE m_file_handle = INVALID_HANDLE_VALUE;
    LARGE_INTEGER m_file_size;
    #endif

    
    coro::IOAwaitable async_read(sys::native_handle_t fd, void* buf, size_t len, sys::NativeOverlapped* ov) {
        return coro::IOAwaitable(m_ring, fd, buf, len, ov);
    }

    coro::IOAwaitable async_write(sys::native_handle_t fd, const void* buf, size_t len, sys::NativeOverlapped* ov) {
        coro::IOAwaitable awaitable(m_ring, fd, (void*)buf, len, ov);
        awaitable.op_type = 1;
        return awaitable;
    }

    coro::IOAwaitable async_accept(sys::native_handle_t server_fd, void* buf, struct sockaddr* addr, int* addr_len, sys::NativeOverlapped* ov) {
        coro::IOAwaitable awaitable(m_ring, server_fd, buf, 0, ov);
        awaitable.op_type = 2;
        awaitable.client_addr = addr;
        awaitable.client_len = addr_len;
        return awaitable;
    }

    coro::IOAwaitable async_sendfile(sys::native_handle_t socket_fd, sys::os_fd_t file_fd, size_t offset, size_t count, sys::NativeOverlapped* ov) {
        coro::IOAwaitable awaitable(m_ring, socket_fd, nullptr, count, ov);
        awaitable.op_type = 3;
        awaitable.file_fd = file_fd;
        awaitable.offset = offset;
        return awaitable;
    }

    coro::IOAwaitable async_recvfrom(sys::native_handle_t fd, void* buf, size_t len, struct sockaddr* addr, int* addr_len, sys::NativeOverlapped* ov) {
        coro::IOAwaitable awaitable(m_ring, fd, buf, len, ov);
        awaitable.op_type = 4;
        awaitable.client_addr = addr;
        awaitable.client_len = addr_len;
        return awaitable;
    }

   
    coro::Task accept_loop() {
        sys::native_handle_t server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_fd == INVALID_SOCKET) throw std::runtime_error("Socket failed");

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(m_port);

        if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) throw std::runtime_error("Bind failed");
        if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) throw std::runtime_error("Listen failed");

        m_ring.attach(server_fd);

        while (true) {
            sockaddr_in client_addr;
            int client_len = sizeof(client_addr);
            sys::NativeOverlapped ov;
            ov.user_data = nullptr;
            
            char accept_buffer[1024];
            co_await async_accept(server_fd, accept_buffer, (sockaddr*)&client_addr, &client_len, &ov);

            #ifdef PLATFORM_WINDOWS
            sys::native_handle_t client_fd = ov.client_socket;
            if (client_fd != sys::INVALID_HANDLE_VALUE_NET) {
                int yes = 1;
                setsockopt((SOCKET)client_fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&server_fd, sizeof(server_fd));
                setsockopt((SOCKET)client_fd, IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(yes));
                
                m_ring.attach(client_fd);
                handle_client(client_fd);
            }
            #endif
        }
    }

    coro::Task handle_client(sys::native_handle_t client_fd) {
        std::cout << "[Server] Client Connected: " << (uint64_t)client_fd << "\n";
        void* buffer = m_pool.allocate();
        sys::NativeOverlapped ov;
        
        bool is_h2 = false;
        http2::Session h2_session;
        http::Parser parser;
        
        tls::TlsSession tls_session;
        if (m_use_tls) {
            tls_session.init(m_tls_ctx.get());
        }

        try {
           
            if (m_use_tls) {
                std::cout << "[Server] Starting TLS Handshake...\n";
                while (true) {
                    int ret = tls_session.do_handshake();
                   
                    std::vector<char> out;
                    tls_session.extract_encrypted_data(out);
                    if (!out.empty()) {
                         
                         const char* ptr = out.data();
                         size_t rem = out.size();
                         while (rem > 0) {
                             memset(&ov, 0, sizeof(ov));
                             int sent = co_await async_write(client_fd, ptr, rem, &ov);
                             if (sent <= 0) throw std::runtime_error("Handshake write failed");
                             ptr += sent;
                             rem -= sent;
                         }
                    }

                    if (ret == 0) {
                        std::cout << "[Server] TLS Handshake Complete\n";
                        break; 
                    }
                    if (ret == 1) { 
                         memset(&ov, 0, sizeof(ov));
                         int n = co_await async_read(client_fd, buffer, core::BufferPool::BLOCK_SIZE, &ov);
                         
                         if (n <= 0) throw std::runtime_error("Handshake read failed");
                         tls_session.feed_encrypted_data(buffer, n);
                    } else if (ret == 2) { 
                       
                    } else {
                        throw std::runtime_error("Handshake error");
                    }
                }

            }
            
            while (true) {
                memset(&ov, 0, sizeof(ov));
                int bytes_read = co_await async_read(client_fd, buffer, core::BufferPool::BLOCK_SIZE, &ov);
                if (bytes_read <= 0) break;

                std::vector<char> decrypted_data;
                const char* parse_ptr = (const char*)buffer;
                size_t parse_len = bytes_read;

                if (m_use_tls) {
                    if (tls_session.decrypt(buffer, bytes_read, decrypted_data) < 0) {
                        std::cerr << "[Server] TLS Decrypt Failed\n";
                        break;
                    }
                    parse_ptr = decrypted_data.data();
                    parse_len = decrypted_data.size();
                    if (parse_len == 0) continue; 
                }

              
                if (!is_h2 && parse_len >= 24 && memcmp(parse_ptr, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24) == 0) {
                    is_h2 = true;
                    h2_session.send_settings(); // Send server SETTINGS immediately
                }

                if (is_h2) {
                    if (!h2_session.on_data((const uint8_t*)parse_ptr, parse_len)) break;
                    
                   
                    std::vector<char> h2_out;
                    h2_session.consume_output(h2_out);
                    
                    if (!h2_out.empty()) {
                        if (m_use_tls) {
                            std::vector<char> encrypted;
                            tls_session.encrypt(h2_out.data(), h2_out.size(), encrypted);
                            const char* ptr = encrypted.data();
                            size_t rem = encrypted.size();
                            while (rem > 0) {
                                memset(&ov, 0, sizeof(ov));
                                int sent = co_await async_write(client_fd, ptr, rem, &ov);
                                if (sent <= 0) break;
                                ptr += sent;
                                rem -= sent;
                            }
                        } else {
                           
                            const char* ptr = h2_out.data();
                            size_t rem = h2_out.size();
                            while (rem > 0) {
                                memset(&ov, 0, sizeof(ov));
                                int sent = co_await async_write(client_fd, ptr, rem, &ov);
                                if (sent <= 0) break;
                                ptr += sent;
                                rem -= sent;
                            }
                        }
                    }
                    continue;
                }

               
                if (parser.parse(parse_ptr, parse_len) && parser.state() == http::Parser::State::COMPLETE) {
                    const auto& req = parser.request();
                    
                    http::Response res;
                    if (m_router.handle(req, res)) {
                        std::string s = res.to_string();
                        
                        if (m_use_tls) {
                            std::vector<char> encrypted;
                            tls_session.encrypt(s.data(), s.size(), encrypted);
                            const char* ptr = encrypted.data();
                            size_t rem = encrypted.size();
                            while (rem > 0) {
                                memset(&ov, 0, sizeof(ov));
                                int sent = co_await async_write(client_fd, ptr, rem, &ov);
                                if (sent <= 0) break;
                                ptr += sent;
                                rem -= sent;
                            }
                        } else {
                            const char* ptr = s.c_str();
                            size_t rem = s.size();
                            while (rem > 0) {
                                memset(&ov, 0, sizeof(ov));
                                int sent = co_await async_write(client_fd, ptr, rem, &ov);
                                if (sent <= 0) break;
                                ptr += sent;
                                rem -= sent;
                            }
                        }
                    }
                    else if (req.method == http::Method::HTTP_GET && (req.uri == "/" || req.uri == "/index.html")) {
                        
                         
                         if (m_use_tls) {
                             
                             std::vector<char> file_buf(m_file_size.QuadPart);
                             DWORD read;
                             OVERLAPPED file_ov = {0};
                             file_ov.Offset = 0;
                            
                             
                            
                             char header[256];
                             int header_len = snprintf(header, sizeof(header), 
                                 "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: text/html\r\n"
                                 "Connection: keep-alive\r\n"
                                 "Content-Length: %lld\r\n"
                                 "\r\n", 
                                 m_file_size.QuadPart
                             );
                             
                             std::vector<char> response_data;
                             response_data.insert(response_data.end(), header, header + header_len);
                             
                            
                             std::vector<char> encrypted;
                             tls_session.encrypt(response_data.data(), response_data.size(), encrypted);
                             
                             const char* ptr = encrypted.data();
                             size_t rem = encrypted.size();
                             while (rem > 0) {
                                 memset(&ov, 0, sizeof(ov));
                                 int sent = co_await async_write(client_fd, ptr, rem, &ov);
                                 if (sent <= 0) break;
                                 ptr += sent;
                                 rem -= sent;
                             }
                         } else {
                            
                             char header[256];
                             int header_len = snprintf(header, sizeof(header), 
                                 "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: text/html\r\n"
                                 "Connection: keep-alive\r\n"
                                 "Content-Length: %lld\r\n"
                                 "\r\n", 
                                 m_file_size.QuadPart
                             );

                             const char* ptr = header;
                             size_t rem = header_len;
                             while (rem > 0) {
                                memset(&ov, 0, sizeof(ov));
                                int sent = co_await async_write(client_fd, ptr, rem, &ov);
                                if (sent <= 0) break;
                                ptr += sent;
                                rem -= sent;
                             }
                             
                             #ifdef PLATFORM_WINDOWS
                             if (m_file_handle != INVALID_HANDLE_VALUE) {
                                 memset(&ov, 0, sizeof(ov));
                                 co_await async_sendfile(client_fd, m_file_handle, 0, m_file_size.QuadPart, &ov);
                             }
                             #endif
                         }
                    } else {
                        const char* resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                        if (m_use_tls) {
                            std::vector<char> encrypted;
                            tls_session.encrypt(resp, strlen(resp), encrypted);
                            const char* ptr = encrypted.data();
                            size_t rem = encrypted.size();
                            while (rem > 0) {
                                memset(&ov, 0, sizeof(ov));
                                int sent = co_await async_write(client_fd, ptr, rem, &ov);
                                if (sent <= 0) break;
                                ptr += sent;
                                rem -= sent;
                            }
                        } else {
                            co_await async_write(client_fd, resp, strlen(resp), &ov);
                        }
                        break; 
                    }
                    parser.reset();
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[Server] Client Error: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "[Server] Unknown Client Error\n";
        }
        
        m_pool.deallocate(buffer);
        closesocket((SOCKET)client_fd);
        co_return;
    }

    coro::Task udp_listener() {
        quic::UdpSocket sock;
        sock.init(m_port);
        m_ring.attach(sock.fd);
        quic::Engine engine;
        
        std::cout << "[Server] UDP/QUIC Listener on " << m_port << std::endl;

        while (true) {
            char buffer[1500];
            sockaddr_in client_addr;
            int client_len = sizeof(client_addr);
            sys::NativeOverlapped ov;
            ov.user_data = nullptr;

            co_await async_recvfrom(sock.fd, buffer, sizeof(buffer), (sockaddr*)&client_addr, &client_len, &ov);
            
            if (ov.result > 0) {
                engine.on_packet((uint8_t*)buffer, ov.result, (sockaddr*)&client_addr);
            }
        }
    }
};

}
