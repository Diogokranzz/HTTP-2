# Phase 9: HTTPS/TLS Implementation Plan

## Goal
Enable secure HTTPS communication on port 443 (or 8443) using OpenSSL.
The implementation must be **asynchronous** and **non-blocking**, integrating with the existing IOCP/Coroutines architecture.

## Challenge
OpenSSL is traditionally blocking. To make it async, we will use **BIO Pairs** (Memory BIOs).
- **Network BIO**: The "internal" buffer where OpenSSL writes encrypted data to be sent to the network, and reads encrypted data received from the network.
- **Application BIO**: The interface for our application to write cleartext data (to be encrypted) and read cleartext data (decrypted).

## Architecture

### 1. `TlsSession` Class
A wrapper around `SSL*` and `SSL_CTX*`.

```cpp
class TlsSession {
public:
    void init(SSL_CTX* ctx);
    void set_fd(int fd); // Not needed for BIO pair, but maybe for info
    
    // Handshake
    // Returns:
    //  0: Complete
    //  1: Need Read (from network)
    //  2: Need Write (to network)
    // -1: Error
    int do_handshake();

    // Data Transfer
    int encrypt(const void* in_data, size_t in_len, std::vector<char>& out_encrypted);
    int decrypt(const void* in_encrypted, size_t in_len, std::vector<char>& out_decrypted);

private:
    SSL* m_ssl = nullptr;
    BIO* m_rbio = nullptr; // Read BIO (Network -> SSL)
    BIO* m_wbio = nullptr; // Write BIO (SSL -> Network)
};
```

### 2. `SSL_CTX` Management
Global initialization of OpenSSL context, loading certificates (`server.crt`, `server.key`).

### 3. Integration in `Server::handle_client`

Current Flow:
`async_read` -> `buffer` -> `Parser`

New Flow (HTTPS):
`async_read` -> `encrypted_buffer` -> `TlsSession::decrypt` -> `decrypted_buffer` -> `Parser`

Write Flow:
`Response` -> `string` -> `TlsSession::encrypt` -> `encrypted_buffer` -> `async_write`

### 4. State Machine
The `handle_client` coroutine needs to handle the TLS handshake state before processing HTTP.

```cpp
// Handshake Loop
while (!handshake_complete) {
    int ret = tls.do_handshake();
    if (ret == NEED_READ) {
        // Read from socket, write to BIO
        int n = co_await async_read(...);
        tls.write_to_bio(buffer, n);
    } else if (ret == NEED_WRITE) {
        // Read from BIO, write to socket
        tls.read_from_bio(buffer);
        co_await async_write(...);
    }
}
```

## Prerequisites
- OpenSSL Libraries (`libssl`, `libcrypto`)
- OpenSSL Headers (`<openssl/ssl.h>`, etc.)
- Valid Certificate/Key pair (Self-signed for dev)

## Step-by-Step
1.  **Environment Setup**: Install OpenSSL.
2.  **Cert Generation**: Script to generate `server.crt` and `server.key`.
3.  **`TlsSession` Implementation**: Implement the BIO logic.
4.  **Server Integration**: Modify `Server.hpp` to support TLS mode.
5.  **Verification**: Test with `curl -k https://localhost:8443`.
