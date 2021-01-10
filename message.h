#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

enum MessageType { SUBSCRIBE, UNSUBSCRIBE, MESSAGE, KILL };

class Message {
private:
    MessageType type;
    std::string content;

public:
    Message(MessageType type, std::string content);
    static Message from_buffer(char *buffer);
    std::string get_str_type();
    MessageType get_type();
    std::string get_content();
};

#endif // MESSAGE_H
