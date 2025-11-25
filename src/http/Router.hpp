#pragma once
#include "Request.hpp"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <string_view>

namespace http {

struct Response {
    int status = 200;
    std::string content_type = "text/plain";
    std::string body;
    
    std::string to_string() const {
        std::string res = "HTTP/1.1 " + std::to_string(status) + " OK\r\n";
        res += "Content-Type: " + content_type + "\r\n";
        res += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        res += "Connection: keep-alive\r\n\r\n";
        res += body;
        return res;
    }
};

using Handler = std::function<Response(const Request&)>;

class Router {
public:
    void add(Method method, const std::string& path, Handler handler) {
        
        std::string key = method_to_string(method) + ":" + path;
        routes_[key] = handler;
    }

    bool handle(const Request& req, Response& res) {
        std::string key = method_to_string(req.method) + ":" + std::string(req.uri);
        if (routes_.contains(key)) {
            res = routes_[key](req);
            return true;
        }
        return false;
    }

private:
    std::string method_to_string(Method m) {
        switch(m) {
            case Method::HTTP_GET: return "GET";
            case Method::HTTP_POST: return "POST";
            case Method::HTTP_PUT: return "PUT";
            case Method::HTTP_DELETE: return "DELETE";
            default: return "UNKNOWN";
        }
    }

    std::unordered_map<std::string, Handler> routes_;
};

}
