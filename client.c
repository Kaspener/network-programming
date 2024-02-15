#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "socketFunctions.h"

void *receive_messages(void *arg) {
    char buffer[BUFFERSIZE];
    int socket = *(int*)arg;
    while (1) {
        int bytes_received = recv(socket, buffer, BUFFERSIZE, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected\n");
            close(socket);
            exit(EXIT_SUCCESS);
        }
        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
    }
}

void *send_message(void *arg) {
    char message[BUFFERSIZE];
    int socket = *(int*)arg;
    while (1) {
        scanf("%s", message);
        if (strncmp(message, "EXIT", 4) == 0) exit(EXIT_SUCCESS);
        send(socket, message, strlen(message), 0);
    }
}

struct sockaddr_in enterSocketAddress(char* ip, char* port)
{
    struct sockaddr_in sa;
    inet_pton(AF_INET, ip, &(sa.sin_addr));
    sa.sin_port = htons(atoi(port));
    sa.sin_family = AF_INET;
    return sa;
}

int main(int argc, char **argv)
{
    char buffer[BUFFERSIZE];
    int socket = Socket(AF_INET, SOCK_STREAM, 0);
    if (argc != 3){
        printf("USE: ./client <ip> <port>");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serverAddress = enterSocketAddress(argv[1], argv[2]);
    Connect(socket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));

    pthread_t receive_thread, send_thread;

    pthread_create(&receive_thread, NULL, receive_messages, &socket);

    pthread_create(&send_thread, NULL, send_message, &socket);

    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);
    close(socket);
    return EXIT_SUCCESS;
}
