//Add all imports
#include <vector>
#include <algorithm>
#include <common/order_book.h>
#include <common/trade_leg.h>

struct StartingNotional {
    double notional;
    std::string bottleneckLeg;

    bool operator<(const StartingNotional& other) const {
        // Comparison is based solely on the 'notional' value
        return this->notional < other.notional;
    }
};

StartingNotional calculateStartingNotional(const ArbitragePath& path, 
                                 const std::unordered_map<std::string, OrderBookTick>& pairToPriceMap);

double getEffectiveRate(const TradeLeg& leg, const OrderBookTick& tick, double current_notional_in_previous_leg_currency);

double calculateVwapBid(const std::vector<PriceLevel>& levels, double desired_quantity);

double calculateVwapAsk(const std::vector<PriceLevel>& levels, double old_currency);
