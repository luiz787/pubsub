#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

typedef int Client;
typedef string Tag;

#define BUFSZ 500

class MessageBroker {
private:
    map<Client, set<Tag>> controlMap;

public:
    MessageBroker() { this->controlMap = map<Client, set<Tag>>(); }

    void delete_client(Client client) { controlMap.erase(client); }

    string delete_all_clients() {
        for (auto kv : controlMap) {
            close(kv.first);
        }

        controlMap.clear();

        return "ok";
    }

    Tag getTagFromSubscribeMessage(string msg) { return msg.substr(1); }

    Tag getTagFromUnsubscribeMessage(string msg) { return msg.substr(1); }

    bool validate_sub_unsub_message(string msg) {
        auto msg_without_first = msg.substr(1);
        for (auto ch : msg_without_first) {
            if (!isalpha(ch)) {
                return false;
            }
        }
        return true;
    }

    string subscribe(Client client, string msg) {
        if (!validate_sub_unsub_message(msg)) {
            throw invalid_argument("invalid tag");
        }
        Tag subject = getTagFromSubscribeMessage(msg);
        if (isSubscribed(client, subject)) {
            auto message = "already subscribed +" + subject;
            throw invalid_argument(message);
        }
        controlMap[client].emplace(subject);
        return "subscribed +" + subject;
    }

    bool isSubscribed(Client client, Tag subject) {
        auto element = controlMap[client].find(subject);
        return element != controlMap[client].end();
    }

    string unsubscribe(Client client, string msg) {
        if (!validate_sub_unsub_message(msg)) {
            throw invalid_argument("invalid tag");
        }
        Tag subject = getTagFromUnsubscribeMessage(msg);
        if (!isSubscribed(client, subject)) {
            auto message = "not subscribed +" + subject;
            throw invalid_argument(message);
        }

        controlMap[client].erase(subject);
        return "unsubscribed -" + subject;
    }

    string onMessage(string msg) {
        auto tags = getTags(msg);
        publish(msg, tags);
        return "ok";
    }

    void publish(string msg, set<Tag> tags) {
        for (auto client : controlMap) {
            auto tagsOfInterest = client.second;

            // debugPrint(client);

            vector<Tag> intersection;
            set_intersection(tagsOfInterest.begin(), tagsOfInterest.end(),
                             tags.begin(), tags.end(),
                             std::back_inserter(intersection));

            // If the intersection between the set of tags that the client is
            // interested in and the set of tags in the message is not empty,
            // that means the client is interested in at least one of the
            // message's tags.
            if (!intersection.empty()) {
                send_data(client.first, msg);
            }
        }
    }

    void debugPrint(pair<Client, set<Tag>> clientData) {
        cout << "Client " << clientData.first << endl;
        cout << "Tags of interest:" << endl;
        for (auto tag : clientData.second) {
            cout << tag << endl;
        }
    }

    void send_data(int client, string msg) const {
        cout << client << " would receive the following message: \"" << msg
             << "\"" << endl;
        // TODO: send msg to client.
        char buffer[BUFSZ];
        memset(buffer, 0, BUFSZ);
        for (size_t i = 0; i < msg.size(); i++) {
            buffer[i] = msg[i];
        }

        buffer[strlen(buffer)] = '\n';

        cout << "Will send this: " << endl;

        for (size_t i = 0; i < strlen(buffer) + 1; i++) {
            cout << (int)buffer[i] << ", ";
        }
        cout << endl;

        send(client, buffer, strlen(buffer), 0);
    }

    set<Tag> getTags(string msg) const {
        istringstream iss(msg);
        vector<string> tokens{istream_iterator<string>{iss},
                              istream_iterator<string>{}};

        set<Tag> tags = set<Tag>();

        for (auto token : tokens) {
            if (token[0] == '#') {
                Tag tag = getTagFromToken(token);
                if (tag != "") {
                    tags.emplace(tag);
                }
            }
        }
        return tags;
    }

