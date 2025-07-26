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

    static double calculateVwapBid(const std::vector<PriceLevel>& levels, double desired_quantity) {
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

        if (total_quantity_filled < desired_quantity || total_quantity_filled <= 0) {
            // std::cerr << "Warning: Insufficient liquidity. Desired: " << desired_quantity 
            //         << ", Filled: " << total_quantity_filled << ". Cannot fulfill trade.\n";
            return 0.0;
        }

        return total_price_x_quantity / total_quantity_filled;
    }

    static double calculateVwapAsk(const std::vector<PriceLevel>& levels, double old_currency) {
        if (old_currency <= 0.0) {
            return 0.0;
        }

        const double EPSILON = std::numeric_limits<double>::epsilon() * old_currency;

        double total_eth_acquired = 0.0;
        double usdt_spent_actual = 0.0; 
        double remaining_usdt_to_spend = old_currency;

        for (const auto& level : levels) {
            double price = level.price;
            double available_eth_at_level = level.quantity;


            double cost_to_buy_all_at_level = price * available_eth_at_level;

            if (remaining_usdt_to_spend >= cost_to_buy_all_at_level) {
                // Buy all the ETH at this level
                total_eth_acquired += available_eth_at_level;
                usdt_spent_actual += cost_to_buy_all_at_level;
                remaining_usdt_to_spend -= cost_to_buy_all_at_level;
            } else {
                // Buy only a portion of ETH with the remaining USDT
                // This is where the division of (remaining_usdt / price) happens
                double eth_to_buy_with_remaining_usdt = remaining_usdt_to_spend / price;

                total_eth_acquired += eth_to_buy_with_remaining_usdt;
                usdt_spent_actual += remaining_usdt_to_spend; // All remaining USDT is spent here
                remaining_usdt_to_spend = 0.0; // No USDT left

                break; // All funds spent, stop processing further levels
            }

            // If all old_currency has been spent (or very close to it due to precision), break
            if (remaining_usdt_to_spend <= EPSILON) {
                remaining_usdt_to_spend = 0.0; 
                break;
            }
        }

        if (total_eth_acquired > 0.0 && remaining_usdt_to_spend == 0.0) {
            // The average price is the total USDT spent divided by the total ETH acquired.
            return usdt_spent_actual / total_eth_acquired;
        } 

        std::cerr << "Warning: Trade not fully executed due to insufficient ETH liquidity. "
            << "Desired USDT: " << old_currency
            << ", Actually spent: " << usdt_spent_actual
            << ", Remaining USDT: " << remaining_usdt_to_spend << "\n";

        return 0.0;
    }

    double TradeLeg::getEffectiveRate(const OrderBookTick& tick, double current_notional_in_previous_leg_currency) const {
        if (current_notional_in_previous_leg_currency <= 0 || tick.bids.empty() || tick.asks.empty()) {
            return 0.0;
        }

        double rate = 0.0;

        if (requiresInversion) { 
            double vwap_price_quote_per_base = calculateVwapAsk(tick.asks, current_notional_in_previous_leg_currency);
            
            if (vwap_price_quote_per_base > 0) {
                return 1.0 / vwap_price_quote_per_base;
            } else {
                return 0.0;
            }
        } else { 
            double vwap_price_quote_per_base = calculateVwapBid(tick.bids, current_notional_in_previous_leg_currency);

            if (vwap_price_quote_per_base > 0) {
                return vwap_price_quote_per_base;
            } else {
                return 0.0;
            }
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

