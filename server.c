#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ncurses.h>

#include "socketFunctions.h"

struct ThreadData *clients[MAX_CLIENTS];
int countOfClients = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct ThreadData
{
    int client_socket;
    struct sockaddr_in client_address;
    char buffer[BUFFERSIZE];
    char name[MAX_NAME_LENGTH];
};

int checkName(char *name)
{
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < countOfClients; ++i)
    {
        if (strcmp(clients[i]->name, name) == 0)
        {
            pthread_mutex_unlock(&mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&mutex);
    return 1;
}

void removeClientSocket(int socket)
{
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < countOfClients; ++i)
    {
        if (clients[i]->client_socket == socket)
        {
            for (int j = i + 1; j < countOfClients; ++j)
            {
                clients[j - 1] = clients[j];
            }
            countOfClients--;
            clients[countOfClients] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
}

void broadcastMessage(char *message, int sender_socket)
{
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < countOfClients; ++i)
    {
        if (clients[i]->client_socket != sender_socket)
        {
            send(clients[i]->client_socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

void sendMessageToClients(struct ThreadData *data)
{
    if (data->buffer[0] != '/')
    {
        int nameLen = strlen(data->name);
        int len = strlen(data->buffer);
        memmove(data->buffer + MESSAGE_SIZE + nameLen + 1, data->buffer, len+1);
        memset(data->buffer, len, MESSAGE_SIZE);
        memset(data->buffer + 1, nameLen, 1);
        memcpy(data->buffer + 2, data->name, nameLen);
        broadcastMessage(data->buffer, data->client_socket);
    }
}

void clientConnected(struct ThreadData *data)
{
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(data->client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(data->client_address.sin_port);
    pthread_mutex_lock(&mutex);
    printf("\033[33m[LOG]\033[0m Client \033[33m%s\033[0m connected: %s:%d\n", data->name, client_ip, client_port);
    pthread_mutex_unlock(&mutex);
    sprintf(data->buffer, "User %s connected", data->name);
    int len = strlen(data->buffer);
    memmove(data->buffer + MESSAGE_SIZE, data->buffer, len + 1);
    memset(data->buffer, len, MESSAGE_SIZE);
    broadcastMessage(data->buffer, data->client_socket);
}

void clientDisconnected(struct ThreadData *data)
{
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(data->client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(data->client_address.sin_port);
    pthread_mutex_lock(&mutex);
    printf("\033[33m[LOG]\033[0m Client \033[33m%s\033[0m disconnected: %s:%d\n", data->name, client_ip, client_port);
    pthread_mutex_unlock(&mutex);
    sprintf(data->buffer, "User %s disconnected", data->name);
    int len = strlen(data->buffer);
    memmove(data->buffer + MESSAGE_SIZE, data->buffer, len + 1);
    memset(data->buffer, len, MESSAGE_SIZE);
    broadcastMessage(data->buffer, data->client_socket);
    removeClientSocket(data->client_socket);
    close(data->client_socket);
}

void *clientThread(void *data)
{
    struct ThreadData *thread_data = (struct ThreadData *)data;
    int client_socket = thread_data->client_socket;
    char *buffer = thread_data->buffer;
    char *name = thread_data->name;
    struct sockaddr_in client_address = thread_data->client_address;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_address.sin_port);

    clientConnected(thread_data);
    while (1)
    {
        ssize_t len;
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
        {
            clientDisconnected(thread_data);
            free(thread_data);
            pthread_exit(EXIT_SUCCESS);
        }
        else
        {
            buffer[bytes_received] = 0;
            len = buffer[0];
            ssize_t count = 0;
            count += strlen(buffer);
            memmove(buffer, buffer + 1, strlen(buffer) - 1);
            buffer[strlen(buffer) - 1] = 0;
            char tempBuf[BUFFERSIZE];
            while (len > count)
            {
                bytes_received = recv(client_socket, tempBuf, sizeof(tempBuf), 0);
                memcpy(buffer + strlen(buffer), tempBuf, strlen(tempBuf));
                count += bytes_received;
            }
        }
        buffer[len] = 0;
        int cat = strlen(name) + 3;
        memset(buffer+BUFFERSIZE-cat, 0, cat);
        pthread_mutex_lock(&mutex);
        printf("\033[33m[LOG]\033[0m {\033[33m%s\033[0m} {%s:%d}: %s\n", name, client_ip, client_port, buffer);
        pthread_mutex_unlock(&mutex);
        sendMessageToClients(thread_data);
    }
}

int main()
{
    struct ThreadData *threadData;

    pthread_t thread_id;
    int clientSocket;
    int serverSocket = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress, clientAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    socklen_t length = sizeof(serverAddress);
    Bind(serverSocket, &serverAddress, length);
    GetSockName(serverSocket, &serverAddress, &length);
    Listen(serverSocket, LISTEN_QUEUE);

    printf("Open server:\nAddress = %d\tPort = %d\n", ntohl(serverAddress.sin_addr.s_addr), ntohs(serverAddress.sin_port));
    while (1)
    {
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &length);
        if (clientSocket == -1)
        {
            perror("Accept failed");
            continue;
        }

        threadData = (struct ThreadData *)malloc(sizeof(struct ThreadData));
        threadData->client_address = clientAddress;
        threadData->client_socket = clientSocket;
        memset(threadData->buffer, 0, BUFFERSIZE);
        memset(threadData->name, 0, MAX_NAME_LENGTH);
        recv(clientSocket, threadData->name, sizeof(threadData->name), 0);

        if (countOfClients < MAX_CLIENTS)
        {
            if (!checkName(threadData->name))
            {
                memset(threadData->buffer, NO_NAME, 1);
                threadData->buffer[1] = 0;
                send(clientSocket, threadData->buffer, strlen(threadData->buffer), 0);
                free(threadData);
                continue;
            }
            else
            {
                memset(threadData->buffer, OKEY, 1);
                threadData->buffer[1] = 0;
                send(clientSocket, threadData->buffer, strlen(threadData->buffer), 0);
                clients[countOfClients++] = threadData;
            }
        }
        else
        {
            memset(threadData->buffer, NO_PLACE, 1);
            threadData->buffer[1] = 0;
            send(clientSocket, threadData->buffer, strlen(threadData->buffer), 0);
            free(threadData);
            continue;
        }

        if (pthread_create(&thread_id, NULL, clientThread, threadData))
        {
            perror("Thread creation failed");
            close(clientSocket);
            free(threadData);
        }
        else
        {
            pthread_detach(thread_id);
        }
    }
    close(serverSocket);
    return EXIT_SUCCESS;
}