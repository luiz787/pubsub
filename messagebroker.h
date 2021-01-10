#ifndef MESSAGE_BROKER_H
#define MESSAGE_BROKER_H

#include "common.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

#include <unistd.h>

class MessageBroker {
private:
    std::map<Client, std::set<Tag>> controlMap;

public:
    MessageBroker();

    std::string subscribe(Client client, std::string msg);
    std::string unsubscribe(Client client, std::string msg);
    std::string onMessage(std::string msg);

    void delete_client(Client client);
    std::string delete_all_clients();

    Tag getTagFromSubscribeMessage(std::string msg);
    Tag getTagFromUnsubscribeMessage(std::string msg);
    bool validate_sub_unsub_message(std::string msg);

    bool isSubscribed(Client client, Tag subject);
    void publish(std::string msg, std::set<Tag> tags);
    void send_data(int client, std::string msg) const;

    std::set<Tag> getTags(std::string msg) const;
    Tag getTagFromToken(Tag token) const;
};

#endif
