#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

const int MAX_EVENTS = 10;
const int PORT = 7777;

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket\n";
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
        std::cerr << "Error binding socket\n";
        close(serverSocket);
        return -1;
    }

    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Error listening on socket\n";
        close(serverSocket);
        return -1;
    }

    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        std::cerr << "Error creating epoll file descriptor\n";
        close(serverSocket);
        return -1;
    }

    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = serverSocket;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &event) == -1) {
        std::cerr << "Error adding server socket to epoll\n";
        close(serverSocket);
        close(epollFd);
        return -1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    while (true) {
        epoll_event events[MAX_EVENTS];
        int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);

        for (int i = 0; i < numEvents; ++i) {
            if (events[i].data.fd == serverSocket) {
                // Accept new connection
                sockaddr_in clientAddr{};
                socklen_t clientAddrLen = sizeof(clientAddr);
                int clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);

                event.events = EPOLLIN | EPOLLET;  // Edge-triggered mode
                event.data.fd = clientSocket;

                epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event);

                std::cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << "\n";
            }
            else {
                // Handle data from existing connection
                char buffer[1024];
                int bytesRead = 0;

                while (true) {
                    bytesRead = recv(events[i].data.fd, buffer, sizeof(buffer) - 1, 0);

                    if (bytesRead <= 0) {
                        // Connection closed or error
                        close(events[i].data.fd);
                        std::cout << "Connection closed\n";
                        break;
                    }
                    else {
                        // Null-terminate the received data
                        buffer[bytesRead] = '\0';

                        // Display received message
                        std::cout << "Received message from client: " << buffer << std::endl;

                        // Echo the received data back to the client
                        send(events[i].data.fd, buffer, bytesRead, 0);
                    }
                }
            }
        }
    }

    close(serverSocket);
    close(epollFd);

    return 0;
}
