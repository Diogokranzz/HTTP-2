#pragma once
#include <string_view>
#include <optional>
#include <span>
#include <array>

namespace http {

enum class Method {
    HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_DELETE, HTTP_HEAD, HTTP_OPTIONS, HTTP_PATCH, HTTP_UNKNOWN
};

struct Header {
    std::string_view name;
    std::string_view value;
};

struct Request {
    Method method;
    std::string_view uri;
    int version_major;
    int version_minor;
    
    static constexpr size_t MAX_HEADERS = 32;
    std::array<Header, MAX_HEADERS> headers;
    size_t header_count = 0;
    
    std::string_view body;

    void reset() {
        header_count = 0;
        body = {};
        uri = {};
    }
};

}
