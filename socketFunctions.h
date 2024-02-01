#ifndef SOCKET_FUNCTIONS_H
#define SOCKET_FUNCTIONS_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFERSIZE 32
#define LISTEN_QUEUE 10
#define MAX_CLIENTS 10

int Socket(int domain, int type, int protocol);

void Bind(int sockfd, const struct sockaddr_in *addr, socklen_t addrlen);

void GetSockName(int sockfd, struct sockaddr_in *restrict addr, socklen_t *restrict addrlen);

void Listen(int sockfd, int backlog);

void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

#endif // SOCKET_FUNCTIONS_H