#include <iostream>
#include <ostream>

#include "server/server.h"

int main(int argc, char* argv[]) {
    unsigned int port = (argc > 1) ? std::atoi(argv[2]) : 6543;
    std::string path = (argc > 2) ? argv[2] : "127.0.0.1";

    Server server(port);
    server.run();
    return 0;
}
