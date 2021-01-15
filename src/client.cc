#include "common.h"
#include "console.h"

#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}
struct Globals {
    Console console;
    int socket;

    Globals() {}
};

int onexit(Globals &globals) {
    globals.console.write("[log] connection terminated.");
    close(globals.socket);

    exit(EXIT_SUCCESS);
}

void network_recv(Globals &globals) {
    char buf[BUFSZ];
    while (true) {
        memset(buf, 0, BUFSZ);
        unsigned total = 0;
        size_t count = 0;
        while (true) {
            count = recv(globals.socket, buf + total, BUFSZ - total, 0);

            if (count == 0) {
                onexit(globals);
            }

            total += count;
            if (!validate_message(buf)) {
                globals.console.write(
                    "[log] invalid message received, exiting");
                onexit(globals);
            }

            // as \n denotes end of message, we check for that.
            char lastchar = buf[total - 1];
            if (lastchar == '\n') {
                break;
            }
        }

        // Uses aux variable to use operator "=" overload instead of manual
        // copying
        std::string aux = buf;
        globals.console.write(aux);
    }
}

void process_input(const std::string &input, Globals &globals) {
    char buffer[BUFSZ];
    memset(buffer, 0, BUFSZ);

    for (size_t i = 0; i < input.size(); i++) {
        buffer[i] = input[i];
    }

    // Appends \n to the final of the buffer to signal message end
    buffer[strlen(buffer)] = '\n';

    size_t count = send(globals.socket, buffer, strlen(buffer), 0);
    if (count != strlen(buffer)) {
        logexit("send");
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage))) {
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    Globals globals;
    globals.socket = s;
    printf("[log] connected to %s\n", addrstr);

    // Creates dedicated thread to receive messages from the network.
    std::thread recv_thread(&network_recv, std::ref(globals));

    while (true) {
        std::string input = globals.console.read();
        process_input(input, globals);
    }

    recv_thread.join();
    return onexit(globals);
}
