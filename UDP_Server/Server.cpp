#include <iostream>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <unistd.h>

const int PORT = 12345;
const int BUFFER_SIZE = 1024;

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // ���� ����
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // ���� �ּ� ����
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // ���Ͽ� �ּ� �Ҵ�
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        // Ŭ���̾�Ʈ�κ��� ������ ����
        ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0,
            (struct sockaddr*)&client_addr, &client_len);

        if (recv_len == -1) {
            perror("Error in receiving data");
            continue;
        }

        // ������ �����͸� ȭ�鿡 ���
        buffer[recv_len] = '\0';
        std::cout << "Received message from " << inet_ntoa(client_addr.sin_addr) << ": " << buffer << std::endl;

        // Ŭ���̾�Ʈ���� ������ �۽�
        const char* response = "Hello from server!";
        ssize_t send_len = sendto(sockfd, response, strlen(response), 0,
            (struct sockaddr*)&client_addr, client_len);

        if (send_len == -1) {
            perror("Error in sending data");
            continue;
        }
    }

    // ���� �ݱ�
    close(sockfd);

    return 0;
}
