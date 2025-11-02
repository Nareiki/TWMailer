//
// Created by Christian Redl on 02.11.25.
//

#include "../../include/helpers/arg_validation.h"

#include <iostream>
#include <ostream>
#include <arpa/inet.h>

bool is_valid_ipv4(const std::string &ip) {
    in_addr a{};
    return inet_pton(AF_INET, ip.c_str(), &a) == 1;
}

bool parse_port(const std::string &s, unsigned int &out_port) {
    char* end = nullptr;

    unsigned long val = std::strtoul(s.c_str(), &end, 10);

    std::cout << s.c_str() << " " << end << std::endl;
    if (end == s.c_str() || *end != '\0')
        return false;

    if (val == 0 || val > 65535) return false;

    out_port = static_cast<unsigned int>(val);
    return true;
}