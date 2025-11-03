// client.c
#include "client/clientApp.h"
#include "client/client_args.h"
#include <string>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {

    // Simple validation:
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>\n";
        std::cerr << "Error: wrong number of arguments\n";
        return 1;
    }

    const std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
    const int         port = (argc > 2) ? std::atoi(argv[2]) : 6543;

    //parse_client_args(argc, argv, host, port);

    if (port <= 0 || port > 65535) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>\n";
        std::cerr << "Error: Invalid Port number\n";
        return 2;
    }

    ClientApp app(host, port);
    return app.run();
}
