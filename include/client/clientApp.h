//
// Created by black on 11/2/25.
//

#ifndef TWMAILER_CLIENTAPP_H
#define TWMAILER_CLIENTAPP_H


#pragma once
#include <string>

class ClientApp {
public:
    ClientApp(const std::string& host, int port);
    ~ClientApp();

    int run();

private:
    int sock_ = -1;
    bool connect_socket(const std::string& host, int port);
    void close_socket();
};



#endif //TWMAILER_CLIENTAPP_H