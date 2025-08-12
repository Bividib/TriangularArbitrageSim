#define _WIN32_WINNT 0x0601 
#define NOMINMAX          

#include <iostream>
#include "server/arbitrage_server.h"
#include <boost/asio/ssl.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include "exchange/binance/binance_client.h"
#include <memory>

int main() {

    const char* env_path = std::getenv("BINANCE_UPDATE_FILE_PATH");
    const char* env_target = std::getenv("BINANCE_STREAM_TARGET");
    const char* env_arbitrage_path = std::getenv("BINANCE_ARBITRAGE_PATH");
    const char* env_profit_threshold = std::getenv("BINANCE_PROFIT_THRESHOLD");
    const char* env_taker_fee = std::getenv("BINANCE_TAKER_FEE");

    const std::string host = "stream.binance.com";
    const std::string port = "9443";

    const std::string trade_write_file_path = env_path ? env_path : "";
    const std::string target = env_target ? env_target : "/stream?streams=btcusdt@depth5@1000ms/ethbtc@depth5@1000ms/ethusdt@depth5@1000ms";
    const std::string arbitrage_path = env_arbitrage_path ? env_arbitrage_path : "btc:btcusdt:BUY,ethusdt:SELL,ethbtc:BUY";

    boost::asio::io_context io_context; 
    auto work_guard = boost::asio::make_work_guard(io_context);

    // Use TLS Version 1.2 
    boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12_client);

    ArbitragePath path = ArbitragePath::from_string(arbitrage_path);

    ServerConfig server_config(
        env_profit_threshold ? std::stod(env_profit_threshold) : 0.0001,
        env_taker_fee ? std::stod(env_taker_fee) : 0.0005                   
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