#define _WIN32_WINNT 0x0601 
#define NOMINMAX          

#include <iostream>
#include "server/arbitrage_server.h"
#include "server/arbitrage_session.h" 
#include <boost/asio/ssl.hpp>
#include "exchange/binance/binance_client.h"
#include <memory>

int main() {

    // -- LOCAL SERVER SETUP -- 

    // Run this server locally if you want to create a local websocket connection
    // In this manner, the client becomes the market data streamer 
    // auto server = std::make_shared<Server>(io_context,endpoint);
    // server->run();


    // -- BEGIN CLIENT CODE (Receives the market data) -- 

    const std::string host = "stream.binance.com";
    const std::string port = "9443";
    const std::string target = "/stream?streams=btcusdt@bookTicker";

    boost::asio::io_context io_context; 
    boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12_client);
    ctx.set_verify_mode(boost::asio::ssl::verify_none);

    
    // boost::asio::ip::tcp::resolver resolver(io_context);
    // auto& results = resolver.resolve(host,port);
    // //Create a websocket stream with SSL
    // boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> ws(io_context, ctx);
    // auto& lowest_layer = boost::beast::get_lowest_layer(ws);
    // lowest_layer.connect(results);
    // ws.next_layer().handshake( boost::asio::ssl::stream_base::client);
    // ws.handshake(host,target);
    // //create flat buffer
    // boost::beast::flat_buffer buffer;
    // ws.read(buffer);

    boost::asio::ip::tcp::resolver resolver(io_context);
    auto results = resolver.resolve(host, port);

    boost::beast::ssl_stream<boost::beast::tcp_stream> ssl_stream(io_context, ctx);
    boost::beast::get_lowest_layer(ssl_stream).connect(results);
    ssl_stream.handshake(boost::asio::ssl::stream_base::client);
    std::unique_ptr<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>> ws_ptr;
    ws_ptr = std::make_unique<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>>(std::move(ssl_stream));
    ws_ptr->set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::request_type& req) {
        req.set(boost::beast::http::field::user_agent, std::string("Boost.Beast WebSocket Client"));
    }));

    ws_ptr->handshake(host, target); 

    boost::beast::flat_buffer buffer;

    ws_ptr->read(buffer);

    const std::string& json_string = boost::beast::buffers_to_string(buffer.data());
    std::cout << "Received: " << json_string << std::endl;


    // auto client = std::make_shared<BinanceClient>(io_context,ctx);
    // client->async_connect(host, port, target);

    // -- END CLIENT CODE --

    io_context.run();
    return 0;
}