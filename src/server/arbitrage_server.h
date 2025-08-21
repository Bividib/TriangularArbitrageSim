#ifndef ARBITRAGE_SERVER_H
#define ARBITRAGE_SERVER_H

#include "common/order_book.h"
#include "common/trade_leg.h"
#include "file/trade_file_writer.h"
#include "arbitrage_calculator.h"
#include <memory>
#include <set>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>

struct ServerConfig {
    double profitThreshold;
    double takerFee;
    double maxStartingNotionalFraction;
    double maxStartingNotionalRecalcInterval;

    // Constructor to easily initialize config
    ServerConfig(double profitThresh, double fee, double maxNotionalFraction, double maxNotionalRecalcInterval)
        : profitThreshold(profitThresh+1), takerFee(std::pow(1 - fee, 3)), maxStartingNotionalFraction(maxNotionalFraction), maxStartingNotionalRecalcInterval(maxNotionalRecalcInterval) {}
};

class Server : public std::enable_shared_from_this<Server> {
public:
    Server(const ArbitragePath& path,
           const ServerConfig& config);

    Server(const ArbitragePath& path,
           const ServerConfig& config,
           std::unique_ptr<TradeFileWriter>&& writer);

    //Receive updates from 3rd party clients
    void on_update(OrderBookTick& update);


private:
    std::mutex mutex;
    double currentNotional;
    double ticksRemainingBeforeRecalc;
    StartingNotional startingNotional;
    long long lastUpdateId; 
    const ArbitragePath path;
    const ServerConfig config;
    const std::unique_ptr<TradeFileWriter> tradeFileWriter;
    std::unordered_map<std::string, OrderBookTick> pairToPriceMap; 

    void recalcStartingNotional();

};

#endif //ARBITRAGE_SERVER_H