//
// Created by black on 11/2/25.
//

#ifndef TWMAILER_PROTOCOL_IO_H
#define TWMAILER_PROTOCOL_IO_H
#pragma once
#include <string>

int  send_line(int sock, const std::string& s);
int  recv_line(int sock, std::string& out);
void trim_line(std::string& s);
bool iequal(const std::string& a, const std::string& b);
bool wait_readable(int fd, int timeout_ms);

#endif //TWMAILER_PROTOCOL_IO_H