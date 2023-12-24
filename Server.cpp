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

            std::cout << "Client connected." << std::endl;

            try {
                for (;;) {
                    // �����͸� �޾Ƽ� �ٽ� Ŭ���̾�Ʈ���� ����
                    std::array<char, 1024> data;
                    size_t bytesRead = socket.read_some(buffer(data));

                    if (bytesRead == 0) {
                        // Ŭ���̾�Ʈ�� ������ ������
                        std::cout << "Client disconnected." << std::endl;
                        break;
                    }

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
