//
// Created by black on 11/2/25.
//

#include "client/commands.h"
#include "helpers/protocol_Io.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "mypw.h"

/**
 * @file commands.cpp
 * @brief Implements client-side command handlers for the simple mail protocol.
 *
 * Responsibilities:
 *  - Read the server's optional two-line welcome banner.
 *  - Handle multi-step client interactions for SEND, LIST, READ, and DEL.
 *  - Use protocol helpers (send_line / recv_line / trim_line / wait_readable).
 *
 * Protocol:
 *  - Line-oriented text (CRLF terminated).
 *  - Server replies with status lines like "OK" or "ERR", and additional lines for some commands.
 *
 * Notes:
 *  - All I/O is blocking.
 *  - On socket errors or EOF, functions return false to signal the caller to abort the session.
 */

// ====== Session Welcome ======================================================

/**
 * @brief Reads and prints the server's welcome banner (up to two lines).
 * @param sock Connected socket.
 *
 * Behavior:
 *  - Attempts to read the first line (blocking).
 *  - If another line is already available within ~100 ms, reads and prints it as well.
 */
void read_welcome(int sock) {
    std::string line;
    if (recv_line(sock, line) > 0) {
        std::cout << line << "\n";
    }
    if (wait_readable(sock, 100)) {
        if (recv_line(sock, line) > 0) {
            std::cout << line << "\n";
        }
    }
}

// ====== LOGIN ===============================================================

/**
 * @brief Performs a LOGIN exchange with the server.
 *
 * Sends username and password (hidden via getpass()) to the server and waits
 * for an authentication response. The server replies with "OK" on success or
 * "ERR" on failure.
 *
 * @param sock Connected client socket.
 * @return true if the server responded with "OK", false otherwise.
 *
 * @note Caller must have already sent the command "LOGIN".
 */
bool cmd_login(int sock)
{
    std::string s;

    std::cout << "username: ";
    std::cout.flush();
    if (!std::getline(std::cin, s)) {
        return false;
    }
    trim_line(s);
    if (send_line(sock, s) == -1) {
        perror("send");
        return false;
    }

    std::cout.flush();
    s = getpass();

    trim_line(s);
    if (send_line(sock, s) == -1) {
        perror("send");
        return false;
    }

    std::string resp;
    int r = recv_line(sock, resp);
    if (r <= 0) {
        if (r < 0) perror("recv");
        else std::cerr << "closed\n";
        return false;
    }

    std::cout << "<< " << resp << "\n";

    if (!iequal(resp, "OK")) {
        std::cerr << "<< Login failed\n";
        return false;
    }

    return true;
}

// ====== SEND ================================================================

/**
 * @brief Interactive handler for the SEND command.
 * @param sock Connected socket.
 * @return true on success (including server "OK" or "ERR" print), false on local I/O error.
 *
 * Flow:
 *  - Prompt user for sender, receiver, subject, and message body (dot-terminated).
 *  - Transmit each line to the server (CRLF).
 *  - Read and print exactly one status line from the server.
 */
bool cmd_send(int sock) {
    std::string s;

    std::cout << "receiver: ";
    std::cout.flush();
    if (!std::getline(std::cin, s)) {
        return false;
    }
    trim_line(s);
    if (send_line(sock, s) == -1) {
        perror("send");
        return false;
    }

    std::cout << "subject: ";
    std::cout.flush();
    if (!std::getline(std::cin, s)) {
        return false;
    }
    trim_line(s);
    if (send_line(sock, s) == -1) {
        perror("send");
        return false;
    }

    std::cout << "message ('.' to end):\n";
    for (;;) {
        if (!std::getline(std::cin, s)) {
            perror("stdin");
            return false;
        }
        trim_line(s);
        if (s == ".") {
            if (send_line(sock, ".") == -1) {
                perror("send");
                return false;
            }
            break;
        }
        if (send_line(sock, s) == -1) {
            perror("send");
            return false;
        }
    }

    std::string resp;
    int r = recv_line(sock, resp);
    if (r <= 0) {
        if (r < 0) {
            perror("recv");
        } else {
            std::cerr << "closed\n";
        }
        return false;
    }
    std::cout << "<< " << resp << "\n";
    if (!iequal(resp, "OK")) {
        std::cerr << "<< Server error\n";
    }
    return true;
}

