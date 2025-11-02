//
// Created by Christian Redl on 01.11.25.
//
#include "server/server.h"
#include "server/mail_utils.h"

#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstdio>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "readLineFromSocket.h"

#define BUF 1024
#define BACKLOG 5

// ----- Static instance (for signal handling) -----
Server* Server::instance = nullptr;

/**
 * @file server.cpp
 * @brief Implements a minimal blocking TCP server handling a simple text protocol.
 *
 * Responsibilities:
 *  - Create, bind, and listen on a TCP socket.
 *  - Accept a single client at a time (blocking accept/IO).
 *  - Dispatch commands ("SEND", "QUIT") and respond with "ERR" for unknown commands.
 *  - Graceful shutdown on SIGINT via a static signal handler flag.
 *
 * Notes:
 *  - Uses SO_REUSEADDR (+ SO_REUSEPORT if available) to ease rapid restarts.
 *  - Accept loop is EINTR-aware and will retry unless shutdown is requested.
 *  - Signal handler avoids non-async-signal-safe operations (no iostream; only shutdown/close and atomics).
 *  - This implementation is single-threaded and handles one client connection at a time.
 *  - Networking is blocking; long-running handlers will block the main loop.
 *
 * Warnings:
 *  - No TLS/authorization; protocol is plaintext.
 *  - No per-client timeouts or read limits; upstream should enforce policies to avoid slowloris-style abuse.
 *  - `readLineFromSocket` include path differs from other files; ensure include paths are consistent project-wide.
 */

// ====== Ctors / Dtors =======================================================

/**
 * @brief Construct the server bound to a port. Registers SIGINT handler.
 * @param port TCP port to listen on (host byte order).
 */
Server::Server(int port)
    : port_(port),
      create_socket_(-1),
      new_socket_(-1),
      abortRequested_(false)
{
    instance = this;
    std::signal(SIGINT, &Server::signalHandler);
}

/**
 * @brief Destructor triggers a best-effort shutdown of sockets.
 */
Server::~Server() {
    shutdownServer();
}

// ====== Initialization / Socket Setup ======================================

/**
 * @brief Create, configure, bind, and listen on the server socket.
 * @throws Exits process on failure (per current design).
 *
 * Details:
 *  - AF_INET / SOCK_STREAM / TCP.
 *  - SO_REUSEADDR (+ SO_REUSEPORT if defined) to allow quick rebind.
 *  - Binds to INADDR_ANY on the configured port.
 *  - Listen backlog is BACKLOG.
 */
