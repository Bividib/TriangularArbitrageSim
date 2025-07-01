#ifndef ARBITRAGE_SERVER_H
#define ARBITRAGE_SERVER_H

#include <memory>
#include <set>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>

// Forward declare Session
class Session;

class Server : public std::enable_shared_from_this<Server> {
public:
    Server(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint& endpoint);
    Server();

    // Method to start the server's acceptance loop (acts as the client)
    void run();

    //Alternatively receive updates from 3rd party clients
    void on_update();

private:
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::io_context& io_context;
    static const int MAX_CONNECTIONS; 

    // Set to hold shared pointers to active Session objects
    std::set<std::shared_ptr<Session>> active_sessions; 

    void do_accept();
    void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);
};

#endif ARBITRAGE_SERVER_H