#define _WIN32_WINNT 0x0601 
#define NOMINMAX    

#include "market_data_client.h"
#include "common/common.h"
#include <iostream>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/bind_handler.hpp>

const std::string Client::WS_CLIENT_HEADER = "TriangularArbitrageAsyncWsClient";

Client::Client(boost::asio::io_context& ioc) : ws(boost::asio::make_strand(ioc)){
    // Set websocket handshake timeout if client fails to complete after establishing the underlying TCP Connection
    ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

    // Apply custom headers 
    ws.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::request_type& req){
            req.set(boost::beast::http::field::user_agent,WS_CLIENT_HEADER);
        }
    ));

    std::cout << "Client initialised" << "\n";
}

void Client::async_connect(boost::asio::ip::tcp::endpoint& endpoint){

    host = endpoint.address().to_string();
    port = std::to_string(endpoint.port());
    std::cout << "Connecting Async to Host: " << host << " Port: " << port << "\n";

    auto& lowest_layer = boost::beast::get_lowest_layer(ws);

    // Set Expiry for underlying TCP Connection
    lowest_layer.expires_after(std::chrono::seconds(30));

    lowest_layer.async_connect(
        endpoint,
        boost::beast::bind_front_handler(
            &Client::on_connect,
            shared_from_this()
        )
    );
}


void Client::on_connect(boost::beast::error_code ec){

    if (ec)
        return fail(ec,"Client Connect");

    std::cout << "Client TCP Connection established, starting WS Handshake..." << "\n";

    // Turn off Expiry for TCP Connection, Websockets have their own
    boost::beast::get_lowest_layer(ws).expires_never();

    // Try handshake with the Server
    ws.async_handshake(
        host,
        "/",
        boost::beast::bind_front_handler(
            &Client::on_handshake,
            shared_from_this()
        )
    );
}

void Client::on_handshake(boost::beast::error_code ec){

    if (ec){
        return fail(ec,"Client Handshake");
    } 

    std::cout << "Client successfully connected to Websocket" << "\n";

    write();
}

void Client::write(){
    buffer.clear();
    write_str_to_buf("Test Message",buffer);

    ws.async_write(
        buffer.data(),
        boost::beast::bind_front_handler(
            &Client::on_write,
            shared_from_this()
        )
    );
}

void Client::write_str_to_buf(const std::string& str, boost::beast::flat_buffer& fb){
    auto mutable_buf = fb.prepare(str.length());
    size_t bytes_copied = boost::asio::buffer_copy(mutable_buf,boost::asio::buffer(str));
    fb.commit(bytes_copied);
}

void Client::on_write(boost::beast::error_code ec,std::size_t bytes_transferred){
    boost::ignore_unused(bytes_transferred);

    if (ec){
        return fail(ec,"Client Write");
    }

    std::cout << "Client Write successful" << "\n";

    // Close the connection after the first write as this is a test for now 
    ws.async_close(
        boost::beast::websocket::close_code::normal,
        boost::beast::bind_front_handler(
            &Client::on_close,
            shared_from_this()
        )
    );
}

void Client::on_close(boost::beast::error_code ec){
    if (ec)
        return fail(ec,"Client Close");

    std::cout << "Client connection closed gracefully" << "\n";
}