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

    // 소켓 생성
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // 소켓에 주소 할당
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        // 클라이언트로부터 데이터 수신
        ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0,
            (struct sockaddr*)&client_addr, &client_len);

        if (recv_len == -1) {
            perror("Error in receiving data");
            continue;
        }

        // 수신한 데이터를 화면에 출력
        buffer[recv_len] = '\0';
        std::cout << "Received message from " << inet_ntoa(client_addr.sin_addr) << ": " << buffer << std::endl;

        // 클라이언트에게 데이터 송신
        const char* response = "Hello from server!";
        ssize_t send_len = sendto(sockfd, response, strlen(response), 0,
            (struct sockaddr*)&client_addr, client_len);

        if (send_len == -1) {
            perror("Error in sending data");
            continue;
        }
    }

    // 소켓 닫기
    close(sockfd);

    return 0;
}
