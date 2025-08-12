#define _WIN32_WINNT 0x0601 
#define NOMINMAX    

#include "arbitrage_server.h"
#include "arbitrage_calculator.h"
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
    
    pairToPriceMap.insert_or_assign(update.symbol, update);

    // Wait until we have at least 3 pairs to check for arbitrage opportunities
    if (pairToPriceMap.size() < 3){
        return; 
    }

    const StartingNotional& startingNotional = calculateStartingNotional(path,pairToPriceMap);
    double initialNotional = startingNotional.notional;
    double newNotional = initialNotional;

    for (int i = 0; i < path.legs.size(); ++i) {
        const auto& trade_leg = path.legs[i];
        const auto& leg_tick = pairToPriceMap.find(trade_leg.symbol)->second;
        double rate = getEffectiveRate(trade_leg, leg_tick, newNotional);

        // std::cout << "Update ID: " << leg_tick.updateId
        //           << ", Symbol: " << trade_leg.symbol
        //           << ", Rate VWAP: " << rate << "\n";

        if (rate <= 0){
            break;
        }

        newNotional = newNotional * rate;
    }

    // Subtract taker fees
    newNotional = newNotional * config.takerFee;

    double profit = newNotional - initialNotional;
    std::cout << std::fixed << std::setprecision(15) << profit << "\n"; // Example: 15 decimal places

    // Consider only large enough profits to protect against realtime slippages
    if (newNotional >= initialNotional * (1 + config.profitThreshold)) {
        std::cout << "Arbitrage opportunity detected!\n";
        std::cout << "Initial Traded Notional: " << initialNotional << "\n";
        std::cout << "Final Notional after Trades: " << newNotional << "\n";
        std::cout << "Profit: " << profit << "\n";

        currentNotional = newNotional;
        update.arbitrageOpportunity = true; 
    } 

    update.processTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::cout << "update process time: " << update.processTime - update.tickInitTime << " ms\n";

    update.unrealisedPnl = profit; 
    update.tradedNotional = initialNotional;
    update.bottleneckLeg = startingNotional.bottleneckLeg;

    if (tradeFileWriter){
        tradeFileWriter->write(update);
    }
}