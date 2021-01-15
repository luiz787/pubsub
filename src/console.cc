#include "console.h"

// Adapted from
// https://stackoverflow.com/questions/55414228/simultaneous-input-and-output-in-a-console

int Console::get_char() {
    struct termios oldattr;
    tcgetattr(STDIN_FILENO, &oldattr);
    struct termios newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    const int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch;
}

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

void Console::write(const char *text) { this->write(text, strlen(text)); }

void Console::write(const std::string &text) {
    this->write(text.c_str(), text.size());
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
