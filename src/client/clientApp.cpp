//
// Created by black on 11/2/25.
//

#include "client/clientApp.h"
#include "helpers/protocol_Io.h"
#include "client/commands.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/**
 * @file clientApp.cpp
 * @brief Implements the interactive client application (REPL) for the simple mail protocol.
 *
 * Responsibilities:
 *  - Establish a TCP connection to the server.
 *  - Read the optional server welcome banner.
 *  - Run an interactive command loop (REPL) sending user-entered commands.
 *  - Delegate multi-step protocol flows to command handlers (SEND/LIST/READ/DEL).
 *
 * Protocol:
 *  - Line-oriented text over TCP (CRLF terminated).
 *  - Commands: SEND, LIST, READ, DEL, QUIT.
 *
 * Notes:
 *  - I/O is blocking and single-connection per process.
 *  - Error handling prints diagnostics and terminates the session where appropriate.
 *  - Helpers in protocol_Io.h provide line send/receive and small utilities.
 */

// ====== Construction / Destruction ==========================================

/**
 * @brief Constructs the client app and connects to the server.
 * @param host Server IP or hostname (expects dotted-quad for inet_aton).
 * @param port TCP port.
 * @note Exits the process with failure if the socket cannot be established.
 */
ClientApp::ClientApp(const std::string& host, int port) {
    if (!connect_socket(host, port)) {
        std::exit(EXIT_FAILURE);
    }
    read_welcome(sock_);
}

/**
 * @brief Destructor closes the socket (best effort, idempotent).
 */
ClientApp::~ClientApp() {
    close_socket();
}

// ====== Connection Management ===============================================

/**
 * @brief Create and connect a blocking TCP socket to the given host/port.
 * @param host Dotted-quad IP string (e.g., "127.0.0.1").
 * @param port TCP port.
 * @return true on success, false on error (and prints diagnostics).
 *
 * @note On success, prints a short connection message to stdout.
 */
bool ClientApp::connect_socket(const std::string& host, int port) {
    sock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == -1) {
        perror("socket");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (!inet_aton(host.c_str(), &addr.sin_addr)) {
        std::cerr << "bad host\n";
        close_socket();
        return false;
    }

    if (::connect(sock_, (sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close_socket();
        return false;
    }

    std::cout << "Connected to " << host << ":" << port << "\n";
    return true;
}

/**
 * @brief Closes the current socket if open. Safe to call multiple times.
 */
void ClientApp::close_socket() {
    if (sock_ != -1) {
        ::shutdown(sock_, SHUT_RDWR);
        ::close(sock_);
        sock_ = -1;
    }
}

// ====== REPL / Command Dispatch =============================================

/**
 * @brief Runs the interactive loop: read a command, send to server, dispatch handler.
 * @return EXIT_SUCCESS on normal termination, EXIT_FAILURE on IO errors.
 *
 * Flow:
 *  - Prompts with ">> ".
 *  - Sends the raw command line.
 *  - QUIT: reads one status line and exits the loop.
 *  - SEND/LIST/READ/DEL: delegates to specialized handlers in client/commands.
 *  - Unknown command: expects a single-line response (e.g., "ERR").
 */
int ClientApp::run() {
    std::string line;
    for (;;) {
        std::cout << ">> ";
        std::cout.flush();
        if (!std::getline(std::cin, line)) {
            break;
        }
        trim_line(line);
        if (line.empty()) {
            continue;
        }

        if (send_line(sock_, line) == -1) {
            perror("send");
            return EXIT_FAILURE;
        }

        if (iequal(line, "QUIT")) {
            std::string resp;
            int r = recv_line(sock_, resp);
            if (r > 0) {
                std::cout << "<< " << resp << "\n";
            }
            break;
        }

        if (iequal(line, "LOGIN")) {
            cmd_login(sock_);
        } else if (iequal(line, "SEND")) {
            if (!cmd_send(sock_)) {
                return EXIT_FAILURE;
            }
        } else if (iequal(line, "LIST")) {
            if (!cmd_list(sock_)) {
                return EXIT_FAILURE;
            }
        } else if (iequal(line, "READ")) {
            if (!cmd_read(sock_)) {
                return EXIT_FAILURE;
            }
        } else if (iequal(line, "DEL")) {
            if (!cmd_del(sock_)) {
                return EXIT_FAILURE;
            }
        } else {
            // Fallback to a single-line reply (e.g., "ERR")
            std::string resp;
            int r = recv_line(sock_, resp);
            if (r <= 0) {
                if (r < 0) {
                    perror("recv");
                } else {
                    std::cerr << "closed\n";
                }
                return EXIT_FAILURE;
            }
            std::cout << "<< " << resp << "\n";
        }
    }
    return EXIT_SUCCESS;
}
