#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "socketFunctions.h"

int clients[MAX_CLIENTS];
int countOfClients = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct ThreadData
{
    int client_socket;
    struct sockaddr_in client_address;
    char buffer[BUFFERSIZE];
    char name[MAX_NAME_LENGTH];
};

void removeClientSocket(int socket)
{
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < countOfClients; ++i)
    {
        if (clients[i] == socket)
        {
            for (int j = i + 1; j < countOfClients; ++j)
            {
                clients[j - 1] = clients[j];
            }
            countOfClients--;
            clients[countOfClients] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
}

void broadcastMessage(char *message, int sender_socket)
{
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i] != sender_socket)
        {
            send(clients[i], message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

void clientConnected(struct ThreadData *data)
{
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(data->client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(data->client_address.sin_port);
    pthread_mutex_lock(&mutex);
    printf("Client %s connected: %s:%d\n", data->name, client_ip, client_port);
    pthread_mutex_unlock(&mutex);
    sprintf(data->buffer, "User %s connected", data->name);
    broadcastMessage(data->buffer, data->client_socket);
}

void clientDisconnected(struct ThreadData *data)
{
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(data->client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(data->client_address.sin_port);
    pthread_mutex_lock(&mutex);
    printf("Client %s disconnected: %s:%d\n", data->name, client_ip, client_port);
    pthread_mutex_unlock(&mutex);
    sprintf(data->buffer, "User %s disconnected", data->name);
    broadcastMessage(data->buffer, data->client_socket);
    removeClientSocket(data->client_socket);
    close(data->client_socket);
}

void *clientThread(void *data)
{
    struct ThreadData *thread_data = (struct ThreadData *)data;
    int client_socket = thread_data->client_socket;
    char* buffer = thread_data->buffer;
    struct sockaddr_in client_address = thread_data->client_address;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_address.sin_port);

    clientConnected(thread_data);
    while (1)
    {
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0)
        {
            clientDisconnected(thread_data);
            free(data);
            pthread_exit(EXIT_SUCCESS);
        }
        buffer[bytes_received] = 0;
        pthread_mutex_lock(&mutex);
        printf("Received from %s:%d: %s\n", client_ip, client_port, buffer);
        pthread_mutex_unlock(&mutex);
        broadcastMessage(buffer, client_socket);
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
        if (countOfClients < MAX_CLIENTS)
        {
            clients[countOfClients++] = clientSocket;
        }
        else
        {
            // TODO: отправка сообщение то что нет места
            continue;
        }

        threadData = (struct ThreadData *)malloc(sizeof(struct ThreadData));
        threadData->client_address = clientAddress;
        threadData->client_socket = clientSocket;
        memset(threadData->buffer, 0, BUFFERSIZE);

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