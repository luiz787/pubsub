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

void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFSZ 500

bool validate_message(char *msg, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (static_cast<unsigned char>(msg[i]) > 127) {
            return false;
        }
    }
    return true;
}

int onexit(int s) {
    printf("[log] connection terminated.\n");
    close(s);

    return EXIT_SUCCESS;
}

int getChar() {
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
    // mutex for console I/O
    std::mutex _mtx;
    // current input
    std::string _input;
    // prompt output
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
    { // activate prompt
        std::lock_guard<std::mutex> lock(_mtx);
        _prompt = "> ";
        _input.clear();
        std::cout << _prompt << std::flush;
    }
    enum { Enter = '\n', BackSpc = 127 };
    for (;;) {
        switch (int c = getChar()) {
        case Enter: {
            std::lock_guard<std::mutex> lock(_mtx);
            std::string input = _input;
            _prompt.clear();
            _input.clear();
            std::cout << std::endl;
            return input;
        } // unreachable: break;
        case BackSpc: {
            std::lock_guard<std::mutex> lock(_mtx);
            if (_input.empty())
                break;
            _input.pop_back();
            std::cout << "\b \b" << std::flush;
        } break;
        default: {
            if (c < ' ' || c >= '\x7f')
                break;
            std::lock_guard<std::mutex> lock(_mtx);
            _input += c;
            std::cout << (char)c << std::flush;
        } break;
        }
    }
}

void Console::write(const char *text, size_t len) {
    if (!len)
        return;
    bool eol = text[len - 1] == '\n';
    std::lock_guard<std::mutex> lock(_mtx);
    // remove current input echo
    if (size_t size = _prompt.size() + _input.size()) {
        std::cout << std::setfill('\b') << std::setw(size) << ""
                  << std::setfill(' ') << std::setw(size) << ""
                  << std::setfill('\b') << std::setw(size) << "";
    }
    // print text
    std::cout << text;
    if (!eol)
        std::cout << std::endl;
    // print current input echo
    std::cout << _prompt << _input << std::flush;
}

struct Flags // this flag is shared between both the threads
{
    // the mini console
    Console console;
    int s;

    // constructor.
    Flags() {}
};

void network_recv(Flags &shared) {
    char buf[BUFSZ];
    while (1) {
        memset(buf, 0, BUFSZ);
        unsigned total = 0;
        size_t count = 0;
        while (1) {
            count = recv(shared.s, buf + total, BUFSZ - total, 0);

            std::string logMessage2;
            logMessage2.append("[log] count = ");
            logMessage2.append(std::to_string(count));
            shared.console.write(logMessage2);
            // printf("[log] count = %lu\n", count);
            if (count == 0) {
                // Connection terminated
                onexit(shared.s);
            }

            total += count;
            if (!validate_message(buf, total)) {
                shared.console.write("invalid message received");
                // Invalid message, exiting
                onexit(shared.s);
            }

            char lastchar = buf[total - 1];
            std::string logMessage1;
            logMessage1.append("[log] char at last buf index: ");
            logMessage1.push_back(lastchar);
            shared.console.write(logMessage1);
            // as \n denotes end of message, we check for that.
            if (lastchar == '\n') {
                shared.console.write("[log] end of msg.");
                break;
            }
        }

        std::string logMessage;
        logMessage.append("received ");
        logMessage.append(std::to_string(total));
        logMessage.append(" bytes\n");

        shared.console.write(logMessage);

        std::string aux = buf;
        shared.console.write(aux);
    }
}

void processInput(const std::string &input, Flags &shared) {
    // shared.console.write(input);
    // TODO: dispatch send to another thread.
    char buffer[BUFSZ];
    memset(buffer, 0, BUFSZ);

    for (size_t i = 0; i < input.size(); i++) {
        buffer[i] = input[i];
    }

    buffer[strlen(buffer)] = '\n';

    /*for (size_t i = 0; i < strlen(buffer); i++) {
        std::string aux = "";
        aux.append(std::to_string(int(buffer[i])));
        aux.append(", ");
        shared.console.write(aux);
    }*/

    size_t count = send(shared.s, buffer, strlen(buffer), 0);
    if (count != strlen(buffer)) {
        logexit("send");
    }

    memset(buffer, 0, BUFSZ);
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

    Flags shared;
    shared.s = s;
    printf("connected to %s\n", addrstr);
    std::thread threadProc(&network_recv, std::ref(shared));

    while (1) {
        std::string input = shared.console.read();
        processInput(input, shared);
    }
    threadProc.join();
    return onexit(s);
}
