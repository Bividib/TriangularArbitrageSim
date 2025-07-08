#ifndef TRADE_LEG_H
#define TRADE_LEG_H

#include <string>
#include <iostream> 
#include <array>
#include <algorithm> 
#include "common/common.h" 

struct TradeLeg {
    std::string symbol;         // The actual trading pair symbol (e.g., "BTCUSDT")
    bool requiresInversion;     // True if we need to use 1.0 / tick.askPrice, False if we use tick.bidPrice

    TradeLeg(const std::string& sym, bool flip)
        : symbol(sym), requiresInversion(flip) {}

    static double calculateVwap(const std::vector<PriceLevel>& levels, double desired_quantity) {
        if (desired_quantity <= 0) {
            return 0.0; 
        }

        double total_price_x_quantity = 0.0;
        double total_quantity_filled = 0.0;
        double remaining_quantity_to_fill = desired_quantity;

        for (const auto& level : levels) {
            if (remaining_quantity_to_fill <= 0) {
                break; 
            }

            double fill_quantity = std::min(level.quantity, remaining_quantity_to_fill);

            total_price_x_quantity += (level.price * fill_quantity);
            total_quantity_filled += fill_quantity;
            remaining_quantity_to_fill -= fill_quantity;
        }

        if (total_quantity_filled < desired_quantity) {
            std::cerr << "Warning: Insufficient liquidity. Desired: " << desired_quantity 
                    << ", Filled: " << total_quantity_filled << ". Cannot fulfill trade.\n";
            return 0.0;
        }

        if (total_quantity_filled <= 0) {
            return 0.0;
        }

        return total_price_x_quantity / total_quantity_filled;
    }

    double getEffectiveRate(const OrderBookTick& tick, double trade_size_for_this_leg) const {
        if (trade_size_for_this_leg <= 0) {
            return 0.0; 
        }

        double rate = 0.0;
        if (requiresInversion) {
            rate = calculateVwap(tick.asks, trade_size_for_this_leg);
            
            if (rate > 0) { 
                return 1.0 / rate;
            } else {
                std::cerr << "Warning: VWAP ask price is zero or negative for " << symbol << ". Cannot calculate rate." << std::endl;
                return 0.0;
            }
        } else {
            rate = calculateVwap(tick.bids, trade_size_for_this_leg);
            
            if (rate <= 0) {
                 std::cerr << "Warning: VWAP bid price is zero or negative for " << symbol << ". Cannot calculate rate." << std::endl;
                 return 0.0;
            }
            return rate;
        }
    }
};

struct ArbitragePath {
    std::string startCurrency;
    std::array<TradeLeg, 3> legs; 

    ArbitragePath(const std::string& startCur,
                  const TradeLeg& leg1,
                  const TradeLeg& leg2,
                  const TradeLeg& leg3)
        : startCurrency(startCur),
          legs{leg1, leg2, leg3} 
    {}
};

#endif // TRADE_LEG_H

