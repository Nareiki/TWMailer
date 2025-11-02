//
// Created by black on 11/2/25.
//

#include "server/mail_utils.h"
#include "helpers/readLineFromSocket.h"
#include "helpers/protocolValidation.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

namespace fs = std::filesystem;

/**
 * @brief Handles the SEND command for a simple mail server.
 *
 * Protocol (expected from client):
 *   1) sender username line
 *   2) receiver username line
 *   3) subject line
 *   4) message body lines terminated by a single line with a dot "."
 *
 * Storage layout:
 *   mail-spool/<receiver>/<nextId>.txt
 *     - First line:  "FROM: <sender>"
 *     - Second line: "SUBJECT: <subject>"
 *     - Blank line
 *     - Message body (verbatim; newline-separated)
 *
 * @param clientSocket Connected client socket (assumed blocking).
 * @return true on success (also responds "OK\n"), false on protocol/IO error (responds "ERR\n" where possible).
 *
 * @note This function performs input validation via is_valid_username() and is_valid_subject().
 * @note The function scans the receiver's inbox directory to find the next numeric message id.
 * @warning No concurrency control on id assignment—two concurrent SENDs to the same inbox may race.
 * @warning Message is stored verbatim; no size limit enforced here—upstream should guard against abuse.
 */
bool handle_send(int clientSocket) {
    // ----- Variables -----
    std::string sender, receiver, subject, line, message;

    // ----- Read header lines (sender / receiver / subject) -----
    // Each read expects a single line (without trailing '\n' in the variable).
    if (!readLineFromSocket(clientSocket, sender)) { return false; }
    if (!readLineFromSocket(clientSocket, receiver)) { return false; }
    if (!readLineFromSocket(clientSocket, subject)) { return false; }

    // ----- Validate protocol fields -----
    // Fail fast on invalid fields and notify client using the unified error helper.
    if (!is_valid_username(sender)) {
        std::cerr << "SEND: invalid sender '" << sender << "'\n";
        send_err_line(clientSocket);
        return false;
    }
    if (!is_valid_username(receiver)) {
        std::cerr << "SEND: invalid receiver '" << receiver << "'\n";
        send_err_line(clientSocket);
        return false;
    }
    if (!is_valid_subject(subject)) {
        std::cerr << "SEND: subject too long (" << subject.size() << ")\n";
        send_err_line(clientSocket);
        return false;
    }

    // ----- Read message body (dot-terminated) -----
    // Accumulates lines until a line containing exactly "." appears.
    // Lines are rejoined with '\n', preserving original line boundaries.
    while (true) {
        if (!readLineFromSocket(clientSocket, line)) { return false; }
        if (line == ".") { break; }
        message += line + "\n";
    }

    // ----- Resolve storage paths and ensure inbox exists -----
    fs::path basePath = "mail-spool";
    fs::path userInboxPath = basePath / receiver;
    fs::create_directories(userInboxPath); // idempotent; ensures receiver folder exists

    // ----- Determine next message id (scan existing numeric filenames) -----
    // Looks for files like "<id>.txt" and picks max(id)+1. Non-numeric stems are ignored.
    // NOTE: This is O(n) in number of files and not atomic; consider a metadata index or lock in the future.
    int nextId = 1;
    for (const auto& entry : fs::directory_iterator(userInboxPath)) {
        if (entry.is_regular_file()) {
            try {
                int id = std::stoi(entry.path().stem().string());
                if (id >= nextId) { nextId = id + 1; }
            } catch (...) {
                // Non-numeric filename; skip silently.
                continue;
            }
        }
    }

    // ----- Compose output filename -----
    fs::path filename = userInboxPath / (std::to_string(nextId) + ".txt");

    // ----- Persist message to disk -----
    // File format:
    //   FROM: <sender>
    //   SUBJECT: <subject>
    //
    //   <message...>
    std::ofstream out(filename);
    if (!out.is_open()) {
        // If we cannot open the file, inform the client.
        // NOTE: Using raw send() for consistency with existing code path.
        send(clientSocket, "ERR\n", 4, 0);
        return false;
    }

    out << "FROM: " << sender << "\n";
    out << "SUBJECT: " << subject << "\n\n";
    out << message;
    out.close();

    // ----- Acknowledge success -----
    send(clientSocket, "OK\n", 3, 0);
    return true;
}
