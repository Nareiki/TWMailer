#include "server/server.h"

int main() {
    Server server(6543);
    server.run();
    return 0;
}
