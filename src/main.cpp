#define _WIN32_WINNT 0x0601 
#define NOMINMAX          

#include <iostream>
#include "server/arbitrage_server.h"
#include <boost/asio/ssl.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include "exchange/binance/binance_client.h"
#include <memory>

int main() {

    // Read environment variables for the path to write data to 
    std::string trade_write_file_path = std::getenv("TRADE_WRITE_FILE_PATH");

    const std::string host = "stream.binance.com";
    const std::string port = "9443";
    // const std::string target = "/stream?streams=btcusdt@depth5@100ms/ethbtc@depth5@100ms/ethusdt@depth5@100ms";
    const std::string target = "/stream?streams=btcusdt@depth5@1000ms/ethbtc@depth5@1000ms/ethusdt@depth5@1000ms";

    boost::asio::io_context io_context; 
    auto work_guard = boost::asio::make_work_guard(io_context);

    // Use TLS Version 1.2 
    boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12_client);

    ArbitragePath path(
        "btc",
        TradeLeg("btcusdt", false), 
        TradeLeg("ethusdt", true),
        TradeLeg("ethbtc", false)
    );

    ServerConfig server_config(
        0.00001, // profit threshold of 0.1%
        0.00005, // taker fee of 0.05%
        0.01 // initial notional amount
    );

    auto client = std::make_shared<BinanceClient>(io_context,ctx);

    if (trade_write_file_path.empty()) {
        auto server = std::make_shared<Server>(path, server_config);
        client->set_callback(server);
    } else {
        auto server = std::make_shared<Server>(path, server_config, std::make_unique<TradeFileWriter>(trade_write_file_path));
        client->set_callback(server);
    }

    client->async_connect(host, port, target);

    io_context.run();
    return 0;
}