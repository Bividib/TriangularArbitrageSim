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
    const char* env_max_starting_notional_fraction = std::getenv("BINANCE_MAX_STARTING_NOTIONAL_FRACTION");
    const char* env_max_starting_notional_recalc_interval = std::getenv("BINANCE_MAX_STARTING_NOTIONAL_RECALC_INTERVAL");
    const char* env_use_first_level_only = std::getenv("BINANCE_USE_FIRST_LEVEL_ONLY");

    const std::string host = "stream.binance.com";
    const std::string port = "9443";

    const std::string trade_write_file_path = env_path ? env_path : "";
    const std::string target = env_target ? env_target : "/stream?streams=btcusdt@depth1@100ms/ltcbtc@depth1@100ms/ltcusdt@depth1@100ms";
    const std::string arbitrage_path = env_arbitrage_path ? env_arbitrage_path : "usdt:btcusdt:SELL,ltcbtc:SELL,ltcusdt:BUY";

    boost::asio::io_context io_context; 
    auto work_guard = boost::asio::make_work_guard(io_context);

    // Use TLS Version 1.2 
    boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12_client);

    ArbitragePath path = ArbitragePath::from_string(arbitrage_path);

    ServerConfig server_config(
        env_profit_threshold ? std::stod(env_profit_threshold) : 0,
        env_taker_fee ? std::stod(env_taker_fee) : 0.0000,
        env_max_starting_notional_fraction ? std::stod(env_max_starting_notional_fraction) : 0.8,
        env_max_starting_notional_recalc_interval ? std::stod(env_max_starting_notional_recalc_interval) : 0,
        env_use_first_level_only ? std::string(env_use_first_level_only) == "true" : true
    );

    auto client = std::make_shared<BinanceClient>(io_context,ctx);

    if (trade_write_file_path.empty()) {
        auto server = std::make_shared<Server>(path, server_config);
        client->set_callback(server);
    } else {
        auto server = std::make_shared<Server>(path, server_config, std::make_unique<TradeFileWriter>(trade_write_file_path));
        client->set_callback(server);
    }

    std::cout << "Starting Triangular Arbitrage Bot" << "\n"
              << "########## CONFIGURATION ##########" << "\n"
              << "Writing results to: " << trade_write_file_path << "\n"
              << "Websocket Stream Target: " << target << "\n"
              << "Arbitrage Path to Search: " << arbitrage_path << "\n"
              << "Profit Threshold: " << server_config.profitThreshold << "\n"
              << "Taker Fee: " << server_config.takerFee << "\n"
              << "Max Starting Notional Fraction: " << server_config.maxStartingNotionalFraction << "\n"
              << "Max Starting Notional Recalc Interval: " << server_config.maxStartingNotionalRecalcInterval << "\n"
              << "####################################" << "\n";

    client->async_connect(host, port, target);

    io_context.run();
    return 0;
}