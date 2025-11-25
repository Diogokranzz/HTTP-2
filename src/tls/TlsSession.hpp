#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <vector>
#include <iostream>

namespace tls {

class TlsSession {
public:
    TlsSession() : m_ssl(nullptr), m_rbio(nullptr), m_wbio(nullptr) {}
    
    ~TlsSession() {
        if (m_ssl) {
            SSL_free(m_ssl);
        }
        if (m_rbio) {
            BIO_free(m_rbio);
        }
    }

    void init(SSL_CTX* ctx) {
        m_ssl = SSL_new(ctx);
        
        BIO* internal_bio = nullptr;
        BIO* network_bio = nullptr;

        
        if (BIO_new_bio_pair(&internal_bio, 0, &network_bio, 0) != 1) {
            throw std::runtime_error("Failed to create BIO pair");
        }

        SSL_set_bio(m_ssl, internal_bio, internal_bio);
        
        
        m_rbio = network_bio; 
        m_wbio = network_bio; 
    }

   
    int do_handshake() {
        int ret = SSL_accept(m_ssl);
        if (ret == 1) return 0; // Complete

        int err = SSL_get_error(m_ssl, ret);
        if (err == SSL_ERROR_WANT_READ) return 1;
        if (err == SSL_ERROR_WANT_WRITE) return 2;
        
        print_error();
        return -1;
    }

   
    void feed_encrypted_data(const void* in_data, size_t in_len) {
        if (in_len > 0) {
            BIO_write(m_rbio, in_data, (int)in_len);
        }
    }

    
    int decrypt(const void* in_data, size_t in_len, std::vector<char>& out_decrypted) {
       
        feed_encrypted_data(in_data, in_len);

        
        char buf[4096];
        while (true) {
            int read = SSL_read(m_ssl, buf, sizeof(buf));
            if (read > 0) {
                out_decrypted.insert(out_decrypted.end(), buf, buf + read);
            } else {
                int err = SSL_get_error(m_ssl, read);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                    break; 
                }
                if (err == SSL_ERROR_ZERO_RETURN) return 0; 
                return -1; 
            }
        }
        return 1;
    }

   
    int encrypt(const void* in_data, size_t in_len, std::vector<char>& out_encrypted) {
        if (in_len > 0) {
            int written = SSL_write(m_ssl, in_data, (int)in_len);
            if (written <= 0) return -1;
        }

       
        char buf[4096];
        while (true) {
            int read = BIO_read(m_wbio, buf, sizeof(buf));
            if (read > 0) {
                out_encrypted.insert(out_encrypted.end(), buf, buf + read);
            } else {
                if (BIO_should_retry(m_wbio)) break;
                return -1;
            }
        }
        return 1;
    }
    
   
    void extract_encrypted_data(std::vector<char>& out_encrypted) {
        char buf[4096];
        while (true) {
            int read = BIO_read(m_wbio, buf, sizeof(buf));
            if (read > 0) {
                out_encrypted.insert(out_encrypted.end(), buf, buf + read);
            } else {
                if (BIO_should_retry(m_wbio)) break;
                break;
            }
        }
    }

private:
    SSL* m_ssl;
    BIO* m_rbio;
    BIO* m_wbio;

    void print_error() {
        unsigned long err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(err, buf, sizeof(buf));
        std::cerr << "OpenSSL Error: " << buf << "\n";
    }
};

}
