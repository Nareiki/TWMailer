// client.c
#include "client/clientApp.h"
#include <string>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
    unsigned int port = (argc > 2) ? std::atoi(argv[2]) : 6543;

    if (port <= 0 || port > 65535) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>\n";
        std::cerr << "Error: Invalid Port number\n";
        return 2;
    }
    std::cout << host << ":" << port << std::endl;
    std::cout << port << std::endl;

    ClientApp app(host, port);
    return app.run();
}
