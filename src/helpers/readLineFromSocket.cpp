//
// Created by black on 11/2/25.
//

#include "helpers/readLineFromSocket.h"
#include <sys/socket.h>

/**
 * @file readLineFromSocket.cpp
 * @brief Provides a blocking utility function to read a single line from a socket.
 *
 * Behavior:
 *  - Reads one byte at a time using recv().
 *  - Stops when a newline character ('\n') is encountered.
 *  - Carriage returns ('\r') are ignored to support CRLF or LF line endings.
 *  - Output excludes both '\r' and '\n'.
 *
 * @param socket   Connected socket descriptor (blocking mode assumed).
 * @param outLine  Reference to a string that will contain the line read (without trailing newline).
 * @return true if a line was successfully read, false if the connection closed or recv() failed.
 *
 * @note This function blocks until either a newline or EOF occurs.
 * @note No timeout or buffer length limit is enforced here — upstream should prevent large unbounded reads.
 * @warning Reading one byte at a time is simple but inefficient; consider buffered reads for high-throughput scenarios.
 */
bool readLineFromSocket(int socket, std::string& outLine) {
    outLine.clear();
    char ch;

    while (true) {
        ssize_t n = recv(socket, &ch, 1, 0);
        if (n <= 0) { return false; }          // connection closed or error
        if (ch == '\n') { break; }          // end of line (stop)
        if (ch != '\r') { outLine += ch; }     // skip carriage return; append other characters
    }

    return true;
}
