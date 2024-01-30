#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "socketFunctions.h"

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
    int socket = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress = enterSocketAddress();
    int delay;
    printf("Enter delay [1-9]: ");
    scanf("%d", &delay);
    if (delay < 1 || delay > 9){
        perror("Error delay");
        exit(EXIT_FAILURE);
    }
    Connect(socket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    while (1)
    {
        sprintf(buf, "%d", delay);
        buf[1] = '\0';
        if (send(socket, buf, strlen(buf), 0) == -1)
        {
            perror("sandto error");
            exit(EXIT_FAILURE);
        }
        int length = sizeof(serverAddress);
        ssize_t msglength = recv(socket, buf, 26, 0);
        if (msglength == -1 ){
            perror("Error socket client");
            exit(EXIT_FAILURE);
        }
        if (msglength == 0){
            printf("End of server!\n");
            exit(EXIT_SUCCESS);
        }
        printf("%s\n", buf);
        sleep(delay);
    }
    return EXIT_SUCCESS;
}
