#include "message.h"

Message::Message(MessageType type, std::string content)
    : type(type), content(content) {}

Message Message::from_buffer(char *buffer) {
    char firstletter = buffer[0];
    std::string aux = buffer;

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

    return Message(type, aux);
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
