#include "common.h"
#include "message.h"
#include "messagebroker.h"

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

string performAction(int socket, MessageBroker *broker, Message msg) {
    try {
        switch (msg.get_type()) {
        case SUBSCRIBE:
            printf("[log] handling subscribe msg\n");
            cout << msg.get_content() << endl;
            return broker->subscribe(socket, msg.get_content());
            break;
        case UNSUBSCRIBE:
            return broker->unsubscribe(socket, msg.get_content());
            break;
        case MESSAGE:
            return broker->onMessage(msg.get_content());
            break;
        case KILL:
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
    for (auto ch : message.get_content()) {
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

        unsigned total = 0;
        size_t count = 0;
        bool found_terminator = false;
        while (1) {
            count = recv(cdata->csock, buf + total, BUFSZ - total, 0);

            cout << "Received this: " << endl;
            for (size_t i = 0; i < strlen(buf); i++) {
                if (buf[i] == '\n') {
                    found_terminator = true;
                }
                cout << (int)buf[i] << ", ";
            }
            cout << endl;
            char lastchar = buf[strlen(buf) - 1];
            printf("[log] char at last buf index: %d\n", lastchar);
            if (count == 0) {
                printf("[log] client %s disconnected.\n", caddrstr);
                break;
            }

            total += count;
            if (lastchar == '\n') {
                // message received fully, continue.
                break;
            }
            printf("[msg] part: %s, %d bytes: %s\n", caddrstr, (int)count, buf);
            if (total == BUFSZ && !found_terminator) {
                break;
            }
            if (total > BUFSZ) {
                logexit("dos");
            }
        }

        if (total == BUFSZ && !found_terminator) {
            printf("[log] client sent max amount of bytes but no message "
                   "terminator, aborting connection.\n");
            break;
        }
        if (count == 0) {
            printf("[log] client %s disconnected.\n", caddrstr);
            break;
        }
        printf("[msg] full: %s, %d bytes: %s\n", caddrstr, (int)count, buf);

        // parse msg
        auto message = Message::from_buffer(buf);
        if (!validate_message(message)) {
            printf("[log] message sent by client %s is invalid, aborting "
                   "connection\n",
                   caddrstr);
            break;
        }

        printf("[log] message type: %s\n", message.get_str_type().c_str());

        string res = performAction(cdata->csock, broker, message);

        if (message.get_type() == KILL) {
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
