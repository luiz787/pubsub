#ifndef CONSOLE_H
#define CONSOLE_H

#include <mutex>
#include <string.h>
#include <string>
#include <termios.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>

// Adapted from
// https://stackoverflow.com/questions/55414228/simultaneous-input-and-output-in-a-console
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
    int get_char();

    void write(const char *text, size_t size);
    void write(const char *text);
    void write(const std::string &text);
};

#endif // CONSOLE_H
