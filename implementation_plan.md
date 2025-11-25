# Implementation Plan - World Class Server Evolution

## Goal
Transform the raw C++23 IOCP server into a production-grade application server with Security (HTTPS), Intelligence (API/JSON), and Efficiency (HTTP/2).

## Phase 8: Level 2 - API Server (The Brain)
We will start here as it builds directly on the existing HTTP/1.1 parser.
- **Router**: A high-performance `Trie` or `HashMap` based router to dispatch `GET /api/users` to specific handlers.
- **JSON**: Integration of `simdjson` (fastest JSON parser) to handle payloads.
- **Architecture**:
    - `http::Router`: Maps paths to `std::function` handlers.
    - `http::Response`: Helper to construct JSON responses efficiently.

## Phase 9: Level 1 - Security (The Shield)
Async TLS using OpenSSL BIO pairs.
- **Challenge**: OpenSSL is synchronous.
- **Solution**: Use `BIO_s_mem()` (Memory BIO).
    - **Read Path**: Socket -> IOCP Buffer -> `BIO_write` (Encrypted) -> `SSL_read` (Decrypted) -> App.
    - **Write Path**: App -> `SSL_write` (Plain) -> `BIO_read` (Encrypted) -> IOCP Buffer -> Socket.
- **Components**:
    - `sys::SslContext`: Manages certificates.
    - `sys::SslStream`: Wraps a raw socket and handles the encryption layer.

## Phase 10: Level 3 - HTTP/2 (The Future)
Binary multiplexing.
- **Framing**: Implement the binary frame parser (Length, Type, Flags, StreamID, Payload).
- **HPACK**: Huffman encoding/decoding for headers.
- **Streams**: Virtual channels over a single TCP connection.

## Execution Order
1.  **API Server**: Immediate value, no heavy external linking (can mock simdjson if needed).
2.  **HTTPS**: Requires OpenSSL setup.
3.  **HTTP/2**: Complex logic, depends on HTTPS usually.
