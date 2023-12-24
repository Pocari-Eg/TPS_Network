#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;

class Server {
public:
    Server(io_context& ioContext, unsigned short port)
        : ioContext(ioContext), acceptor(ioContext, ip::tcp::endpoint(ip::tcp::v4(), port)) {
        startAccept();
    }

private:
    void startAccept() {
        // 새로운 소켓 생성
        auto socket = std::make_shared<ip::tcp::socket>(ioContext);

        // 비동기 연결 수락 시작
        acceptor.async_accept(*socket,
            [this, socket](boost::system::error_code ec) {
                if (!ec) {
                    std::cout << "Accepted connection from: " << socket->remote_endpoint() << std::endl;

                    // 클라이언트와의 통신을 위한 세션 시작
                    startSession(socket);
                }
                else {
                    std::cerr << "Accept error: " << ec.message() << std::endl;
                }

                // 다시 연결을 대기
                startAccept();
            });
    }

    void startSession(std::shared_ptr<ip::tcp::socket> socket) {
        // 클라이언트와의 통신을 위한 비동기 작업을 여기에 추가
        // 예를 들어, async_read, async_write 등을 사용할 수 있음
    }

private:
    io_context& ioContext;
    ip::tcp::acceptor acceptor;
};

int main() {
    try {
        io_context ioContext;
        unsigned short port = 7777;

        Server server(ioContext, port);

        // 비동기 이벤트 처리 시작
        ioContext.run();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
