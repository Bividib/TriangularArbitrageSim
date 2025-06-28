#ifndef ARBITRAGE_SESSION_H
#define ARBITRAGE_SESSION_H

#include <memory>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket/stream.hpp>

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(boost::asio::ip::tcp::socket&& socket);
    void run();

private:
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws;
    boost::beast::flat_buffer buffer;
    static const std::string WS_SERVER_HEADER;

    void on_accept(boost::beast::error_code ec);
    void read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
};

#endif ARBITRAGE_SESSION_H