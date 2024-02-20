#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "socketFunctions.h"

void *receive_messages(void *arg)
{
    char buffer[BUFFERSIZE];
    char name[MAX_NAME_LENGTH];
    int socket = *(int *)arg;
    while (1)
    {
        ssize_t len;
        ssize_t bytes_received = recv(socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
        {
            printf("Server disconnected\n");
            close(socket);
            exit(EXIT_SUCCESS);
        }
        else
        {
            buffer[bytes_received] = 0;
            len = buffer[0];
            ssize_t count = 0;
            memmove(buffer, buffer + 1, strlen(buffer) - 1);
            count += strlen(buffer);
            buffer[strlen(buffer) - 1] = 0;
            char tempBuf[BUFFERSIZE];
            while (len > count)
            {
                bytes_received = recv(socket, tempBuf, sizeof(tempBuf), 0);
                memcpy(buffer + strlen(buffer), tempBuf, strlen(tempBuf));
                count += bytes_received;
            }
            int nameLen = buffer[0];
            memcpy(name, buffer+1, nameLen);
            name[nameLen] = 0;
            memmove(buffer, buffer + nameLen + 1, strlen(buffer) - nameLen - 1);
            buffer[strlen(buffer) - nameLen - 1] = 0;
        }
        printf("{%s} %s\n", name, buffer);
    }
}

void *send_message(void *arg)
{
    char *message = NULL;
    ssize_t len = BUFFERSIZE;
    int socket = *(int *)arg;
    while (1)
    {
        int size = getline(&message, &len, stdin);
        size--;
        if (size > 127)
            size = 127;
        message[size] = 0;
        int space = 0;
        for(int i = 0; i < strlen(message); ++i){
            if (message[i] != ' ') break;
            else space++;
        }
    
        memmove(message + MESSAGE_SIZE, message, size);
        memset(message, size, MESSAGE_SIZE);
        if (strncmp(message + MESSAGE_SIZE, "EXIT", 4) == 0)
            exit(EXIT_SUCCESS);
        send(socket, message, size+1, 0);
    }
}

struct sockaddr_in enterSocketAddress(char *ip, char *port)
{
    struct sockaddr_in sa;
    inet_pton(AF_INET, ip, &(sa.sin_addr));
    sa.sin_port = htons(atoi(port));
    sa.sin_family = AF_INET;
    return sa;
}

int main(int argc, char **argv)
{
    int socket = Socket(AF_INET, SOCK_STREAM, 0);
    if (argc != 4)
    {
        printf("USE: ./client <ip> <port> <name>");
        exit(EXIT_FAILURE);
    }
    if (strlen(argv[3]) >= 16)
    {
        printf("The name is too big");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serverAddress = enterSocketAddress(argv[1], argv[2]);
    Connect(socket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    char *name = argv[3];
    send(socket, name, strlen(name), 0);
    char ans[2];
    recv(socket, ans, sizeof(ans), 0);
    switch (ans[0])
    {
    case NO_NAME:
        printf("This name is already in use\n");
        return EXIT_FAILURE;
    case NO_PLACE:
        printf("There is no space on the server\n");
        return EXIT_FAILURE;
    }

    pthread_t receive_thread, send_thread;

    pthread_create(&receive_thread, NULL, receive_messages, &socket);

    pthread_create(&send_thread, NULL, send_message, &socket);

    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);
    close(socket);
    return EXIT_SUCCESS;
}
