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
        // ���ο� ���� ����
        auto socket = std::make_shared<ip::tcp::socket>(ioContext);

        // �񵿱� ���� ���� ����
        acceptor.async_accept(*socket,
            [this, socket](boost::system::error_code ec) {
                if (!ec) {
                    std::cout << "Accepted connection from: " << socket->remote_endpoint() << std::endl;

                    // Ŭ���̾�Ʈ���� ����� ���� ���� ����
                    startSession(socket);
                }
                else {
                    std::cerr << "Accept error: " << ec.message() << std::endl;
                }

                // �ٽ� ������ ���
                startAccept();
            });
    }

    void startSession(std::shared_ptr<ip::tcp::socket> socket) {
        // Ŭ���̾�Ʈ���� ����� ���� �񵿱� �۾��� ���⿡ �߰�
        // ���� ���, async_read, async_write ���� ����� �� ����
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

        // �񵿱� �̺�Ʈ ó�� ����
        ioContext.run();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
