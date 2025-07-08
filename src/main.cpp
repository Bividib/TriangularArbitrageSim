#define _WIN32_WINNT 0x0601 
#define NOMINMAX          

#include <iostream>
#include "server/arbitrage_server.h"
#include <boost/asio/ssl.hpp>
#include "exchange/binance/binance_client.h"
#include <memory>

int main() {

    const std::string host = "stream.binance.com";
    const std::string port = "9443";
    const std::string target = "/stream?streams=btcusdt@depth5@100ms/ethbtc@depth5@100ms/ethusdt@depth5@100ms";

    boost::asio::io_context io_context; 

    // Use TLS Version 1.2 
    boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12_client);
    
    auto server = std::make_shared<Server>("btcusdt", "ethbtc", "ethusdt", 0.01,0.001);
    auto client = std::make_shared<BinanceClient>(io_context,ctx);
    client->add_callback(server);
    client->async_connect(host, port, target);

    io_context.run();
    return 0;
}