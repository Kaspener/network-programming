#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

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

int privateMessage(char *message, char *name)
{
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < countOfClients; ++i)
    {
        if (strlen(clients[i]->name) == strlen(name))
        {
            if (strcmp(clients[i]->name, name) == 0)
            {
                send(clients[i]->client_socket, message, strlen(message), 0);
                pthread_mutex_unlock(&mutex);
                return 1;
            }
        }
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}

void userList(struct ThreadData *data)
{
    memset(data->buffer, 0, BUFFERSIZE);
    if (countOfClients == 1 && data->client_socket != 0 || countOfClients == 0 && data->client_socket == 0)
        sprintf(data->buffer, "0000No users");
    else
        sprintf(data->buffer, "0000Users:");
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < countOfClients; ++i)
    {
        if (clients[i]->client_socket != data->client_socket)
        {
            memset(data->buffer + strlen(data->buffer), '\n', 1);
            memcpy(data->buffer + strlen(data->buffer), clients[i]->name, strlen(clients[i]->name));
        }
    }
    memset(data->buffer, strlen(data->buffer), MESSAGE_SIZE);
    memset(data->buffer + 1, USERLIST, TYPE_SIZE);
    memset(data->buffer + 2, 1, 1);
    if (data->client_socket != 0)
        send(data->client_socket, data->buffer, strlen(data->buffer), 0);
    else
    {
        memmove(data->buffer, data->buffer + 4, strlen(data->buffer) - 3);
        printf("\033[33m%s\033[0m\n", data->buffer);
    }
    pthread_mutex_unlock(&mutex);
}

void errorMessage(struct ThreadData *data, char *message)
{
    sprintf(data->buffer, "5%c%s%s", (char)strlen(data->name), data->name, message);
    memmove(data->buffer + MESSAGE_SIZE, data->buffer, strlen(data->buffer) + 1);
    memset(data->buffer, strlen(data->buffer), MESSAGE_SIZE);
    if (data->client_socket != 0)
        send(data->client_socket, data->buffer, strlen(data->buffer), 0);
    else
    {
        memmove(data->buffer, data->buffer + 9, strlen(data->buffer) - 8);
        printf("\033[31m%s\033[0m\n", data->buffer);
    }
}

int kickUser(char *message, char *name)
{
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < countOfClients; ++i)
    {
        if (strlen(clients[i]->name) == strlen(name))
        {
            if (strcmp(clients[i]->name, name) == 0)
            {
                send(clients[i]->client_socket, message, strlen(message), 0);
                pthread_mutex_unlock(&mutex);
                return 1;
            }
        }
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}

void sendMessageToClients(struct ThreadData *data)
{
    int nameLen = strlen(data->name);
    int len = strlen(data->buffer);
    int messageLen = len;
    memmove(data->buffer + MESSAGE_SIZE + TYPE_SIZE + nameLen + 1, data->buffer, len);
    len += nameLen + 3;
    memset(data->buffer, len, MESSAGE_SIZE);
    memset(data->buffer + 2, nameLen, 1);
    memcpy(data->buffer + 3, data->name, nameLen);
    data->buffer[len] = 0;
    if (data->buffer[3 + nameLen] != '/')
    {
        if (data->client_socket != 0)
            memset(data->buffer + 1, BROADCASTMESSAGE, TYPE_SIZE);
        else
            memset(data->buffer + 1, BROADCASTSERVER, TYPE_SIZE);
        broadcastMessage(data->buffer, data->client_socket);
        return;
    }
    char command[5];
    strncpy(command, data->buffer + 4 + nameLen, 4);
    command[4] = 0;
    if (strcmp(command, "priv") == 0)
    {
        char name[MAX_NAME_LENGTH];
        for (int i = 0; i < MAX_NAME_LENGTH; ++i)
        {
            if (data->buffer[9 + nameLen + i] == ' ')
            {
                name[i] = 0;
                break;
            }
            else
            {
                name[i] = data->buffer[9 + nameLen + i];
            }
        }
        memmove(data->buffer + 3 + nameLen, data->buffer + 10 + nameLen + strlen(name), strlen(data->buffer) - 9 - nameLen - strlen(name));
        memset(data->buffer, len - 7 - strlen(name), MESSAGE_SIZE);
        if (data->client_socket != 0)
            memset(data->buffer + 1, PRIVATEMESSAGE, TYPE_SIZE);
        else
            memset(data->buffer + 1, PRIVATESERVER, TYPE_SIZE);
        if (!privateMessage(data->buffer, name))
        {
            errorMessage(data, "No user");
        }
        return;
    }
    if (strcmp(command, "list") == 0)
    {
        userList(data);
        return;
    }
    errorMessage(data, "No command");
}

