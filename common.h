#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <sys/socket.h>

void logexit(const char *msg);
int server_sockaddr_init(const char *portstr, struct sockaddr_storage *storage);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

#endif // COMMON_H
