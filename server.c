#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "socketFunctions.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
FILE *shared_file;

struct ThreadData
{
    int client_socket;
    struct sockaddr_in client_address;
};

void *clientThread(void *data)
{
    struct ThreadData *thread_data = (struct ThreadData *)data;
    int client_socket = thread_data->client_socket;
    struct sockaddr_in client_address = thread_data->client_address;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_address.sin_port);

    while (1)
    {
        char buffer[BUFFERSIZE];
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0)
        {
            close(client_socket);
            free(thread_data);
            pthread_exit(NULL);
        }

        buffer[bytes_received] = '\0';

        char ans[] = "ANSWERD FROM SERVER FOR  ";
        ans[24] = buffer[0];
        send(client_socket, ans, strlen(ans), 0);

        pthread_mutex_lock(&mutex);

        printf("Received from %s:%d: %s\n", client_ip, client_port, buffer);
        fprintf(shared_file, "Received from %s:%d: %s\n", client_ip, client_port, buffer);
        fflush(shared_file);

        pthread_mutex_unlock(&mutex);
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

    shared_file = fopen("shared_file.txt", "a");
    if (shared_file == NULL)
    {
        perror("Failed to open shared file");
        exit(EXIT_FAILURE);
    }

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
    fclose(shared_file);
    return EXIT_SUCCESS;
}