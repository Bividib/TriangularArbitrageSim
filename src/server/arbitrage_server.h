#ifndef ARBITRAGE_SERVER_H
#define ARBITRAGE_SERVER_H

#include "common/common.h"
#include "common/trade_leg.h"
#include <memory>
#include <set>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>

struct ServerConfig {
    double profitThreshold;
    double takerFee;
    double initialNotional;

    // Constructor to easily initialize config
    ServerConfig(double profitThresh, double fee, double notional)
        : profitThreshold(profitThresh), takerFee(fee), initialNotional(notional) {}
};

class Server : public std::enable_shared_from_this<Server> {
public:
    Server(const ArbitragePath& path,
           const ServerConfig& config);

    //Receive updates from 3rd party clients
    void on_update(const OrderBookTick& update);

private:
    std::mutex mutex;
    double currentNotional; 
    long long lastUpdateId; 
    const ArbitragePath path;
    const ServerConfig config;
    std::unordered_map<std::string, OrderBookTick> pairToPriceMap; 

};

#endif //ARBITRAGE_SERVER_H