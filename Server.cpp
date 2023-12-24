#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;

int main() {
    try {
        io_service io_service;

        // ���� ����
        ip::tcp::acceptor acceptor(io_service, ip::tcp::endpoint(ip::tcp::v4(), 7777));

        std::cout << "Echo Server started. Listening on port 7777..." << std::endl;

        for (;;) {
            ip::tcp::socket socket(io_service);
            acceptor.accept(socket);

            // Ŭ���̾�Ʈ ���� ȹ��
            ip::tcp::endpoint clientEndpoint = socket.remote_endpoint();
            std::string clientIP = clientEndpoint.address().to_string();
            unsigned short clientPort = clientEndpoint.port();

            std::cout << "Client connected from " << clientIP << ":" << clientPort << std::endl;

            try {
                for (;;) {
                    // �����͸� �޾Ƽ� �ٽ� Ŭ���̾�Ʈ���� ����
                    std::array<char, 1024> data;
                    size_t bytesRead = socket.read_some(buffer(data));

                    if (bytesRead == 0) {
                        // Ŭ���̾�Ʈ�� ������ ������
                        std::cout << "Client disconnected: " << clientIP << ":" << clientPort << std::endl;
                        break;
                    }

                    // Ŭ���̾�Ʈ�� ���� �޽��� ���
                    std::cout << "Message from " << clientIP << ":" << clientPort << ": " << std::string(data.data(), bytesRead) << std::endl;

                    // Ŭ���̾�Ʈ���� �޽��� �ٽ� ����
                    socket.write_some(buffer(data, bytesRead));
                }
            }
            catch (std::exception& e) {
                // ���� �߻� �� ó��
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
