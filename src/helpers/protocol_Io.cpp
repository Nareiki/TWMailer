//
// Created by black on 11/2/25.
//

#include "helpers/protocol_Io.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <string>
#include <unistd.h>

int send_line(int sock, const std::string& s) {
    ssize_t n = send(sock, s.c_str(), s.size(), 0);
    if (n < 0) return -1;
    n = send(sock, "\r\n", 2, 0);
    if (n < 0) return -1;
    return 0;
}

int recv_line(int sock, std::string& out) {
    out.clear();
    char c;
    for (;;) {
        ssize_t n = recv(sock, &c, 1, 0);
        if (n == 0) return 0;
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (c == '\n') break;
        if (c != '\r') out.push_back(c);
    }
    return 1;
}

void trim_line(std::string& s) {
    size_t a = 0, b = s.size();
    while (b && (s[b-1] == '\n' || s[b-1] == '\r' || s[b-1] == ' ' || s[b-1] == '\t')) --b;
    while (a < b && (s[a] == ' ' || s[a] == '\t')) ++a;
    if (a || b != s.size()) s = s.substr(a, b - a);
}

bool iequal(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    return true;
}

bool wait_readable(int fd, int timeout_ms) {
    fd_set rfds; FD_ZERO(&rfds); FD_SET(fd, &rfds);
    timeval tv; tv.tv_sec = timeout_ms / 1000; tv.tv_usec = (timeout_ms % 1000) * 1000;
    int r = select(fd + 1, &rfds, nullptr, nullptr, &tv);
    return r > 0;
}
