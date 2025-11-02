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
#include <vector>
#include <algorithm>
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
    if (!readLineFromSocket(clientSocket, sender)) { return false; }
    if (!readLineFromSocket(clientSocket, receiver)) { return false; }
    if (!readLineFromSocket(clientSocket, subject)) { return false; }

    // ----- Validate protocol fields -----
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
    while (true) {
        if (!readLineFromSocket(clientSocket, line)) { return false; }
        if (line == ".") { break; }
        message += line + "\n";
    }

    // ----- Resolve storage paths and ensure inbox exists -----
    fs::path basePath = "mail-spool";
    fs::path userInboxPath = basePath / receiver;
    fs::create_directories(userInboxPath);

    // ----- Determine next message id (scan existing numeric filenames) -----
    int nextId = 1;
    for (const auto& entry : fs::directory_iterator(userInboxPath)) {
        if (entry.is_regular_file()) {
            try {
                int id = std::stoi(entry.path().stem().string());
                if (id >= nextId) { nextId = id + 1; }
            } catch (...) {
                continue;
            }
        }
    }

    // ----- Compose output filename -----
    fs::path filename = userInboxPath / (std::to_string(nextId) + ".txt");

    // ----- Persist message to disk -----
    std::ofstream out(filename);
    if (!out.is_open()) {
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

/**
 * @brief Handles the LIST command: returns the number of messages and their subjects.
 *
 * Protocol (client -> server):
 *   LIST\n
 *   <username>\n
 *
 * Response (server -> client):
 *   <count>\n
 *   <subject 1>\n
 *   ...
 *   <subject N>\n
 * If user unknown or no messages: "0\n"
 *
 * @param clientSocket Connected client socket (assumed blocking).
 * @return true on success (including the "0\n" case), false if the connection read fails.
 *
 * @note Only filenames with numeric stems "<id>.txt" are considered messages.
 * @warning No concurrency control while scanning; directory may change during iteration.
 */
bool handle_list(int clientSocket) {
    // ----- Read username -----
    std::string username;
    if (!readLineFromSocket(clientSocket, username)) { return false; }

    // ----- Validate username -----
    if (!is_valid_username(username)) {
        const std::string zero = "0\n";
        (void)::send(clientSocket, zero.c_str(), zero.size(), 0);
        return true;
    }

    // ----- Resolve inbox path -----
    fs::path inbox = fs::path("mail-spool") / username;
    if (!fs::exists(inbox) || !fs::is_directory(inbox)) {
        const std::string zero = "0\n";
        (void)::send(clientSocket, zero.c_str(), zero.size(), 0);
        return true;
    }

    // ----- Collect (id, subject) from files named "<id>.txt" -----
    struct Item { int id; std::string subject; };
    std::vector<Item> items;

    for (const auto& entry : fs::directory_iterator(inbox)) {
        if (!entry.is_regular_file()) { continue; }

        const std::string stem = entry.path().stem().string();
        int id = 0;
        if (!parse_message_id(stem, id)) { continue; }

        std::ifstream in(entry.path());
        if (!in.is_open()) { continue; }

        std::string line;
        std::string subject;

        // Expected header from handle_send():
        //   FROM: <sender>
        //   SUBJECT: <subject>
        //   <blank>
        if (std::getline(in, line)) { /* FROM line, ignored */ }
        if (std::getline(in, line)) {
            constexpr const char* kPrefix = "SUBJECT: ";
            if (line.rfind(kPrefix, 0) == 0) {
                subject = line.substr(std::char_traits<char>::length(kPrefix));
            } else {
                subject = line; // tolerant fallback
            }
        }

        items.push_back({ id, std::move(subject) });
    }

    // ----- Sort by numeric id ascending -----
    std::sort(items.begin(), items.end(),
              [](const Item& a, const Item& b){ return a.id < b.id; });

    // ----- Send count then each subject line -----
    {
        std::ostringstream oss;
        oss << items.size() << "\n";
        const std::string head = oss.str();
        (void)::send(clientSocket, head.c_str(), head.size(), 0);
    }

    for (const auto& it : items) {
        const std::string line = it.subject + "\n";
        (void)::send(clientSocket, line.c_str(), line.size(), 0);
    }

    return true;
}

/**
 * @brief Handles the READ command: returns the full stored message content.
 *
 * Protocol (client -> server):
 *   READ\n
 *   <username>\n
 *   <message-id>\n     // numeric file id: "<id>.txt"
 *
 * Response (server -> client):
 *   OK\n
 *   <complete message file content>\n
 * or:
 *   ERR\n
 *
 * @param clientSocket Connected client socket (assumed blocking).
 * @return true on success path (including ERR reply), false only if socket read fails upfront.
 *
 * @note Message id is interpreted as the numeric filename stem.
 */
bool handle_read(int clientSocket) {
    // ----- Read parameters -----
    std::string username, idStr;
    if (!readLineFromSocket(clientSocket, username)) { return false; }
    if (!readLineFromSocket(clientSocket, idStr))   { return false; }

    // ----- Validate -----
    if (!is_valid_username(username)) {
        send_err_line(clientSocket);
        return true;
    }

    int id = 0;
    if (!parse_message_id(idStr, id)) {
        send_err_line(clientSocket);
        return true;
    }

    // ----- Resolve path & open file -----
    fs::path path = fs::path("mail-spool") / username / (std::to_string(id) + ".txt");
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        send_err_line(clientSocket);
        return true;
    }

    // ----- Read entire file -----
    std::ostringstream buf;
    buf << in.rdbuf();
    const std::string content = buf.str();

    // ----- Respond with OK and file content (verbatim) -----
    send_ok_line(clientSocket);
    if (!content.empty()) {
        (void)::send(clientSocket, content.c_str(), content.size(), 0);
        if (content.back() != '\n') {
            static const char nl = '\n';
            (void)::send(clientSocket, &nl, 1, 0);
        }
    } else {
        static const char nl = '\n';
        (void)::send(clientSocket, &nl, 1, 0);
    }

    return true;
}

/**
 * @brief Handles the DEL command: deletes a stored message file.
 *
 * Protocol (client -> server):
 *   DEL\n
 *   <username>\n
 *   <message-id>\n     // numeric file id: "<id>.txt"
 *
 * Response (server -> client):
 *   OK\n
 * or:
 *   ERR\n
 *
 * @param clientSocket Connected client socket (assumed blocking).
 * @return true on success path (including ERR reply), false only if socket read fails upfront.
 *
 * @note Deletion uses std::filesystem::remove with error_code and reports ERR on failure.
 */
bool handle_del(int clientSocket) {
    // ----- Read parameters -----
    std::string username, idStr;
    if (!readLineFromSocket(clientSocket, username)) { return false; }
    if (!readLineFromSocket(clientSocket, idStr))   { return false; }

    // ----- Validate -----
    if (!is_valid_username(username)) {
        send_err_line(clientSocket);
        return true;
    }

    int id = 0;
    if (!parse_message_id(idStr, id)) {
        send_err_line(clientSocket);
        return true;
    }

    // ----- Remove file -----
    fs::path path = fs::path("mail-spool") / username / (std::to_string(id) + ".txt");
    std::error_code ec;
    const bool removed = fs::remove(path, ec);
    if (ec || !removed) {
        send_err_line(clientSocket);
        return true;
    }

    // ----- Acknowledge success -----
    send_ok_line(clientSocket);
    return true;
}
