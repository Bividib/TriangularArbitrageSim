#define _WIN32_WINNT 0x0601 
#define NOMINMAX    

#include "arbitrage_session.h"
#include "common/common.h"

#include <iostream>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

const std::string Session::WS_SERVER_HEADER = "TriangularArbitrageAsyncWsServer";

// Take ownership of R-Value socket reference 
Session::Session(boost::asio::ip::tcp::socket&& socket) : ws(std::move(socket)){

    // Set websocket handshake timeout if client fails to complete after establishing the underlying TCP Connection
    ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

    // Apply custom headers 
    ws.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::response_type& res){
            res.set(boost::beast::http::field::server,WS_SERVER_HEADER);
        }
    ));
}

void Session::run(){
    // Async accept the client's HTTP upgrade request to Websocket Protocol 
    ws.async_accept(boost::beast::bind_front_handler(
        &Session::on_accept,
        shared_from_this()
    ));
}

void Session::on_accept(boost::beast::error_code ec){
    if (ec){
        return fail(ec,"Accept WS Connection");
    }

    std::cout << "Server session WS Handshake successful" << "\n";

    //Start reading incoming messages 
    read();
}

void Session::read(){
    ws.async_read(buffer,boost::beast::bind_front_handler(
        &Session::on_read,
        shared_from_this()
    ));
}

void Session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred){

    // Sometimes we only handle exception, no need to use bytes_transferred parameter 
    boost::ignore_unused(bytes_transferred);

    // Client wants to disconnect, accept close handshake
    if (ec == boost::beast::websocket::error::closed){
        return; 
    }

    if (ec){
        fail(ec,"Read Data");
    } else {
        // Read the data
        // TODO: Read the actual market data with a data structure of choice
        std::string data = boost::beast::buffers_to_string(buffer.data());
        std::cout << "Server Session Received Data: " << data << "\n";
    }

    // Prepare read for next data 
    buffer.clear();
    read();
}
