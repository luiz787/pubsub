#ifndef MESSAGE_H
#define MESSAGE_H

#include <string.h>
#include <string>
#include <vector>

enum MessageType { SUBSCRIBE, UNSUBSCRIBE, MESSAGE, KILL };

class Message {
private:
    MessageType type;
    std::string content;

public:
    Message(MessageType type, std::string content);
    static std::vector<Message> from_buffer(char *buffer);
    std::string get_str_type();
    MessageType get_type();
    std::string get_content();
};

#endif // MESSAGE_H
