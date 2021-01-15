#include "message.h"

Message::Message(MessageType type, std::string content)
    : type(type), content(content) {}

std::vector<Message> Message::from_buffer(char *buffer) {
    std::vector<std::string> msgs;
    int start = 0;
    for (size_t i = 0; i < strlen(buffer); i++) {
        if (buffer[i] == '\n') {
            std::string msg;
            for (size_t j = start; j <= i; j++) {
                msg += buffer[j];
            }
            msgs.push_back(msg);
            start = i + 1;
        }
    }

    printf("[log] found %lu messages in buffer.\n", msgs.size());

    std::vector<Message> messages;

    for (auto msg : msgs) {
        char firstletter = msg[0];
        std::string aux = msg;
        aux = aux.substr(0, aux.size() - 1);

        MessageType type;
        if (aux == "##kill") {
            type = KILL;
        } else if (firstletter == '+') {
            type = SUBSCRIBE;
        } else if (firstletter == '-') {
            type = UNSUBSCRIBE;
        } else {
            type = MESSAGE;
        }
        messages.push_back(Message(type, aux));
    }

    return messages;
}

std::string Message::get_str_type() {
    switch (this->type) {
    case SUBSCRIBE:
        return "SUBSCRIBE";
    case UNSUBSCRIBE:
        return "UNSUBSCRIBE";
    case MESSAGE:
        return "MESSAGE";
    case KILL:
        return "KILL";
    default:
        return "unknown";
    }
}

MessageType Message::get_type() { return this->type; }

std::string Message::get_content() { return this->content; }
