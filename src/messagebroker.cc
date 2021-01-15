#include "messagebroker.h"

MessageBroker::MessageBroker() {
    this->controlMap = std::map<Client, std::set<Tag>>();
}

void MessageBroker::delete_client(Client client) {
    this->controlMap.erase(client);
}

std::string MessageBroker::delete_all_clients() {
    for (auto kv : this->controlMap) {
        close(kv.first);
    }

    this->controlMap.clear();

    return "ok";
}

Tag MessageBroker::getTagFromSubscribeMessage(std::string msg) {
    return msg.substr(1);
}

Tag MessageBroker::getTagFromUnsubscribeMessage(std::string msg) {
    return msg.substr(1);
}

bool MessageBroker::validate_sub_unsub_message(std::string msg) {
    auto msg_without_first = msg.substr(1);
    for (auto ch : msg_without_first) {
        if (!isalpha(ch)) {
            return false;
        }
    }
    return true;
}

std::string MessageBroker::subscribe(Client client, std::string msg) {
    if (!this->validate_sub_unsub_message(msg)) {
        throw std::invalid_argument("invalid tag");
    }
    Tag subject = this->getTagFromSubscribeMessage(msg);
    if (this->isSubscribed(client, subject)) {
        auto message = "already subscribed +" + subject;
        throw std::invalid_argument(message);
    }
    this->controlMap[client].emplace(subject);
    return "subscribed +" + subject;
}

bool MessageBroker::isSubscribed(Client client, Tag subject) {
    auto element = this->controlMap[client].find(subject);
    return element != this->controlMap[client].end();
}

std::string MessageBroker::unsubscribe(Client client, std::string msg) {
    if (!this->validate_sub_unsub_message(msg)) {
        throw std::invalid_argument("invalid tag");
    }
    Tag subject = this->getTagFromUnsubscribeMessage(msg);
    if (!this->isSubscribed(client, subject)) {
        auto message = "not subscribed -" + subject;
        throw std::invalid_argument(message);
    }

    this->controlMap[client].erase(subject);
    return "unsubscribed -" + subject;
}

std::string MessageBroker::onMessage(std::string msg) {
    auto tags = this->getTags(msg);
    this->publish(msg, tags);
    return "";
}

void MessageBroker::publish(std::string msg, std::set<Tag> tags) {
    for (auto tag : tags) {
        printf("%s, ", tag.c_str());
    }
    printf("\n");

    for (auto client : this->controlMap) {
        auto tagsOfInterest = client.second;

        // debugPrint(client);

        std::vector<Tag> intersection;
        set_intersection(tagsOfInterest.begin(), tagsOfInterest.end(),
                         tags.begin(), tags.end(),
                         std::back_inserter(intersection));

        // If the intersection between the set of tags that the client is
        // interested in and the set of tags in the message is not empty,
        // that means the client is interested in at least one of the
        // message's tags.
        if (!intersection.empty()) {
            this->send_data(client.first, msg);
        }
    }
}

void MessageBroker::send_data(int client, std::string msg) const {
    std::cout << client << " would receive the following message: \"" << msg
              << "\"" << std::endl;
    // TODO: send msg to client.
    char buffer[BUFSZ];
    memset(buffer, 0, BUFSZ);
    for (size_t i = 0; i < msg.size(); i++) {
        buffer[i] = msg[i];
    }

    buffer[strlen(buffer)] = '\n';

    for (size_t i = 0; i < strlen(buffer) + 1; i++) {
        std::cout << (int)buffer[i] << ", ";
    }
    std::cout << std::endl;

    send(client, buffer, strlen(buffer), 0);
}

std::set<Tag> MessageBroker::getTags(std::string msg) const {
    std::istringstream iss(msg);
    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                    std::istream_iterator<std::string>{}};

    std::set<Tag> tags = std::set<Tag>();

    for (auto token : tokens) {
        if (token[0] == '#') {
            Tag tag = this->getTagFromToken(token);
            if (tag != "") {
                tags.emplace(tag);
            }
        }
    }
    return tags;
}

Tag MessageBroker::getTagFromToken(Tag token) const {
    // A tag has to have a single #.
    Tag tag = "";

    uint8_t amount_of_hashtags = 0;

    for (size_t i = 0; i < token.size(); i++) {
        auto c = token[i];

        if (c == '#') {
            amount_of_hashtags++;
        }

        if (amount_of_hashtags == 2) {
            // not valid.
            return "";
        }

        if (isalpha((int)c)) {
            tag += c;
        }
    }
    return tag;
}