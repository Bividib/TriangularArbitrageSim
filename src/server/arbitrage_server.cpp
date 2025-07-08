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

    // Store most recent update for each pair
    if (it != pairToPriceMap.end()) {
        if (it->second.updateId <= update.updateId) {
            return;
        } else {
            it->second = update; 
        }
    } else {
        pairToPriceMap[update.symbol] = update; 
    }

    // Wait until we have at least 3 pairs to check for arbitrage opportunities
    if (pairToPriceMap.size() < 3){
        return; 
    }

    
    const auto& trade_leg1 = path.legs[0];
    const auto& trade_leg2 = path.legs[1];
    const auto& trade_leg3 = path.legs[2];

    // Latest tick updates for each leg of the arbitrage path
    const auto& leg1_tick = pairToPriceMap.find(trade_leg1.symbol)->second;
    const auto& leg2_tick = pairToPriceMap.find(trade_leg2.symbol)->second;
    const auto& leg3_tick = pairToPriceMap.find(trade_leg3.symbol)->second;

    // Scale up minimum expected notional amount for arbitrage
    const double thresholdNotional = config.initialNotional * (1 + config.profitThreshold);

    // Subtract taker fees 
    double newNotional = config.initialNotional;
    
    double price = trade_leg1.getEffectiveRate(leg1_tick,newNotional);

    if (price <= 0){
        return; 
    }

    // At this point we have converted initial notional from currency 1 to currency 2
    newNotional *= price;


    // Subtract taker fees
    newNotional = config.initialNotional * std::pow(1 - config.takerFee, 3); 
    

}
