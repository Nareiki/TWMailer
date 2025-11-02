// client.c
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cctype>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/time.h>
#include <sys/select.h>


#define BUF 1024
#define PORT 6543

static int send_line(int sock, const std::string& s) {
    ssize_t n = send(sock, s.c_str(), s.size(), 0);
    if (n < 0) return -1;
    n = send(sock, "\r\n", 2, 0);
    if (n < 0) return -1;
    return 0;
}

static int recv_line(int sock, std::string& out) {
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

static void trim_line(std::string& s) {
    size_t a = 0, b = s.size();
    while (b && (s[b-1] == '\n' || s[b-1] == '\r' || s[b-1] == ' ' || s[b-1] == '\t')) --b;
    while (a < b && (s[a] == ' ' || s[a] == '\t')) ++a;
    if (a || b != s.size()) s = s.substr(a, b - a);
}

static bool iequal(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    }
    return true;
}

static bool starts_with_icase(const std::string& s, const std::string& p) {
    if (s.size() < p.size()) return false;
    for (size_t i = 0; i < p.size(); ++i) {
        if (std::tolower((unsigned char)s[i]) != std::tolower((unsigned char)p[i])) return false;
    }
    return true;
}

static bool wait_readable(int fd, int timeout_ms) {
    fd_set rfds; FD_ZERO(&rfds); FD_SET(fd, &rfds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int r = select(fd + 1, &rfds, NULL, NULL, &tv);
    return r > 0 && FD_ISSET(fd, &rfds);
}

int main(int argc, char** argv) {
    const char* host = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? std::atoi(argv[2]) : 6543;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) { perror("socket"); return EXIT_FAILURE; }

    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons((uint16_t)port);
    if (!inet_aton(host, &addr.sin_addr)) { fprintf(stderr, "bad host\n"); close(sock); return EXIT_FAILURE; }

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == -1) { perror("connect"); close(sock); return EXIT_FAILURE; }

    std::string line;
    if (recv_line(sock, line) > 0) std::cout << line << "\n";
    if (wait_readable(sock, 100)) if (recv_line(sock, line) > 0) std::cout << line << "\n";

    for (;;) {
        std::cout << ">> ";
        std::cout.flush();
        if (!std::getline(std::cin, line)) break;
        trim_line(line);
        if (line.empty()) continue;

        if (send_line(sock, line) == -1) { perror("send"); break; }

        if (iequal(line, "QUIT")) {
            std::string resp;
            int r = recv_line(sock, resp);
            if (r <= 0) break;
            std::cout << "<< " << resp << "\n";
            break;
        }

        if (iequal(line, "SEND")) {
            std::string s;
            std::cout << "sender: "; std::cout.flush();
            if (!std::getline(std::cin, s)) {
                break;
            }
            trim_line(s);

            if (send_line(sock, s) == -1) { perror("send"); break; }

            std::cout << "receiver: "; std::cout.flush();
            if (!std::getline(std::cin, s)) {
                break;
            }
            trim_line(s);

            if (send_line(sock, s) == -1) { perror("send"); break; }

            std::cout << "subject: "; std::cout.flush();
            if (!std::getline(std::cin, s)) {
                break;
            }
            trim_line(s);

            if (send_line(sock, s) == -1) { perror("send"); break; }

            std::cout << "message ('.' to end):\n";
            for (;;) {
                if (!std::getline(std::cin, s)) { perror("stdin"); break; }
                trim_line(s);
                if (s == ".") { if (send_line(sock, ".") == -1) { perror("send"); } break; }
                if (send_line(sock, s) == -1) { perror("send"); break; }
            }

            std::string resp;
            int r = recv_line(sock, resp);
            if (r <= 0) { if (r < 0) perror("recv"); else std::cerr << "closed\n"; break; }
            std::cout << "<< " << resp << "\n";
            if (!iequal(resp, "OK")) { std::cerr << "<< Server error\n"; continue; }
            continue;
        }

        if (starts_with_icase(line, "LIST")) {
            std::string user;
            if (!std::getline(std::cin, user) || user.size() == 0) {}
        }

        if (iequal(line, "LIST")) {
            std::string user;
            std::cout << "user: "; std::cout.flush();
            if (!std::getline(std::cin, user)) {
                break;
            }
            trim_line(user);

            if (send_line(sock, user) == -1) { perror("send"); break; }

            std::string countLine;
            int r = recv_line(sock, countLine);
            if (r <= 0) { if (r < 0) perror("recv"); else std::cerr << "closed\n"; break; }
            std::cout << "<< " << countLine << "\n";
            long n = std::strtol(countLine.c_str(), nullptr, 10);
            if (n < 0) n = 0;
            for (long i = 0; i < n; ++i) {
                std::string subj;
                r = recv_line(sock, subj);
                if (r <= 0) break;
                std::cout << i+1 << ": " << subj << "\n";
            }
            continue;
        }

        if (iequal(line, "READ")) {
            std::string user, id;
            std::cout << "user: "; std::cout.flush();
            if (!std::getline(std::cin, user)) {
                break;
            }
            trim_line(user);
            if (send_line(sock, user) == -1) { perror("send"); break; }
            std::cout << "id: "; std::cout.flush();
            if (!std::getline(std::cin, id)) {
                break;
            }
            trim_line(id);
            if (send_line(sock, id) == -1) { perror("send"); break; }

            std::string status;
            int r = recv_line(sock, status);
            if (r <= 0) { if (r < 0) perror("recv"); else std::cerr << "closed\n"; break; }
            std::cout << "<< " << status << "\n";
            if (!iequal(status, "OK")) { std::cerr << "<< Server error\n"; continue; }

            std::string chunk, all;
            for (;;) {
                if (!wait_readable(sock, 150)) break;
                char buf[4096];
                ssize_t n = recv(sock, buf, sizeof(buf), 0);
                if (n <= 0) break;
                all.append(buf, buf + n);
                if (n < (ssize_t)sizeof(buf)) {
                    if (!wait_readable(sock, 50)) break;
                }
            }
            if (!all.empty() && all.back() != '\n') all.push_back('\n');
            std::cout << all;
            continue;
        }

        if (iequal(line, "DEL")) {
            std::string user, id;
            std::cout << "user: "; std::cout.flush();
            if (!std::getline(std::cin, user)) {
                break;
            }
            trim_line(user);
            if (send_line(sock, user) == -1) { perror("send"); break; }
            std::cout << "id: "; std::cout.flush();
            if (!std::getline(std::cin, id)) {
                break;
            }
            trim_line(id);
            if (send_line(sock, id) == -1) { perror("send"); break; }

            std::string resp;
            int r = recv_line(sock, resp);
            if (r <= 0) { if (r < 0) perror("recv"); else std::cerr << "closed\n"; break; }
            std::cout << "<< " << resp << "\n";
            continue;
        }

        std::string resp;
        int r = recv_line(sock, resp);
        if (r <= 0) { if (r < 0) perror("recv"); else std::cerr << "closed\n"; break; }
        std::cout << "<< " << resp << "\n";
    }

    if (sock != -1) { shutdown(sock, SHUT_RDWR); close(sock); sock = -1; }
    return EXIT_SUCCESS;
}
