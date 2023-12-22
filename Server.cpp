// server.cpp
#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;
using ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        doRead();
    }

private:
    void doRead() {
        auto self(shared_from_this());
        socket_.async_read_some(
            buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    doWrite(length);
                }
                else {
                    std::cerr << "Read error: " << ec.message() << "\n";
                }
            });
    }

    void doWrite(std::size_t length) {
        auto self(shared_from_this());
        async_write(
            socket_,
            buffer(data_, length),
            [this, self](boost::system::error_code ec, std::size_t /* length */) {
                if (!ec) {
                    std::cout << "Received from client: " << data_ << "\n";
                    doRead();
                    // 클라이언트로 다시 데이터를 보내기
                    doSend();
                }
                else {
                    std::cerr << "Write error: " << ec.message() << "\n";
                }
            });
    }

    void doSend() {
        auto self(shared_from_this());
        std::string message = "I received your message.";
        async_write(
            socket_,
            buffer(message),
            [this, self](boost::system::error_code ec, std::size_t /* length */) {
                if (ec) {
                    std::cerr << "Send error: " << ec.message() << "\n";
                }
            });
    }

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class Server {
public:
    Server(io_service& io_service, short port)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
        socket_(io_service) {
        doAccept();
    }

private:
    void doAccept() {
        acceptor_.async_accept(
            socket_,
            [this](boost::system::error_code ec) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket_))->start();
                }
                else {
                    std::cerr << "Accept error: " << ec.message() << "\n";
                }

                doAccept();
            });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

int main() {
    try {
        io_service io_service;
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 7777));

        for (;;) {
            tcp::socket socket(io_service);
            acceptor.accept(socket);

            std::cout << "Client connected." << std::endl;

            // 클라이언트에게 "Welcome" 메시지를 보냅니다.
            std::string welcomeMessage = "Welcome";
            write(socket, buffer(welcomeMessage + "\n"));
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
