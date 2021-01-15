#include "common.h"

#include <chrono>
#include <iomanip>
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

#define BUFSZ 500

int get_char() {
    struct termios oldattr;
    tcgetattr(STDIN_FILENO, &oldattr);
    struct termios newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    const int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch;
}

class Console {
private:
    std::mutex _mtx;
    std::string _input;
    std::string _prompt;

public:
    Console() {}

    Console(const Console &) = delete;
    Console &operator=(const Console &) = delete;

    std::string read();

    void write(const char *text, size_t size);
    void write(const char *text) { write(text, strlen(text)); }
    void write(const std::string &text) { write(text.c_str(), text.size()); }
};

std::string Console::read() {
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _prompt = "> ";
        _input.clear();
        std::cout << _prompt << std::flush;
    }
    enum { Enter = '\n', BackSpc = 127 };
    for (;;) {
        switch (int c = get_char()) {
        case Enter: {
            std::lock_guard<std::mutex> lock(_mtx);
            std::string input = _input;
            _prompt.clear();
            _input.clear();
            std::cout << std::endl;
            return input;
        }
        case BackSpc: {
            std::lock_guard<std::mutex> lock(_mtx);
            if (_input.empty()) {
                break;
            }
            _input.pop_back();
            std::cout << "\b \b" << std::flush;
        } break;
        default: {
            if (c < ' ' || c >= '\x7f') {
                break;
            }
            std::lock_guard<std::mutex> lock(_mtx);
            _input += c;
            std::cout << (char)c << std::flush;
        } break;
        }
    }
}

void Console::write(const char *text, size_t len) {
    if (!len) {
        return;
    }
    bool eol = text[len - 1] == '\n';
    std::lock_guard<std::mutex> lock(_mtx);
    if (size_t size = _prompt.size() + _input.size()) {
        std::cout << std::setfill('\b') << std::setw(size) << ""
                  << std::setfill(' ') << std::setw(size) << ""
                  << std::setfill('\b') << std::setw(size) << "";
    }
    std::cout << text;
    if (!eol) {
        std::cout << std::endl;
    }
    std::cout << _prompt << _input << std::flush;
}

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

            std::string logMessage2;
            logMessage2.append("[log] Amount of bytes recived: ");
            logMessage2.append(std::to_string(count));
            globals.console.write(logMessage2);

            if (count == 0) {
                onexit(globals);
            }

            total += count;
            if (!validate_message(buf)) {
                globals.console.write("invalid message received, exiting");
                // Invalid message, exiting
                onexit(globals);
            }

            char lastchar = buf[total - 1];
            std::string logMessage1;
            logMessage1.append("[log] Last char at the buffer: ");
            logMessage1.push_back(lastchar);
            globals.console.write(logMessage1);

            // as \n denotes end of message, we check for that.
            if (lastchar == '\n') {
                globals.console.write("[log] end of msg.");
                break;
            }
        }

        std::string logMessage;
        logMessage.append("received ");
        logMessage.append(std::to_string(total));
        logMessage.append(" bytes\n");

        globals.console.write(logMessage);

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
    printf("connected to %s\n", addrstr);

    // Creates dedicated thread to receive messages from the network.
    std::thread recv_thread(&network_recv, std::ref(globals));

    while (true) {
        std::string input = globals.console.read();
        process_input(input, globals);
    }

    recv_thread.join();
    return onexit(globals);
}
