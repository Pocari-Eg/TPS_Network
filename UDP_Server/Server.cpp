#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10
#define MAX_BUFFER_SIZE 1024

int main() {
    int serverSocket, clientSocket, epollfd, eventsCount;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    struct epoll_event ev, events[MAX_EVENTS];
    char buffer[MAX_BUFFER_SIZE];

    // Create TCP socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(7777);

    // Bind the socket to the address and port
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error binding socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) == -1) {
        perror("Error listening on socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Create epoll instance
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("Error creating epoll instance");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Add the server socket to the epoll instance
    ev.events = EPOLLIN;
    ev.data.fd = serverSocket;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serverSocket, &ev) == -1) {
        perror("Error adding server socket to epoll");
        close(serverSocket);
        close(epollfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port 8080...\n");

    while (1) {
        // Wait for events
        eventsCount = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (eventsCount == -1) {
            perror("Error waiting for events");
            break;
        }

        // Handle events
        for (int i = 0; i < eventsCount; i++) {
            if (events[i].data.fd == serverSocket) {
                // Accept incoming connection
                clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
                if (clientSocket == -1) {
                    perror("Error accepting connection");
                    break;
                }

                printf("New connection from %s\n", inet_ntoa(clientAddr.sin_addr));

                // Add the new client socket to the epoll instance
                ev.events = EPOLLIN;
                ev.data.fd = clientSocket;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientSocket, &ev) == -1) {
                    perror("Error adding client socket to epoll");
                    close(clientSocket);
                }
            }
            else {
                // Handle data from clients
                int bytesRead = read(events[i].data.fd, buffer, MAX_BUFFER_SIZE);
                if (bytesRead <= 0) {
                    // Connection closed or error
                    if (bytesRead == 0) {
                        printf("Connection closed by client\n");
                    }
                    else {
                        perror("Error reading from client socket");
                    }

                    // Remove the client socket from the epoll instance
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    close(events[i].data.fd);
                }
                else {
                    // Process the received data (in this example, just print it)
                    buffer[bytesRead] = '\0';
                    printf("Received data from client %s: %s", inet_ntoa(clientAddr.sin_addr), buffer);
                }
            }
        }
    }

    // Clean up
    close(serverSocket);
    close(epollfd);

    return 0;
}
