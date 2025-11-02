//
// Created by black on 11/2/25.
//

#include "helpers/protocolValidation.h"
#include <climits>
#include <cctype>
#include <sys/socket.h>

/**
 * @file protocolValidation.cpp
 * @brief Implements basic input validation and protocol response helpers.
 *
 * Provides:
 *   - Username validation (lowercase letters + digits, ≤ 8 chars)
 *   - Subject validation (≤ 80 chars, empty allowed)
 *   - Message ID parsing (positive integer, ≤ INT_MAX)
 *   - Standard "OK\n" / "ERR\n" socket response utilities
 *
 * These functions support simple text-based protocols used by the mail server.
 *
 * @note All validation is purely syntactic; no existence checks or reserved-name rules are applied.
 * @note Sending helpers use ::send() directly for simplicity; non-blocking safety or error handling is not included.
 */

// ====== Username Validation =================================================

/**
 * @brief Checks if a username is syntactically valid.
 *
 * Rules:
 *  - Non-empty, max 8 characters.
 *  - Lowercase a–z and digits 0–9 only.
 *
 * @param s Username string.
 * @return true if valid, false otherwise.
 */
bool is_valid_username(const std::string& s) {
    if (s.empty() || s.size() > 8) return false;
    for (unsigned char c : s) {
        if (!(std::isdigit(c) || (c >= 'a' && c <= 'z'))) { return false; }
    }
    return true;
}

// ====== Subject Validation ==================================================

/**
 * @brief Checks if a subject line is valid.
 *
 * Rules:
 *  - Max 80 characters.
 *  - Empty subject is allowed.
 *
 * @param s Subject string.
 * @return true if valid, false otherwise.
 */
bool is_valid_subject(const std::string& s) {
    return s.size() <= 80; // empty allowed
}

// ====== Message ID Parsing ==================================================

/**
 * @brief Parses a numeric message ID string into an integer.
 *
 * Rules:
 *  - Must contain only digits.
 *  - Must represent a number between 1 and INT_MAX.
 *
 * @param s Input string (digits only).
 * @param out_id Reference to an integer to receive the parsed ID.
 * @return true if successfully parsed and valid, false otherwise.
 *
 * @note Uses 64-bit intermediate (long long) to safely detect overflow.
 */
bool parse_message_id(const std::string& s, int& out_id) {
    if (s.empty()) { return false; }
    long long v = 0;
    for (unsigned char c : s) {
        if (!std::isdigit(c)) { return false; }
        v = v * 10 + (c - '0');
        if (v > INT_MAX) { return false; }
    }
    if (v < 1) { return false; }
    out_id = static_cast<int>(v);
    return true;
}

// ====== Socket Response Helpers ============================================

/**
 * @brief Sends a standard error line ("ERR\n") to a client socket.
 * @param sock Socket descriptor.
 * @note Uses blocking ::send(); ignores return value.
 */
void send_err_line(int sock) {
    static const char err[] = "ERR\n";
    (void)::send(sock, err, sizeof(err) - 1, 0);
}

/**
 * @brief Sends a standard success line ("OK\n") to a client socket.
 * @param sock Socket descriptor.
 * @note Uses blocking ::send(); ignores return value.
 */
void send_ok_line(int sock) {
    static const char ok[] = "OK\n";
    (void)::send(sock, ok, sizeof(ok) - 1, 0);
}
