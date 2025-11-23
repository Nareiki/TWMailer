//
// Created by black on 11/2/25.
//

#ifndef TWMAILER_MAIL_UTILS_H
#define TWMAILER_MAIL_UTILS_H


#include <string>

bool handle_login(int clientSocket, const std::string &clientIp, bool& authenticated, std::string& currentUser);
bool handle_send(int clientSocket, const std::string& username);
bool handle_list(int clientSocket, const std::string& username);
bool handle_read(int clientSocket, const std::string& username);
bool handle_del (int clientSocket, const std::string& username);

#endif //TWMAILER_MAIL_UTILS_H