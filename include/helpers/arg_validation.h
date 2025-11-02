//
// Created by Christian Redl on 02.11.25.
//

#ifndef ARG_VALIDATION_H
#define ARG_VALIDATION_H

#include <string>
#include <cstdint>

bool is_valid_ipv4(const std::string &ip);
bool parse_port(const std::string &s, unsigned int &out_port);

#endif //ARG_VALIDATION_H
