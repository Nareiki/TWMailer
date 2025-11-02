//
// Created by black on 11/2/25.
//

#ifndef TWMAILER_MAIL_UTILS_H
#define TWMAILER_MAIL_UTILS_H


#include <string>

bool handle_send(int clientSocket);
bool handle_list(int clientSocket);
bool handle_read(int clientSocket);
bool handle_del (int clientSocket);

#endif //TWMAILER_MAIL_UTILS_H