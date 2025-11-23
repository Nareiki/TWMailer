//
// Created by Christian Redl on 01.11.25.
//

#ifndef SERVER_H
#define SERVER_H

#include <atomic>
#include "../helpers/threadPool.h"

class Server {
public:
    explicit Server(int port);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void run();

    static void signalHandler(int sig) noexcept;

private:
    int port_;
    int create_socket_;
    int new_socket_;

    std::atomic<bool> abortRequested_{false};

    ThreadPool threadPool_;

    static Server* instance;

    void initSocket();
    void shutdownServer() noexcept;

    static void clientCommunication(int clientSocket);
};

#endif // SERVER_H
