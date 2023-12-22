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
                }
                else {
                    std::cerr << "Write error: " << ec.message() << "\n";
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
        Server server(io_service, 7777);

        // 비동기 작업 처리
        io_service.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