void clientConnected(struct ThreadData *data)
{
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(data->client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(data->client_address.sin_port);
    pthread_mutex_lock(&mutex);
    printf("\033[33m[LOG]\033[0m Client \033[33m%s\033[0m connected: %s:%d\n", data->name, client_ip, client_port);
    pthread_mutex_unlock(&mutex);
    sprintf(data->buffer, "0%c%sconnected", (char)strlen(data->name), data->name);
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
    sprintf(data->buffer, "0%c%sdisconnected", (char)strlen(data->name), data->name);
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
            count += strlen(buffer) - 1;
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
        int cat = strlen(name) + 4;
        memset(buffer + BUFFERSIZE - cat, 0, cat);
        pthread_mutex_lock(&mutex);
        printf("\033[33m[LOG]\033[0m {\033[33m%s\033[0m} {%s:%d}: %s\n", name, client_ip, client_port, buffer);
        pthread_mutex_unlock(&mutex);
        sendMessageToClients(thread_data);
    }
}

void *send_message(void *arg)
{
    char message[BUFFERSIZE];
    memset(message, 0, BUFFERSIZE);
    ssize_t len = BUFFERSIZE;
    struct ThreadData *threadData = (struct ThreadData *)malloc(sizeof(struct ThreadData));
    threadData->client_socket = 0;
    memset(threadData->name, 0, MAX_NAME_LENGTH);
    memcpy(threadData->name, "SERVER", 6);
    while (1)
    {
        fgets(message, BUFFERSIZE, stdin);
        message[strlen(message) - 1] = 0;
        if (strlen(message) == 126)
        {
            int c;
            while ((c = getc(stdin)) != '\n' && c != EOF)
            {
            }
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
        for (int i = 1; i < BUFFERSIZE; ++i)
        {
            if (message[i] == ' ' && message[i - 1] == ' ')
                continue;
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
        if (strncmp(message, "/exit", 5) == 0)
            exit(EXIT_SUCCESS);
        memcpy(threadData->buffer, message, BUFFERSIZE);
        if (strncmp(message, "/kick", 5) == 0)
        {
            char name[MAX_NAME_LENGTH];
            for (int i = 0; i < MAX_NAME_LENGTH; ++i)
            {
                if (threadData->buffer[6 + i] == ' ')
                {
                    name[i] = 0;
                    break;
                }
                else
                {
                    name[i] = threadData->buffer[6 + i];
                }
            }
            sprintf(threadData->buffer, "0000YOU BE KICKED");
            memset(threadData->buffer, strlen(threadData->buffer) - 1, MESSAGE_SIZE);
            memset(threadData->buffer + 1, KICK, TYPE_SIZE);
            memset(threadData->buffer + 2, 1, 1);
            if (!kickUser(threadData->buffer, name))
            {
                errorMessage(threadData, "No user");
            }
            continue;
        }
        sendMessageToClients(threadData);
    }
}

int main()
{
    struct ThreadData *threadData;

    pthread_t thread_id, send_thread;
    int clientSocket;
    int serverSocket = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress, clientAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    socklen_t length = sizeof(serverAddress);
    Bind(serverSocket, &serverAddress, length);
    GetSockName(serverSocket, &serverAddress, &length);
    Listen(serverSocket, LISTEN_QUEUE);

    pthread_create(&send_thread, NULL, send_message, NULL);

    printf("Open server:\nPort = %d\n", ntohs(serverAddress.sin_port));
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
    pthread_join(send_thread, NULL);
    close(serverSocket);
    return EXIT_SUCCESS;
}