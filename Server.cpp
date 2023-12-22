#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;
using ip::tcp;

void handle_read(const boost::system::error_code& error, size_t bytes_transferred, std::array<char, 1024>& data, tcp::socket& socket) {
    if (!error) {
        std::cout << "Received data: " << std::string(data.data(), bytes_transferred) << std::endl;

        // Echo the received data back to the client
        async_write(socket, buffer(data, bytes_transferred),
            [&](const boost::system::error_code& write_error, size_t /*bytes_written*/) {
                if (write_error) {
                    std::cerr << "Error during write: " << write_error.message() << std::endl;
                }
            });
    }
    else {
        std::cerr << "Error during read: " << error.message() << std::endl;
    }
}

int main() {
    try {
        boost::asio::io_service io_service;

        // Server
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 7777));
        tcp::socket server_socket(io_service);

        std::cout << "Waiting for connection..." << std::endl;
        acceptor.accept(server_socket);
        std::cout << "Connection established." << std::endl;

        std::array<char, 1024> data;

        // Read data from the client
        server_socket.async_read_some(buffer(data), boost::bind(handle_read, placeholders::error, placeholders::bytes_transferred, data, server_socket));

        // Run the io_service to handle asynchronous operations
        io_service.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
