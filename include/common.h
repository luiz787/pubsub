#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <string>
#include <sys/socket.h>

#define BUFSZ 500

typedef int Client;
typedef std::string Tag;

void logexit(const char *msg);
int server_sockaddr_init(const char *portstr, struct sockaddr_storage *storage);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);
bool validate_message(char *buffer);

#endif // COMMON_H
