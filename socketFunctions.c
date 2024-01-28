#include "socketFunctions.h"
#include <stdlib.h>
#include <stdio.h>

int Socket(int domain, int type, int protocol)
{
    int serverSocket = socket(domain, type, protocol);
    if (serverSocket == -1)
    {
        perror("The server socket cannot be opened");
        exit(EXIT_FAILURE);
    }
    return serverSocket;
}

void Bind(int sockfd, const struct sockaddr_in *addr, socklen_t addrlen){
    int res = bind(sockfd, (struct sockaddr *)addr, addrlen);
    if (res == -1){
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
}

void GetSockName(int sockfd, struct sockaddr_in *restrict addr, socklen_t *restrict addrlen){
    if (getsockname(sockfd, (struct sockaddr *restrict)addr, addrlen)){
        perror("The getsockname call failed");
        exit(EXIT_FAILURE);
    }
}