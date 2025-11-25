#include "Parser.hpp"
#include <cctype>

namespace http {

Parser::Parser() {
    reset();
}

void Parser::reset() {
    m_state = State::METHOD_START;
    m_req.reset();
    m_ptr_start = nullptr;
}

bool Parser::parse(const char* data, size_t len) {
    const char* p = data;
    const char* end = data + len;

    while (p < end) {
        char c = *p;
        
        switch (m_state) {
            case State::METHOD_START:
                if (!std::isalpha(c)) return false;
                m_ptr_start = p;
                m_state = State::METHOD;
                break;
                
            case State::METHOD:
                if (c == ' ') {
                    std::string_view m(m_ptr_start, p - m_ptr_start);
                    if (m == "GET") m_req.method = Method::HTTP_GET;
                    else if (m == "POST") m_req.method = Method::HTTP_POST;
                    else m_req.method = Method::HTTP_UNKNOWN;
                    m_state = State::URI_START;
                } else if (!std::isalpha(c)) {
                    return false;
                }
                break;
                
            case State::URI_START:
                if (c == ' ') return false; 
                m_ptr_start = p;
                m_state = State::URI;
                break;
                
            case State::URI:
                if (c == ' ') {
                    m_req.uri = std::string_view(m_ptr_start, p - m_ptr_start);
                    m_state = State::VERSION_H;
                } else if (c == '\r' || c == '\n') {
                    return false;
                }
                break;
                
            case State::VERSION_H: if (c == 'H') m_state = State::VERSION_HT; else return false; break;
            case State::VERSION_HT: if (c == 'T') m_state = State::VERSION_HTT; else return false; break;
            case State::VERSION_HTT: if (c == 'T') m_state = State::VERSION_HTTP; else return false; break;
            case State::VERSION_HTTP: if (c == 'P') m_state = State::VERSION_SLASH; else return false; break;
            case State::VERSION_SLASH: if (c == '/') m_state = State::VERSION_MAJOR; else return false; break;
            
            case State::VERSION_MAJOR:
                if (std::isdigit(c)) {
                    m_req.version_major = c - '0';
                    m_state = State::VERSION_DOT;
                } else return false;
                break;
                
            case State::VERSION_DOT:
                if (c == '.') m_state = State::VERSION_MINOR;
                else return false;
                break;
                
            case State::VERSION_MINOR:
                if (std::isdigit(c)) {
                    m_req.version_minor = c - '0';
                    m_state = State::NEWLINE_1;
                } else return false;
                break;
                
            case State::NEWLINE_1:
                if (c == '\r') {} 
                else if (c == '\n') m_state = State::HEADER_KEY_START;
                else return false;
                break;

            case State::HEADER_KEY_START:
                if (c == '\r') {
                    m_state = State::NEWLINE_3;
                } else if (c == '\n') {
                    m_state = State::BODY; 
                    return true; 
                } else {
                    m_ptr_start = p;
                    m_state = State::HEADER_KEY;
                }
                break;
                
            case State::HEADER_KEY:
                if (c == ':') {
                    if (m_req.header_count < Request::MAX_HEADERS) {
                        m_req.headers[m_req.header_count].name = std::string_view(m_ptr_start, p - m_ptr_start);
                    }
                    m_state = State::HEADER_COLON;
                } else if (c == '\r' || c == '\n') {
                    return false;
                }
                break;
                
            case State::HEADER_COLON:
                if (c == ' ') m_state = State::HEADER_VALUE_START;
                else return false; 
                break;
                
            case State::HEADER_VALUE_START:
                m_ptr_start = p;
                m_state = State::HEADER_VALUE;
                break;
                
            case State::HEADER_VALUE:
                if (c == '\r') {
                    if (m_req.header_count < Request::MAX_HEADERS) {
                        m_req.headers[m_req.header_count].value = std::string_view(m_ptr_start, p - m_ptr_start);
                        m_req.header_count++;
                    }
                    m_state = State::NEWLINE_2;
                } else if (c == '\n') {
                     if (m_req.header_count < Request::MAX_HEADERS) {
                        m_req.headers[m_req.header_count].value = std::string_view(m_ptr_start, p - m_ptr_start);
                        m_req.header_count++;
                    }
                    m_state = State::HEADER_KEY_START;
                }
                break;
                
            case State::NEWLINE_2:
                if (c == '\n') m_state = State::HEADER_KEY_START;
                else return false;
                break;
                
            case State::NEWLINE_3:
                if (c == '\n') {
                    m_state = State::BODY;
                    
                    m_state = State::COMPLETE;
                    return true;
                }
                else return false;
                break;
                
            case State::BODY:
            case State::COMPLETE:
                return true;
                
            case State::PARSE_ERROR:
                return false;
        }
        p++;
    }
    return true;
}

}
