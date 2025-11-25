#pragma once
#include "Request.hpp"
#include <span>

namespace http {

class Parser {
public:
    enum class State {
        METHOD_START, METHOD,
        URI_START, URI,
        VERSION_H, VERSION_HT, VERSION_HTT, VERSION_HTTP, VERSION_SLASH, VERSION_MAJOR, VERSION_DOT, VERSION_MINOR,
        NEWLINE_1,
        HEADER_KEY_START, HEADER_KEY,
        HEADER_COLON, HEADER_SPACE,
        HEADER_VALUE_START, HEADER_VALUE,
        NEWLINE_2,
        NEWLINE_3,
        BODY,
        COMPLETE,
        PARSE_ERROR
    };

    Parser();
    
    void reset();
    
    
    bool parse(const char* data, size_t len);
    
    const Request& request() const { return m_req; }
    State state() const { return m_state; }

private:
    State m_state;
    Request m_req;
    
    
    const char* m_ptr_start;
};

}
