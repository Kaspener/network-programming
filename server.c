#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "socketFunctions.h"

#define BUFFERSIZE 128

int main()
{
    char buffer[BUFFERSIZE];
    int serverSocket = Socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serverAddress, clientAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    socklen_t length = sizeof(serverAddress);
    Bind(serverSocket, &serverAddress, length);
    GetSockName(serverSocket, &serverAddress, &length);
    printf("Open server:\nAddress = %d\tPort = %d\n", ntohl(serverAddress.sin_addr.s_addr), ntohs(serverAddress.sin_port));
    while(1){
        length = sizeof(clientAddress);
        memset(buffer, 0, BUFFERSIZE);
        ssize_t msglength = recvfrom(serverSocket, buffer, BUFFERSIZE, 0 , (struct sockaddr *) &clientAddress, &length);
        if (msglength == -1 ){
            perror("Error socket client");
            exit(EXIT_FAILURE);
        }
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress.sin_addr), ip, INET_ADDRSTRLEN);
        printf("SERVER: IP адрес клиента: %s\n", ip); 
        printf("SERVER: PORT клиента: %d\n", ntohs(clientAddress.sin_port)) ; 
        printf("SERVER: Длина: %ld\n", msglength);
        printf("SERVER: Сообщение: %s\n\n", buffer);
        char ans[] = "ANSWERD FROM SERVER FOR  ";
        ans[24] = buffer[0]; 
        if (sendto(serverSocket, ans, BUFFERSIZE, 0, (struct sockaddr *) &clientAddress, sizeof(clientAddress)) == -1)
        {
            perror("sandto error");
            exit(1);
        }
    }
    return EXIT_SUCCESS;
}