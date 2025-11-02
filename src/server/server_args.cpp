//
// Created by Christian Redl on 02.11.25.
//

#include "server/server_args.h"

#include <filesystem>

#include "helpers/arg_validation.h"

int parse_server_args(int argc, char *argv[], unsigned int &server_port, std::string &mail_spool) {
    if (argc != 3) return 1;

    unsigned int p{};
    if (!parse_port(argv[1], p)) return 2;

    std::filesystem::__cxx11::path dir = argv[2];
    if (!exists(dir) || !is_directory(dir)) {
        create_directory(dir);
    }

    server_port = p;
    mail_spool = argv[2];
    return 0;
}
