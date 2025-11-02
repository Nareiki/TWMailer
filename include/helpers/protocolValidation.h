//
// Created by black on 11/2/25.
//



#ifndef TWMAILER_PROTOCOLVALIDATION_H
#define TWMAILER_PROTOCOLVALIDATION_H

#include <string>
#include <cctype>
#include <climits>
#include <sys/socket.h>

bool is_valid_username(const std::string& s);
bool is_valid_subject(const std::string& s);
bool parse_message_id(const std::string& s, int& out_id);

void send_err_line(int sock);
void send_ok_line(int sock);

#endif //TWMAILER_PROTOCOLVALIDATION_H