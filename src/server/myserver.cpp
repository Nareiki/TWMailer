#include <iostream>
#include <ostream>

#include "security/blacklist.h"
#include "server/server.h"
#include "server/ldap/ldap_utils.h"

int main(int argc, char* argv[]) {
    unsigned int port = (argc > 1) ? std::atoi(argv[1]) : 6543;
    std::string path = (argc > 2) ? argv[2] : "127.0.0.1";

    std::cout << port << std::endl;
    std::cout << path << std::endl;
    std::cout << argc << std::endl;

    for (int i = 1; i < argc; i++) {
        std::cout << i << argv[i] << std::endl;
    }

    load_blacklist("blacklist.txt");

    Server server(port);
    server.run();
    return 0;
}
