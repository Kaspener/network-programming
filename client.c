#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "socketFunctions.h"

#define BUFFERSIZE 128

struct sockaddr_in enterSocketAddress()
{
    struct sockaddr_in sa;
    char ip[INET_ADDRSTRLEN];
    uint16_t port;
    printf("Enter IP: ");
    scanf("%s", ip);
    inet_pton(AF_INET, ip, &(sa.sin_addr));
    printf("Enter PORT: ");
    scanf("%hd", &port);
    sa.sin_port = htons(port);
    sa.sin_family = AF_INET;
    return sa;
}

int main()
{
    char buf[BUFFERSIZE];
    int socket = Socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serverAddress = enterSocketAddress();
    int delay;
    printf("Enter delay [1-9]: ");
    scanf("%d", &delay);
    if (delay < 1 || delay > 9){
        perror("Error delay");
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        memset(buf, 0, BUFFERSIZE);
        sprintf(buf, "%d", delay);
        if (sendto(socket, buf, BUFFERSIZE, 0, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1)
        {
            perror("sandto error");
            exit(1);
        }
        int length = sizeof(serverAddress);
        memset(buf, 0, BUFFERSIZE);
        int msglength;
        if (msglength = recvfrom(socket, buf, BUFFERSIZE, 0 , (struct sockaddr *) &serverAddress, &length) == -1 ){
            perror("Error socket client");
            exit(EXIT_FAILURE);
        }
        printf("%s\n", buf);
        sleep(delay);
    }
    return EXIT_SUCCESS;
}
