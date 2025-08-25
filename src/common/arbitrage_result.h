#ifndef ARBITRAGE_RESULT_H
#define ARBITRAGE_RESULT_H

#include <string>
#include <vector>
#include <iostream>
#include <array>
#include "common/order_book.h"

struct ArbitrageResult {

    const std::string symbol;
    const std::string jsonStr;
    long long tickInitTime;
    const long long processTime; 
    const double unrealisedPnl; 
    const double tradedNotional; 
    const std::string bottleneckLeg; 
    const bool arbitrageOpportunity;
    const std::array<double, 3> rates; 

    ArbitrageResult(const std::string& sym, const std::string& json, long long ti, long long pt, double upnl, double tn, const std::string& bl, bool ao, const std::array<double, 3>& r)
        : symbol(sym), jsonStr(json), tickInitTime(ti), processTime(pt), unrealisedPnl(upnl), tradedNotional(tn), bottleneckLeg(bl), arbitrageOpportunity(ao), rates(r) {}

};

#endif // ARBITRAGE_RESULT_H