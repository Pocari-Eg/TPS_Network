#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;

int main() {
    try {
        io_service io_service;

        // 소켓 생성
        ip::tcp::acceptor acceptor(io_service, ip::tcp::endpoint(ip::tcp::v4(), 7777));

        std::cout << "Echo Server started. Listening on port 7777..." << std::endl;

        for (;;) {
            ip::tcp::socket socket(io_service);
            acceptor.accept(socket);

            std::cout << "Client connected." << std::endl;

            try {
                for (;;) {
                    // 데이터를 받아서 다시 클라이언트에게 보냄
                    std::array<char, 1024> data;
                    size_t bytesRead = socket.read_some(buffer(data));

                    if (bytesRead == 0) {
                        // 클라이언트가 연결을 종료함
                        std::cout << "Client disconnected." << std::endl;
                        break;
                    }

                    socket.write_some(buffer(data, bytesRead));
                }
            }
            catch (std::exception& e) {
                // 에러 발생 시 처리
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
