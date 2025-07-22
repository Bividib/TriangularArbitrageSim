#define _WIN32_WINNT 0x0601 
#define NOMINMAX    

#include "arbitrage_server.h"
#include "common/common.h"
#include <boost/asio/strand.hpp>
#include <iostream>
#include <cmath>

Server::Server(const ArbitragePath& path, 
               const ServerConfig& config)
               : lastUpdateId(0), 
               path(path),
               config(config),
               currentNotional(config.initialNotional) {
}

void Server::on_update(const OrderBookTick& update) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = pairToPriceMap.find(update.symbol);

    if (it != pairToPriceMap.end() && update.updateId <= it->second.updateId) {
        return;
    }
    
    pairToPriceMap.insert_or_assign(update.symbol, update);

    // Wait until we have at least 3 pairs to check for arbitrage opportunities
    if (pairToPriceMap.size() < 3){
        return; 
    }

    // Scale up minimum expected notional amount for arbitrage
    const double thresholdNotional = config.initialNotional * (1 + config.profitThreshold);

    double newNotional = config.initialNotional;

    // Loop 3 all three legs of the arbitrage path
    for (int i = 0; i < 3; ++i) {
        const auto& trade_leg = path.legs[i];
        const auto& leg_tick = pairToPriceMap.find(trade_leg.symbol)->second;
        double nextNotional = trade_leg.getEffectiveRate(leg_tick,newNotional);

        if (nextNotional <= 0){
            return;
        }

        newNotional = nextNotional;
    }

    // Subtract taker fees
    newNotional = newNotional * std::pow(1 - config.takerFee, 3); 

    if (newNotional >= thresholdNotional) {
        std::cout << "Arbitrage opportunity detected!\n";
        std::cout << "Initial Traded Notional: " << config.initialNotional << "\n";
        std::cout << "Final Notional after Trades: " << newNotional << "\n";
        std::cout << "Profit: " << (newNotional - config.initialNotional) << "\n";

        currentNotional = newNotional;
    } 

}
