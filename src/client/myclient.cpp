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
#include <sys/time.h>
#include <sys/select.h>

#define BUF 1024
#define PORT 6543

// sendet eine Zeile ohne NUL-Byte, fügt CRLF hinzu
static int send_line(int sock, const char* s) {
    size_t len = strlen(s);
    ssize_t n = send(sock, s, len, 0);
    if (n < 0) return -1;
    n = send(sock, "\r\n", 2, 0);
    if (n < 0) return -1;
    return 0;
}

// liest bis '\n', trimmt optionales '\r', nullterminiert
static int recv_line(int sock, char* buf, size_t cap) {
    size_t used = 0;
    while (used + 1 < cap) {
        char c;
        ssize_t n = recv(sock, &c, 1, 0);
        if (n == 0) { // server hat geschlossen
            if (used == 0) return 0; // EOF
            break;
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (c == '\n') break;
        buf[used++] = c;
    }
    if (used > 0 && buf[used - 1] == '\r') used--;
    buf[used] = '\0';
    return (int)used;
}

// trimmt \r\n am Ende der lokalen Eingabe
static void trim_stdin_line(char* s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) s[--len] = '\0';
    // rechts trimmen: Space/Tab
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t')) s[--len] = '\0';
    // links trimmen
    size_t i = 0;
    while (s[i] == ' ' || s[i] == '\t') i++;
    if (i > 0) memmove(s, s + i, len - i + 1);
}

static int equals_ignore_case(const char* a, const char* b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}


int main(int argc, char **argv)
{
    int create_socket = -1;
    struct sockaddr_in address;
    char buffer[BUF];

    // Socket
    if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    // Address
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    if (argc < 2) inet_aton("127.0.0.1", &address.sin_addr);
    else          inet_aton(argv[1], &address.sin_addr);

    // Connect
    if (connect(create_socket, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("connect");
        close(create_socket);
        return EXIT_FAILURE;
    }
    printf("Connected to %s:%d\n", inet_ntoa(address.sin_addr), PORT);

    // Welcome (2 Zeilen vom Server sind üblich; wir lesen, solange schnell was kommt)
    // Mindestens eine Zeile lesen:
    int r = recv_line(create_socket, buffer, BUF);
    if (r > 0) printf("%s\n", buffer);
    // Versuche noch eine zweite Zeile zu lesen (optional):
    // Nicht blockierend: nur wenn schon was da ist. Wir lassen es simpel und lesen noch einmal,
    // falls der Server bereits beide Zeilen geschickt hat.
    struct timeval tv = {0, 100000}; // 100 ms
    fd_set rfds; FD_ZERO(&rfds); FD_SET(create_socket, &rfds);
    if (select(create_socket + 1, &rfds, NULL, NULL, &tv) > 0) {
        r = recv_line(create_socket, buffer, BUF);
        if (r > 0) printf("%s\n", buffer);
    }

    int isQuit = 0;

    while (!isQuit) {
        printf(">> ");
        fflush(stdout);
        if (!fgets(buffer, BUF, stdin)) break;
        trim_stdin_line(buffer);

        // leere Eingabe ignorieren
        if (buffer[0] == '\0') continue;

        // QUIT erkennung vor dem Senden (wir senden ihn aber trotzdem korrekt als Kommando)
        isQuit = equals_ignore_case(buffer, "quit");

        // 1) Kommando als Zeile senden (ohne NUL, mit CRLF)
        if (send_line(create_socket, buffer) == -1) {
            perror("send");
            break;
        }

        // 2) Wenn SEND: Header + Body nachschicken
        if (strcmp(buffer, "SEND") == 0) {
            // sender
            printf("sender: "); fflush(stdout);
            if (!fgets(buffer, BUF, stdin)) break;
            trim_stdin_line(buffer);
            if (send_line(create_socket, buffer) == -1) { perror("send"); break; }

            // receiver
            printf("receiver: "); fflush(stdout);
            if (!fgets(buffer, BUF, stdin)) break;
            trim_stdin_line(buffer);
            if (send_line(create_socket, buffer) == -1) { perror("send"); break; }

            // subject
            printf("subject: "); fflush(stdout);
            if (!fgets(buffer, BUF, stdin)) break;
            trim_stdin_line(buffer);
            if (send_line(create_socket, buffer) == -1) { perror("send"); break; }

            // body
            printf("message ('.' alleine beendet):\n");
            for (;;) {
                if (!fgets(buffer, BUF, stdin)) { perror("stdin"); break; }
                trim_stdin_line(buffer);
                if (strcmp(buffer, ".") == 0) {
                    if (send_line(create_socket, ".") == -1) { perror("send"); }
                    break;
                }
                if (send_line(create_socket, buffer) == -1) { perror("send"); break; }
            }
        }

        // 3) Server-Antwort (eine Zeile) lesen
        r = recv_line(create_socket, buffer, BUF);
        if (r <= 0) {
            if (r < 0) perror("recv");
            else fprintf(stderr, "Server closed connection\n");
            break;
        }
        printf("<< %s\n", buffer);

        // Für einfache Kommandos erwarten wir "OK" oder "ERR"
        if (!equals_ignore_case(buffer, "OK")) {
            fprintf(stderr, "<< Server error occurred, abort\n");
            break;
        }
    }

    // Close
    if (create_socket != -1) {
        shutdown(create_socket, SHUT_RDWR);
        close(create_socket);
        create_socket = -1;
    }
    return EXIT_SUCCESS;
}
