#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>

#include "socketFunctions.h"

int main()
{
    int clientSockets[MAX_CLIENTS];
    int clientSocket;
    fd_set read_fds; // набор файловых дескрипторов
    int max_socket;
    int serverSocket = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress, clientAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    socklen_t length = sizeof(serverAddress);
    Bind(serverSocket, &serverAddress, length);
    GetSockName(serverSocket, &serverAddress, &length);
    Listen(serverSocket, LISTEN_QUEUE);
    printf("Open server:\nAddress = %d\tPort = %d\n", ntohl(serverAddress.sin_addr.s_addr), ntohs(serverAddress.sin_port));

    memset(clientSockets, 0, sizeof(clientSockets));
    char client_ip[INET_ADDRSTRLEN];
    char buffer[BUFFERSIZE];

    while (1)
    {
        FD_ZERO(&read_fds); // очищение набора файловых дескриптеров
        FD_SET(serverSocket, &read_fds);
        max_socket = serverSocket;
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int tmp = clientSockets[i];
            if (tmp > 0)
            {
                FD_SET(tmp, &read_fds);
            }

            if (tmp > max_socket)
            {
                max_socket = tmp;
            }
        }
        int activ = select(max_socket+1, &read_fds, NULL, NULL, NULL);
        if (activ < 0)
        {
            perror("Select error");
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(serverSocket, &read_fds)) // если сокет сервера остался
        {
            length = sizeof(clientAddress);
            clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &length);
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clientSockets[i] == 0)
                {
                    clientSockets[i] = clientSocket;
                    printf("Client connected, socket fd is %d\n", clientSocket);
                    break;
                }
            }
        }
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (FD_ISSET(clientSockets[i], &read_fds))
            {
                ssize_t bytes_received = recv(clientSockets[i], buffer, sizeof(buffer), 0);
                if (bytes_received <= 0)
                {
                    // Обработка отключения клиента
                    printf("Client disconnected, socket fd is %d\n", clientSockets[i]);
                    close(clientSockets[i]);
                    clientSockets[i] = 0;
                }
                else
                {
                    inet_ntop(AF_INET, &(clientAddress.sin_addr), client_ip, INET_ADDRSTRLEN);
                    int client_port = ntohs(clientAddress.sin_port);
                    buffer[bytes_received] = '\0';
                    printf("SERVER: IP адрес клиента: %s\n", client_ip);
                    printf("SERVER: PORT клиента: %d\n", client_port);
                    printf("SERVER: Сообщение: %s\n\n", buffer);
                    char ans[] = "ANSWERD FROM SERVER FOR  ";
                    ans[24] = buffer[0];
                    send(clientSockets[i], ans, strlen(ans), 0);
                }
            }
        }
    }
    close(serverSocket);
    return EXIT_SUCCESS;
}