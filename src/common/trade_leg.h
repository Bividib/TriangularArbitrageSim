#ifndef TRADE_LEG_H
#define TRADE_LEG_H

#include <string>
#include <iostream> 
#include <sstream>
#include <array>
#include <algorithm> 
#include "common/order_book.h" 

struct TradeLeg {
    std::string symbol;         // The actual trading pair symbol (e.g., "BTCUSDT")
    bool requiresInversion;     // True if we need to use 1.0 / tick.askPrice, False if we use tick.bidPrice

    TradeLeg(const std::string& sym, bool flip)
        : symbol(sym), requiresInversion(flip) {}
};

struct ArbitragePath {
    std::string startCurrency;
    std::array<TradeLeg, 3> legs; 

    ArbitragePath(const std::string& startCur,
                  const TradeLeg& leg1,
                  const TradeLeg& leg2,
                  const TradeLeg& leg3)
        : startCurrency(std::move(startCur)),
          legs{leg1, leg2, leg3} 
    {}

    ArbitragePath(std::string startCur, const std::vector<TradeLeg>& trade_legs)
        : startCurrency(std::move(startCur)),
          legs{{trade_legs[0],trade_legs[1],trade_legs[2]}}
    {}

    const TradeLeg& getFirstLeg() const { return legs[0]; }
    const TradeLeg& getSecondLeg() const { return legs[1]; }
    const TradeLeg& getThirdLeg() const { return legs[2]; }

    /**
     * Parses a string representation of an arbitrage path, there must only be 3 legs
     * 
     * An example valid String:
     * btc:btcusdt:BUY,ethusdt:SELL,ethbtc:BUY
     * 
     * First Value is the starting currency 
     * The next 3 values are the trade legs representing the currency pair and the buy/sell direction
     */
    static ArbitragePath from_string(const std::string& str) {
        size_t first_colon = str.find(':');
        if (first_colon == std::string::npos) {
            throw std::invalid_argument("Invalid format: missing base asset delimiter ':'");
        }

        std::string base = str.substr(0, first_colon);
        std::string legs_str = str.substr(first_colon + 1);

        std::vector<TradeLeg> parsed_legs;
        std::stringstream ss(legs_str);
        std::string leg_segment;

        while (std::getline(ss, leg_segment, ',')) {
            if (leg_segment.empty()) continue;

            size_t last_colon = leg_segment.rfind(':');
            if (last_colon == std::string::npos || last_colon == 0) {
                throw std::invalid_argument("Invalid leg format: " + leg_segment);
            }

            std::string symbol = leg_segment.substr(0, last_colon);
            std::string action = leg_segment.substr(last_colon + 1);

            bool is_sell;
            if (action == "SELL") {
                is_sell = true;
            } else if (action == "BUY") {
                is_sell = false;
            } else {
                throw std::invalid_argument("Invalid action in leg: " + action);
            }

            parsed_legs.emplace_back(symbol, is_sell);
        }

        return ArbitragePath(base, parsed_legs);
    }
};

#endif // TRADE_LEG_H

