// client.c
#include "client/clientApp.h"
#include <string>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    const std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
    const int         port = (argc > 2) ? std::atoi(argv[2]) : 6543;

    ClientApp app(host, port);
    return app.run();
}