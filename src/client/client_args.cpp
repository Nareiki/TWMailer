//
// Created by Christian Redl on 02.11.25.
//


#include "client/client_args.h"
#include "helpers/arg_validation.h"

#include <string>

int parse_client_args(int argc, char *argv[], std::string &ip, unsigned int &port) {
    if (argc != 3) return 1;

    if (!is_valid_ipv4(argv[1])) return 1;

    unsigned int p{};
    if (!parse_port(argv[2], p)) return 3;

    ip = argv[1];
    port = p;
    return 0;
}