// ====== LIST ================================================================

/**
 * @brief Interactive handler for the LIST command (lists subjects for a user).
 * @param sock Connected socket.
 * @return true on success, false on local I/O error.
 *
 * Flow:
 *  - Prompt for username and send it.
 *  - Read a count line (N).
 *  - Read N subject lines and print them enumerated.
 */
bool cmd_list(int sock) {

    std::string countLine;
    int r = recv_line(sock, countLine);
    std::cout << countLine << "\n";
    if (r <= 0) {
        if (r < 0) {
            perror("recv");
        } else {
            std::cerr << "closed\n";
        }
        return false;
    }
    std::cout << "<< " << countLine << "\n";

    long n = std::strtol(countLine.c_str(), nullptr, 10);
    if (n < 0) {
        n = 0;
    }
    for (long i = 0; i < n; ++i) {
        std::string subj;
        r = recv_line(sock, subj);
        if (r <= 0) {
            break;
        }
        std::cout << i + 1 << ": " << subj << "\n";
    }
    return true;
}

// ====== READ ================================================================

/**
 * @brief Interactive handler for the READ command (prints a full stored message).
 * @param sock Connected socket.
 * @return true on success (even if server returns ERR), false on local I/O error.
 *
 * Flow:
 *  - Prompt for username and message id; send both.
 *  - Expect a status line; if "OK", read remaining bytes (with short waits) and print verbatim.
 */
bool cmd_read(int sock) {

    std::string id;

    std::cout << "id: ";
    std::cout.flush();
    if (!std::getline(std::cin, id)) {
        return false;
    }
    std::cout << id << "\n";
    trim_line(id);
    if (send_line(sock, id) == -1) {
        perror("send");
        return false;
    }

    std::string status;
    std::cout << "test" << std::endl;
    int r = recv_line(sock, status);
    if (r <= 0) {
        if (r < 0) {
            perror("recv");
        } else {
            std::cerr << "closed\n";
        }
        return false;
    }
    std::cout << "<< " << status << "\n";
    if (!iequal(status, "OK")) {
        std::cerr << "<< Server error\n";
        return true;
    }

    std::string all;
    for (;;) {
        if (!wait_readable(sock, 150)) {
            break;
        }
        char buf[4096];
        ssize_t n = recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) {
            break;
        }
        all.append(buf, buf + n);
        if (n < (ssize_t)sizeof(buf)) {
            if (!wait_readable(sock, 50)) {
                break;
            }
        }
    }
    if (!all.empty() && all.back() != '\n') {
        all.push_back('\n');
    }
    std::cout << all;
    return true;
}

// ====== DEL =================================================================

/**
 * @brief Interactive handler for the DEL command (delete a stored message).
 * @param sock Connected socket.
 * @return true on success (prints server response), false on local I/O error.
 *
 * Flow:
 *  - Prompt for username and message id; send both.
 *  - Read and print a single status line from server.
 */
bool cmd_del(int sock) {

    std::string id;

    std::cout << "id: ";
    std::cout.flush();
    if (!std::getline(std::cin, id)) {
        return false;
    }
    trim_line(id);
    if (send_line(sock, id) == -1) {
        perror("send");
        return false;
    }

    std::string resp;
    int r = recv_line(sock, resp);
    if (r <= 0) {
        if (r < 0) {
            perror("recv");
        } else {
            std::cerr << "closed\n";
        }
        return false;
    }
    std::cout << "<< " << resp << "\n";
    return true;
}
