//
// Created by black on 11/2/25.
//

#ifndef TWMAILER_COMMANDS_H
#define TWMAILER_COMMANDS_H

#pragma once
#include <string>

bool cmd_login(int sock);
bool cmd_send(int sock);
bool cmd_list(int sock);
bool cmd_read(int sock);
bool cmd_del (int sock);

void read_welcome(int sock);

#endif //TWMAILER_COMMANDS_H