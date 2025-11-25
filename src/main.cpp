#include "core/Server.hpp"
#include <iostream>

int main() {
    try {
       
        core::Server server(8080, "server.crt", "server.key");
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
