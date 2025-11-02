//
// Created by Christian Redl on 02.11.25.
//

#ifndef SERVER_ARGS_H
#define SERVER_ARGS_H

#include <string>

int parse_server_args(int argc, char *argv[], unsigned int &server_port, std::string &mail_spool);

#endif //SERVER_ARGS_H
