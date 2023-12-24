#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;

class EchoSession : public std::enable_shared_from_this<EchoSession> {
public:
    EchoSession(io_context& ioContext) : socket(ioContext) {}

    ip::tcp::socket& getSocket() {
        return socket;
    }

    void start() {
        readData();
    }

private:
    void readData() {
        auto self(shared_from_this());
        async_read_until(socket, buffer(data, max_length), '\n',
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    writeData(length);
                }
            });
    }

    void writeData(std::size_t length) {
        auto self(shared_from_this());
        async_write(socket, buffer(data, length),
            [this, self](boost::system::error_code ec, std::size_t /* length */) {
                if (!ec) {
                    readData();  // ���� �����͸� ��ٸ�
                }
            });
    }

private:
    ip::tcp::socket socket;
    enum { max_length = 1024 };
    char data[max_length];
};

class EchoServer {
public:
    EchoServer(io_context& ioContext, unsigned short port)
        : ioContext(ioContext), acceptor(ioContext, ip::tcp::endpoint(ip::tcp::v4(), port)) {
        startAccept();
    }

private:
    void startAccept() {
        auto session = std::make_shared<EchoSession>(ioContext);

        acceptor.async_accept(session->getSocket(),
            [this, session](boost::system::error_code ec) {
                if (!ec) {
                    session->start();
                }
                startAccept();  // ���� ���� ���
            });
    }

private:
    io_context& ioContext;
    ip::tcp::acceptor acceptor;
};

int main() {
    try {
        io_context ioContext;
        unsigned short port = 7777;

        EchoServer server(ioContext, port);

        // �񵿱� �̺�Ʈ ó�� ����
        ioContext.run();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
