#pragma once
#include "../http/Router.hpp"
#include "../http/Json.hpp"
#include <string>
#include <vector>

namespace api {

class UserController {
public:
    static void register_routes(http::Router& router) {
       
        router.add(http::Method::HTTP_GET, "/api/users", [](const http::Request& req) {
            http::Response res;
            res.content_type = "application/json";
           
            res.body = http::Json::serialize({
                {"id", "1"},
                {"name", "Diogo"},
                {"role", "Admin"}
            });
            return res;
        });

        
        router.add(http::Method::HTTP_POST, "/api/users", [](const http::Request& req) {
            http::Response res;
            res.content_type = "application/json";
            
            std::string name = http::Json::get_value(req.body, "name");
            if (name.empty()) {
                res.status = 400;
                res.body = R"({"error": "Name required"})";
            } else {
                res.status = 201;
                res.body = http::Json::serialize({
                    {"message", "User created"},
                    {"name", name}
                });
            }
            return res;
        });
    }
};

}
