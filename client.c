#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "socketFunctions.h"

static char name[MAX_NAME_LENGTH];

void readMessage(char *buffer)
{
    int operation = buffer[0];
    int nameLen = buffer[1];
    memcpy(name, buffer + 2, nameLen);
    name[nameLen] = 0;
    memmove(buffer, buffer + nameLen + 2, strlen(buffer) - nameLen - 1);
    switch (operation)
    {
    case BROADCASTMESSAGE:
        printf("{\033[33m%s\033[0m} %s\n", name, buffer);
        break;
    case PRIVATEMESSAGE:
        printf("{\033[33m%s\033[0m -> \033[34mYOU\033[0m} %s\n", name, buffer);
        break;
    case USERLIST:
        printf("\033[33m%s\033[0m\n", buffer);
        break;
    case ERRORMESSAGE:
        printf("\033[31m%s\033[0m\n", buffer);
        break;
    default:
        break;
    }
}

void *receive_messages(void *arg)
{
    char buffer[BUFFERSIZE];
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
            readMessage(buffer);
        }
    }
}

void *send_message(void *arg)
{
    char message[BUFFERSIZE];
    memset(message, 0, BUFFERSIZE);
    ssize_t len = BUFFERSIZE;
    int socket = *(int *)arg;
    while (1)
    {
        fgets(message, BUFFERSIZE, stdin);
        message[strlen(message)-1] = 0;
        if (strlen(message) == 126)
        {
            int c;
            while ((c = getc(stdin)) != '\n' && c != EOF){}
        }
        int spaceStart = 0;
        while (message[spaceStart] == ' ')
        {
            spaceStart++;
        }
        char temp[BUFFERSIZE];
        memset(temp, 0, BUFFERSIZE);
        temp[0] = message[0];
        int iter = 1;
        for(int i = 1; i < BUFFERSIZE; ++i){
            if (message[i] == ' ' && message[i-1] == ' ') continue;
            temp[iter++] = message[i];
        }
        memcpy(message, temp, BUFFERSIZE);
        int size = strlen(message);
        int spaceEnd = size - 1;
        while (message[spaceEnd] == ' ')
        {
            spaceEnd--;
        }
        if (spaceStart >= spaceEnd)
            continue;
        size = spaceEnd - spaceStart + 1;
        memmove(message, message + spaceStart, size);
        memset(message + size, 0, BUFFERSIZE - size);
        memmove(message + MESSAGE_SIZE, message, size);
        memset(message, size, MESSAGE_SIZE);
        if (strncmp(message + MESSAGE_SIZE, "/exit", 5) == 0)
            exit(EXIT_SUCCESS);
        send(socket, message, size + 1, 0);
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
        printf("USE: ./client <ip> <port> <name>\n");
        exit(EXIT_FAILURE);
    }
    if (strlen(argv[3]) >= 16)
    {
        printf("The name is too big\n");
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