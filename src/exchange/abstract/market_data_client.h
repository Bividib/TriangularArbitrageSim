#ifndef MARKET_DATA_CLIENT_H
#define MARKET_DATA_CLIENT_H

#include "common/common.h"
#include <memory>
#include <string>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp> 
#include <boost/beast/core/error.hpp>


/**
 * @class Client
 * @brief Abstract base for a WebSocket client connection.
 *
 * Handles the basic connection lifecycle (TCP connect, SSL handshake, WS handshake).
 * Uses pure virtual functions for handlers, forcing derived classes to implement them.
 */
class Client {
public:
    // Constructor must be public
    explicit Client(boost::asio::io_context& ioc, boost::asio::ssl::context& ssl_ctx) 
        : ws(boost::asio::make_strand(ioc), ssl_ctx) {}

    // Virtual destructor is essential for classes with virtual functions
    virtual ~Client() = default;

    // Public function to start the connection process
    virtual void async_connect(const std::string& host, const std::string& port, const std::string& target) = 0;

protected:
    // Member variables are protected so derived classes can access them
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> ws;
    boost::beast::flat_buffer buffer;
    std::string host;
    std::string port;
    std::string target;

    // These are the asynchronous handlers for the connection steps.
    // They are now pure virtual, forcing any concrete derived class to implement them.
    virtual void on_resolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results) = 0;
    virtual void on_connect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type endpoint) = 0;
    virtual void on_ssl_handshake(boost::beast::error_code ec) = 0;
    virtual void on_ws_handshake(boost::beast::error_code ec) = 0;
    virtual void on_close(boost::beast::error_code ec) = 0;

    // run_client will be called after a successful connection.
    // Pure virtual because the base class doesn't know what to do.
    virtual void run_client() = 0;
};


/**
 * @class BaseClient
 * @brief Extends Client to add abstract read/write operations.
 */
class BaseClient : public Client {
public:
    // Pass constructor arguments to the base class
    BaseClient(boost::asio::io_context& ioc, boost::asio::ssl::context& ssl_ctx)
        : Client(ioc, ssl_ctx) {}


protected:
    // Helper function to copy a string into the buffer for writing.
    // This doesn't need to be virtual as its implementation is universal.
    void write_str_to_buf(const std::string& str, boost::beast::flat_buffer& fb);

    // Abstract interface for reading and writing data.
    // These are pure virtual because a specific client (like BinanceClient)
    // will define how to handle reads and what to write.
    virtual void write() = 0;
    virtual void on_write(boost::beast::error_code ec, std::size_t bytes_transferred) = 0;
    virtual void read() = 0;
    virtual void on_read(boost::beast::error_code ec, std::size_t bytes_transferred) = 0;
};

#endif // MARKET_DATA_CLIENT_H