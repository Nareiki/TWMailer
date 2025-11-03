#include <iostream>
#include <ostream>

#include "server/server.h"
#include "server/server_args.h"

int main(int argc, char* argv[]) {

    unsigned int port{};
    std::string path{};

    int result = parse_server_args(argc, argv, port, path);

    if (result != 0) {
        std::cerr << "Usage: " << argv[0] << " <port> <mail-spool-directory>\n";
        switch (result) {
            case 1: std::cerr << "Error: wrong number of arguments\n"; break;
            case 2: std::cerr << "Error: Invalid Port number\n"; break;
        }
        return result;
    }

    Server server(port);
    server.run();
    return 0;
}
