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

            // 클라이언트 정보 획득
            ip::tcp::endpoint clientEndpoint = socket.remote_endpoint();
            std::string clientIP = clientEndpoint.address().to_string();
            unsigned short clientPort = clientEndpoint.port();

            std::cout << "Client connected from " << clientIP << ":" << clientPort << std::endl;

            try {
                for (;;) {
                    // 데이터를 받아서 다시 클라이언트에게 보냄
                    std::array<char, 1024> data;
                    size_t bytesRead = socket.read_some(buffer(data));

                    if (bytesRead == 0) {
                        // 클라이언트가 연결을 종료함
                        std::cout << "Client disconnected: " << clientIP << ":" << clientPort << std::endl;
                        break;
                    }

                    // 클라이언트가 보낸 메시지 출력
                    std::cout << "Message from " << clientIP << ":" << clientPort << ": " << std::string(data.data(), bytesRead) << std::endl;

                    // 클라이언트에게 메시지 다시 전송
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
