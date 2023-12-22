#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define MAX_BUFFER_SIZE 1024
#define SERVER_PORT 7777
#define MAX_EVENTS 10

void HandleClient(int clientSocket);

int main() {
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(listenSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
        std::cerr << "Failed to bind socket\n";
        close(listenSocket);
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen\n";
        close(listenSocket);
        return 1;
    }

    std::cout << "Server listening on port " << SERVER_PORT << "\n";

    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        std::cerr << "Failed to create epoll file descriptor\n";
        close(listenSocket);
        return 1;
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = listenSocket;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, listenSocket, &event) == -1) {
        std::cerr << "Failed to add listen socket to epoll\n";
        close(listenSocket);
        close(epollFd);
        return 1;
    }

    struct epoll_event* events = new struct epoll_event[MAX_EVENTS];

    while (true) {
        int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (numEvents == -1) {
            std::cerr << "epoll_wait failed\n";
            break;
        }

        for (int i = 0; i < numEvents; ++i) {
            if (events[i].data.fd == listenSocket) {
                // New client connection
                sockaddr_in clientAddr;
                socklen_t clientAddrLen = sizeof(clientAddr);
                int clientSocket = accept(listenSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);
                if (clientSocket == -1) {
                    std::cerr << "Failed to accept connection\n";
                    continue;
                }

                std::cout << "Client connected from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << "\n";

                fcntl(clientSocket, F_SETFL, O_NONBLOCK);

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = clientSocket;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1) {
                    std::cerr << "Failed to add client socket to epoll\n";
                    close(clientSocket);
                    continue;
                }
            }
            else {
                // Data available to read from a client
                int clientSocket = events[i].data.fd;
                HandleClient(clientSocket);
            }
        }
    }

    close(listenSocket);
    close(epollFd);
    delete[] events;

    return 0;
}

void HandleClient(int clientSocket) {
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytesRead = recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
    if (bytesRead == -1) {
        std::cerr << "recv failed\n";
        close(clientSocket);
        return;
    }
    else if (bytesRead == 0) {
        std::cout << "Client disconnected\n";
        close(clientSocket);
        return;
    }

    std::cout << "Received data from client: " << buffer << "\n";

    // Echo the received data back to the client
    ssize_t bytesSent = send(clientSocket, buffer, bytesRead, 0);
    if (bytesSent == -1) {
        std::cerr << "send failed\n";
        close(clientSocket);
        return;
    }

    std::cout << "Data sent to client successfully\n";
}
