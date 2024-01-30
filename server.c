#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#include "socketFunctions.h"

void signal_handler(int signo)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main()
{
    signal(SIGCHLD, signal_handler);

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
        length = sizeof(clientAddress);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &length);
        if (clientSocket == -1)
        {
            perror("Accept failed");
            continue;
        }
        pid_t children_pid = fork();
        if (children_pid == -1)
        {
            perror("Fork failed");
            close(clientSocket);
            continue;
        }
        else if (children_pid == 0)
        {
            close(serverSocket);

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddress.sin_addr), client_ip, INET_ADDRSTRLEN);
            int client_port = ntohs(clientAddress.sin_port);

            char buffer[BUFFERSIZE];
            ssize_t bytes_received;
            while ((bytes_received = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
            {
                buffer[bytes_received] = '\0';
                printf("SERVER: IP адрес клиента: %s\n", client_ip);
                printf("SERVER: PORT клиента: %d\n", client_port);
                printf("SERVER: PID процесса сервера: %d\n", getpid());
                printf("SERVER: Сообщение: %s\n\n", buffer);
                char ans[] = "ANSWERD FROM SERVER FOR  ";
                ans[24] = buffer[0];
                send(clientSocket, ans, strlen(ans), 0);
            }
            close(clientSocket);
            exit(EXIT_SUCCESS);
        }
        else
        {
            close(clientSocket);
        }
    }
    close(serverSocket);
    return EXIT_SUCCESS;
}