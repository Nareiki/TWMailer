//
// Created by black on 11/2/25.
//

#ifndef TWMAILER_READLINEFROMSOCKET_H
#define TWMAILER_READLINEFROMSOCKET_H


#include <string>
#include <unistd.h>

bool readLineFromSocket(int socket, std::string& outLine);

#endif //TWMAILER_READLINEFROMSOCKET_H