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
/*
    An example of the different legs and their relationships:
    
    BTCUSDT USDT/BTC Leg1-Bid
    ETHUSDT USDT     Leg2-Ask
    Need to recognise to divide here 

    BTCUSDT USDT/BTC  Leg1-Bid
    USDTETH USDT      Leg2-Bid
    Need to recognise to divide here

    BTCUSDT USDT/BTC Leg1-Ask
    LTCBTC  BTC      Leg2-Ask
    Need to recognise to multiply here

    BTCUSDT USDT/BTC Leg1-Ask
    BTCLTC  BTC      Leg2-Bid
    Need to recognise to multiply here

    General algorithm for valuing Leg2 in terms of Leg1
    If Leg2 requires inversion (Ask) then calculate sum of Ask quantity * price 
    Otherwise Leg2 doesn't need inversion (Bid) then calculate sum of Bid quantities only
    
    If Leg1 is Bid then divide by Leg1's Rate
    Otherwise Leg1 is Ask so multiply by Leg1's Rate
 */
StartingNotional calculateStartingNotional(const ArbitragePath& path, 
                                 const std::unordered_map<std::string, OrderBookTick>& pairToPriceMap);

double getEffectiveRate(const TradeLeg& leg, const OrderBookTick& tick, double current_notional_in_previous_leg_currency);

double calculateVwapBid(const std::vector<PriceLevel>& levels, double desired_quantity);

double calculateVwapAsk(const std::vector<PriceLevel>& levels, double old_currency);
