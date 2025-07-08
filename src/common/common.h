#ifndef COMMON_H
#define COMMON_H

#include <boost/beast/core/error.hpp>
#include <string>
#include <vector>
#include <iostream>

void fail(const boost::beast::error_code& ec, const char* category);

struct PriceLevel {
    double price;
    double quantity;

    PriceLevel(double p = 0.0, double q = 0.0) : price(p), quantity(q) {}
};

struct OrderBookTick {
    long long updateId;
    std::string symbol;
    
    std::vector<PriceLevel> bids; // Sorted from highest bid price to lowest
    std::vector<PriceLevel> asks; // Sorted from lowest ask price to highest

    long long localTimestampMs;

    OrderBookTick(long long u, 
                  std::string s, 
                  std::vector<PriceLevel> b, 
                  std::vector<PriceLevel> a, 
                  long long ts = 0)
        : updateId(u), 
          symbol(std::move(s)), 
          bids(std::move(b)), 
          asks(std::move(a)), 
          localTimestampMs(ts) {}


    double getBestBidPrice() const { 
        if (!bids.empty()) {
            return bids[0].price; 
        }
        return 0.0; 
    }

    double getBestBidQty() const { 
        if (!bids.empty()) {
            return bids[0].quantity;
        }
        return 0.0;
    }

    double getBestAskPrice() const { 
        if (!asks.empty()) {
            return asks[0].price; 
        }
        return 0.0;
    }

    double getBestAskQty() const { 
        if (!asks.empty()) {
            return asks[0].quantity;
        }
        return 0.0;
    }
};

#endif // COMMON_H