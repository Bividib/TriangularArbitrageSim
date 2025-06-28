#ifndef MARKET_DATA_CLIENT_H
#define MARKET_DATA_CLIENT_H

#include <memory>
#include <string>
#include <boost/asio/io_context.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp> 
#include <boost/beast/core/error.hpp>

// Client class declaration
class Client : public std::enable_shared_from_this<Client> {
public:
    // Constructor declaration
    explicit Client(boost::asio::io_context& ioc);

    // Method to initiate an asynchronous connection
    void async_connect(boost::asio::ip::tcp::endpoint& endpoint);

private:
    // Member variable declarations
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws;
    boost::beast::flat_buffer buffer;
    static const std::string WS_CLIENT_HEADER; // Declaration of static const member
    std::string host;
    std::string port; // Declared as string, as it stores the string representation of port

    // Private helper methods declarations
    void on_connect(boost::beast::error_code ec);
    void on_handshake(boost::beast::error_code ec);
    void write(); // Initiates a write operation
    void write_str_to_buf(const std::string& str, boost::beast::flat_buffer& fb); // Helper to write string to buffer
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_close(boost::beast::error_code ec);
};

#endif 