#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#include <iostream>

namespace tls {

class TlsContext {
public:
    TlsContext() : m_ctx(nullptr) {}
    
    ~TlsContext() {
        if (m_ctx) {
            SSL_CTX_free(m_ctx);
        }
    }

    static int alpn_select_cb(SSL *ssl, const unsigned char **out, unsigned char *outlen,
                              const unsigned char *in, unsigned int inlen, void *arg) {
      
        if (SSL_select_next_proto((unsigned char**)out, outlen, 
                                  (unsigned char*)"\x02h2\x08http/1.1", 12, 
                                  in, inlen) == OPENSSL_NPN_NEGOTIATED) {
            return SSL_TLSEXT_ERR_OK;
        }
        return SSL_TLSEXT_ERR_NOACK;
    }

    void init(const std::string& cert_file, const std::string& key_file) {
        m_ctx = SSL_CTX_new(TLS_server_method());
        if (!m_ctx) {
            throw std::runtime_error("Failed to create SSL_CTX");
        }

        
        SSL_CTX_set_min_proto_version(m_ctx, TLS1_2_VERSION);
        
  
        SSL_CTX_set_alpn_select_cb(m_ctx, alpn_select_cb, nullptr);
        
      
        if (SSL_CTX_use_certificate_file(m_ctx, cert_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
            print_error();
            throw std::runtime_error("Failed to load certificate");
        }

        if (SSL_CTX_use_PrivateKey_file(m_ctx, key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
            print_error();
            throw std::runtime_error("Failed to load private key");
        }

        if (!SSL_CTX_check_private_key(m_ctx)) {
            throw std::runtime_error("Private key does not match certificate");
        }
    }

    SSL_CTX* get() { return m_ctx; }

private:
    SSL_CTX* m_ctx;

    void print_error() {
        unsigned long err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(err, buf, sizeof(buf));
        std::cerr << "OpenSSL Error: " << buf << "\n";
    }
};

}
