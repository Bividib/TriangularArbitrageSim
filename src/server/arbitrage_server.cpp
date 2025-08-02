#define _WIN32_WINNT 0x0601 
#define NOMINMAX    

#include "arbitrage_server.h"
#include <boost/asio/strand.hpp>
#include <iomanip>
#include <iostream>
#include <cmath>

Server::Server(const ArbitragePath& path, 
               const ServerConfig& config)
               : lastUpdateId(0), 
               path(path),
               currentNotional(0),
               config(config),
               tradeFileWriter(nullptr) {
}

Server::Server(const ArbitragePath& path, 
               const ServerConfig& config,
               std::unique_ptr<TradeFileWriter>&& writer)
               : lastUpdateId(0), 
               path(path),
               currentNotional(0),
               config(config),
               tradeFileWriter(std::move(writer)) {
}

void Server::on_update(OrderBookTick& update) {
    // std::lock_guard<std::mutex> lock(mutex);

    auto it = pairToPriceMap.find(update.symbol);
    
    pairToPriceMap.insert_or_assign(update.symbol, update);

    // Wait until we have at least 3 pairs to check for arbitrage opportunities
    if (pairToPriceMap.size() < 3){
        return; 
    }

    // Scale up minimum expected notional amount for arbitrage
    double newNotional = config.initialNotional;

    // Loop 3 all three legs of the arbitrage path
    // see if we can cache the effective rate for each leg, to prevent recomputation
    // get current time 

    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());


    for (int i = 0; i < 3; ++i) {
        const auto& trade_leg = path.legs[i];
        const auto& leg_tick = pairToPriceMap.find(trade_leg.symbol)->second;
        double rate = trade_leg.getEffectiveRate(leg_tick,newNotional);

        std::cout << "Update ID: " << leg_tick.updateId
                  << ", Symbol: " << trade_leg.symbol
                  << ", Rate VWAP: " << rate << "\n";

        if (rate <= 0){
            return;
        }

        newNotional = newNotional * rate;
    }

    time_t now_after = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << "Time taken for 3 legs: " << std::difftime(now_after, now) << " seconds\n";

    // Subtract taker fees
    newNotional = newNotional * config.takerFee;

    double profit = newNotional - config.initialNotional;
    std::cout << std::fixed << std::setprecision(15) << profit << "\n"; // Example: 15 decimal places

    if (newNotional >= config.thresholdNotional) {
        std::cout << "Arbitrage opportunity detected!\n";
        std::cout << "Initial Traded Notional: " << config.initialNotional << "\n";
        std::cout << "Final Notional after Trades: " << newNotional << "\n";
        std::cout << "Profit: " << profit << "\n";

        currentNotional = newNotional;
    } 

    long long localTimestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    update.processTime = localTimestampMs - update.tickInitTime;
    update.unrealisedPnl = profit; 

    if (tradeFileWriter){
        tradeFileWriter->write(update);
    }
}