void Server::initSocket() {
    create_socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (create_socket_ == -1) {
        std::perror("socket");
        std::exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (::setsockopt(create_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0
#ifdef SO_REUSEPORT
        || ::setsockopt(create_socket_, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0
#endif
    ) {
        std::perror("setsockopt");
        std::exit(EXIT_FAILURE);
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // listen on all interfaces
    address.sin_port = htons(static_cast<uint16_t>(port_));

    if (::bind(create_socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::perror("bind");
        std::exit(EXIT_FAILURE);
    }

    if (::listen(create_socket_, BACKLOG) < 0) {
        std::perror("listen");
        std::exit(EXIT_FAILURE);
    }
}

// ====== Main Loop ===========================================================

/**
 * @brief Run the server: accept connections and serve one client at a time.
 *        Exits when SIGINT arrives or a fatal socket error occurs.
 */
void Server::run() {
    initSocket();

    std::cout << "Server running on port " << port_ << "...\n";

    sockaddr_in cliaddress{};
    while (!abortRequested_.load(std::memory_order_relaxed)) {

        socklen_t addrlen = sizeof(cliaddress);
        std::cout << "Waiting for connection...\n";

        new_socket_ = ::accept(create_socket_, reinterpret_cast<sockaddr*>(&cliaddress), &addrlen);
        if (new_socket_ == -1) {
            if (abortRequested_.load(std::memory_order_relaxed)) {
                std::cerr << "Accept interrupted due to SIGINT. Shutting down.\n";
                break;
            }
            if (errno == EINTR) {
                std::cerr << "Accept interrupted (EINTR), retrying...\n";
                continue;
            }
            if (errno == EBADF) { // listening socket was closed
                break;
            }
            std::perror("accept");
            continue;
        }

        std::cout << "Client connected from " << ::inet_ntoa(cliaddress.sin_addr)
                  << ":" << ntohs(cliaddress.sin_port) << "\n";

        // Handle this client synchronously; returns when client disconnects or QUIT is received.
        clientCommunication(new_socket_);
        new_socket_ = -1;
    }

    shutdownServer();
}

// ====== Shutdown / Cleanup ==================================================

/**
 * @brief Best-effort shutdown of client and listening sockets. Idempotent.
 * @note Uses shutdown(SHUT_RDWR) before close() to break out of blocking calls.
 */
void Server::shutdownServer() noexcept {
    if (new_socket_ != -1) {
        ::shutdown(new_socket_, SHUT_RDWR);
        ::close(new_socket_);
        new_socket_ = -1;
    }
    if (create_socket_ != -1) {
        ::shutdown(create_socket_, SHUT_RDWR);
        ::close(create_socket_);
        create_socket_ = -1;
    }
    std::cout << "Server shutdown complete.\n";
}

// ====== Signals =============================================================

/**
 * @brief Async-signal-safe handler for SIGINT. Sets abort flag and closes the listen socket.
 * @param sig Signal number (expects SIGINT).
 * @warning Avoids iostreams; only uses async-signal-safe functions and atomics.
 */
void Server::signalHandler(int sig) noexcept {
    if (sig == SIGINT && instance != nullptr) {
        // No iostream in signal handler!
        instance->abortRequested_.store(true, std::memory_order_relaxed);
        if (instance->create_socket_ != -1) {
            ::shutdown(instance->create_socket_, SHUT_RDWR);
            ::close(instance->create_socket_);
            instance->create_socket_ = -1;
        }
    }
}

// ====== Client Session ======================================================

/**
 * @brief Serve a single client over a blocking socket until QUIT or disconnect.
 * @param clientSocket Accepted client socket (owned by this function).
 *
 * Protocol:
 *  - Sends a welcome banner.
 *  - Reads line-based commands via readLineFromSocket().
 *  - Supported: "SEND" -> delegates to handle_send(); "QUIT"/"quit" -> close session.
 *  - Unknown commands -> responds "ERR\n".
 */
void Server::clientCommunication(int clientSocket) {
    const std::string welcome =
        "Welcome to myserver!\r\nPlease enter your commands...\r\n";
    (void)::send(clientSocket, welcome.c_str(), welcome.size(), 0);

    std::string command;
    for (;;) {
        command.clear();

        if (!readLineFromSocket(clientSocket, command)) {
            break; // connection closed / error
        }

        std::cout << "Command: " << command << std::endl;

        if (command == "SEND") {
            if (!handle_send(clientSocket)) {
                std::cerr << "SEND failed\n";
            }
            continue;
        }

        if (command == "LIST") {
            if (!handle_list(clientSocket)) {
                std::cerr << "LIST failed\n";
            }
            continue;
        }

        if (command == "READ") {
            if (!handle_read(clientSocket)) {
                std::cerr << "READ failed\n";
            }
            continue;
        }

        if (command == "DEL") {
            if (!handle_del(clientSocket)) {
                std::cerr << "DEL failed\n";
            }
            continue;
        }

        if (command == "QUIT" || command == "quit") {
            break;
        }

        // Unknown command: minimal error per current protocol.
        const std::string err = "ERR\n";
        (void)::send(clientSocket, err.c_str(), err.size(), 0);
    }

    // Finalize connection
    ::shutdown(clientSocket, SHUT_RDWR);
    ::close(clientSocket);
}
