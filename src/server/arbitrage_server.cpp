#define _WIN32_WINNT 0x0601 
#define NOMINMAX    

#include "arbitrage_server.h"
#include "arbitrage_session.h"
#include "common/common.h"

#include <boost/asio/strand.hpp>
#include <iostream>

const int Server::MAX_CONNECTIONS = 5;

Server::Server(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint& endpoint) : acceptor(ioc), io_context(ioc) {
    boost::beast::error_code ec; 

    //Open the acceptor 
    acceptor.open(endpoint.protocol(),ec);

    if (ec){
        fail(ec,"Open Acceptor");
    }

    //Allow address reuse 
    acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);

    if (ec){
        fail(ec,"Set Acceptor Option");
    }

    //Bind to the server address 
    acceptor.bind(endpoint,ec);

    if (ec){
        fail(ec,"Address Bind");
    }

    //Start listening for the single connection
    acceptor.listen(MAX_CONNECTIONS,ec);

    if (ec){
        fail(ec,"Start Listening:");
    }

    std::cout << "Server listening on " <<  endpoint.address().to_string() << ":" << std::to_string(endpoint.port()) << "\n";
}

void Server::run(){
    do_accept();
}

void Server::do_accept(){
    std::cout << "Server waiting for incoming TCP connection request ..." << "\n";
    // Accept underlying TCP Connection 
    acceptor.async_accept(
        boost::asio::make_strand(io_context),
        boost::beast::bind_front_handler(
            &Server::on_accept,
            shared_from_this())
        );
}

void Server::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket){
    if (ec){
        fail(ec, "Accept TCP Connection");
    } else {
        std::cout << "Server accepted TCP handshake" << "\n";
        //Create a session on a separate thread for this connection
        //Wrap in shared pointer to prevent socket deallocation 
        std::make_shared<Session>(std::move(socket))->run();
    }

    // Prepare listener to the next client TCP Connection Request
    do_accept();
}