    Tag getTagFromToken(Tag token) const {
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
};

void usage(int argc, char **argv) {
    printf("usage: %s <server port>\n", argv[0]);
    printf("example: %s 1337\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct client_data {
    MessageBroker *broker;
    int csock;
    struct sockaddr_storage storage;
};

enum MessageType { SUBSCRIBE, UNSUBSCRIBE, MESSAGE, KILL };

struct Message {
    MessageType type;
    string content;
};

string getMessageType(MessageType type) {
    switch (type) {
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

Message parseMessage(char *buf) {
    char firstletter = buf[0];
    Message message;
    string aux = buf;

    aux = aux.substr(0, aux.size() - 1);

    printf("[log] @parseMessage, last char of message is: %d\n",
           aux[aux.size() - 1]);

    auto bufzin = aux.c_str();

    cout << "after parse to string: " << endl;
    for (size_t i = 0; i < strlen(bufzin) + 1; i++) {
        cout << (int)bufzin[i] << ", ";
    }
    cout << endl;

    message.content = aux;
    if (aux == "##kill") {
        message.type = KILL;
    } else if (firstletter == '+') {
        message.type = SUBSCRIBE;
    } else if (firstletter == '-') {
        message.type = UNSUBSCRIBE;
    } else {
        message.type = MESSAGE;
    }

    return message;
}

string performAction(int socket, MessageBroker *broker, Message msg) {
    try {
        switch (msg.type) {
        case SUBSCRIBE:
            printf("[log] handling subscribe msg\n");
            cout << msg.content << endl;
            return broker->subscribe(socket, msg.content);
            break;
        case UNSUBSCRIBE:
            return broker->unsubscribe(socket, msg.content);
            break;
        case MESSAGE:
            return broker->onMessage(msg.content);
            break;
        case KILL:
            // FIXME: kill is causing SIGSEGV on clients.
            return broker->delete_all_clients();
        default:
            throw invalid_argument("unknown message type");
            break;
        }
    } catch (const exception &e) {
        printf("[error]: %s\n", e.what());
        return e.what();
    }
}

bool validate_message(Message message) {
    for (auto ch : message.content) {
        if (static_cast<unsigned char>(ch) > 127) {
            return false;
        }
    }
    return true;
}

void *client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);
    auto broker = cdata->broker;

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

    char buf[BUFSZ];

    while (1) {
        memset(buf, 0, BUFSZ);

        // TODO: receive partitioned message.
        size_t count = recv(cdata->csock, buf, BUFSZ, 0);

        cout << "Received this: " << endl;
        for (size_t i = 0; i < strlen(buf) + 1; i++) {
            cout << (int)buf[i] << ", ";
        }
        cout << endl;

        char lastchar = buf[strlen(buf) - 1];
        printf("[log] char at last buf index: %d\n", lastchar);

        if (count == 0) {
            printf("[log] client %s disconnected.\n", caddrstr);
            break;
        }

        printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

        // parse msg
        Message message = parseMessage(buf);
        if (!validate_message(message)) {
            printf("[log] message sent by client %s is invalid, aborting "
                   "connection\n",
                   caddrstr);
            break;
        }
        printf("[log] message type: %s\n",
               getMessageType(message.type).c_str());

        string res = performAction(cdata->csock, broker, message);

        if (message.type == KILL) {
            printf("[log] received kill message, terminating...\n");
            exit(EXIT_SUCCESS);
        }
        sprintf(buf, "%.400s\n", res.c_str());

        // Will send the exact length of buf in order to avoid sending
        // the null terminator over the network.
        count = send(cdata->csock, buf, strlen(buf), 0);
        if (count != strlen(buf)) {
            logexit("send");
        }
    }

    printf("[log] closing connection with %s\n", caddrstr);
    broker->delete_client(cdata->csock);
    close(cdata->csock);

    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    MessageBroker broker = MessageBroker();

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);

        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

        struct client_data *cdata = new client_data;
        if (!cdata) {
            logexit("malloc");
        }
        cdata->csock = csock;
        cdata->broker = &broker;
        memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}
