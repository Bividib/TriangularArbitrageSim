#include "arbitrage_calculator.h"
#include <limits>

StartingNotional calculateStartingNotional(const ArbitragePath& path, const std::unordered_map<std::string, OrderBookTick>& pairToPriceMap) {
  
    auto calculateBookSideValue = 
        [](const std::vector<PriceLevel>& levels, bool sumBaseQuantity) -> double {
        double totalValue = 0.0;
        if (sumBaseQuantity) { // Sum the base currency quantity
            for (const auto& level : levels) {
                totalValue += level.quantity;
            }
        } else { // Sum the quote currency value (price * quantity)
            for (const auto& level : levels) {
                totalValue += level.price * level.quantity;
            }
        }
        return totalValue;
    };

    // --- Leg 1: Calculate its value AND the data needed for Leg 2's conversion ---
    const auto& leg1 = path.getFirstLeg();
    const auto& tick1 = pairToPriceMap.at(leg1.symbol);
    const auto& levels1 = leg1.requiresInversion ? tick1.asks : tick1.bids;

    // Calculate all required values from Leg 1 in a single pass to avoid redundancy.
    double totalQuoteValueLeg1 = 0.0;
    double totalBaseQuantityLeg1 = 0.0;
    for (const auto& level : levels1) {
        totalQuoteValueLeg1 += level.price * level.quantity;
        totalBaseQuantityLeg1 += level.quantity;
    }

    // Determine the final value for Leg 1 based on its inversion flag.
    const double firstLegValue = leg1.requiresInversion ? totalQuoteValueLeg1 : totalBaseQuantityLeg1;
    const StartingNotional leg1StartingNotional = {firstLegValue, leg1.symbol};

    // --- Leg 2: Calculate its value and convert it using Leg 1's data ---
    double secondLegValue = 0.0;
    const auto& leg2 = path.getSecondLeg();
    const auto& tick2 = pairToPriceMap.at(leg2.symbol);
    const auto& levels2 = leg2.requiresInversion ? tick2.asks : tick2.bids;

    const double secondLegValueIntermediate = calculateBookSideValue(levels2, !leg2.requiresInversion);
    const double effectivePriceLeg1 = totalQuoteValueLeg1 / totalBaseQuantityLeg1;

    secondLegValue = leg1.requiresInversion ? secondLegValueIntermediate * effectivePriceLeg1 : secondLegValueIntermediate / effectivePriceLeg1;

    const StartingNotional leg2StartingNotional = {secondLegValue, leg2.symbol};

    // --- Leg 3: Opposite to Leg1's Calculation --- 
    const auto& leg3 = path.getThirdLeg();
    const auto& tick3 = pairToPriceMap.at(leg3.symbol);
    const auto& levels3 = leg3.requiresInversion ? tick3.asks : tick3.bids;
    const double thirdLegValue = calculateBookSideValue(levels3, leg3.requiresInversion);

    const StartingNotional leg3StartingNotional = {thirdLegValue, leg3.symbol};

    // std::cout << "leg1 starting notional is " << leg1StartingNotional.notional << "\n";
    // std::cout << "leg2 starting notional is " << leg2StartingNotional.notional << "\n";
    // std::cout << "leg3 starting notional is " << leg3StartingNotional.notional << "\n";

    return std::min({leg1StartingNotional, leg2StartingNotional, leg3StartingNotional});
}

StartingNotional calculateStartingNotionalWithFirstLevelOnly(const ArbitragePath& path, const std::unordered_map<std::string, OrderBookTick>& pairToPriceMap){

    // Get the first leg's tick data
    const auto& leg1 = path.getFirstLeg();
    const auto& tick1 = pairToPriceMap.at(leg1.symbol);
    const double firstLegValue = leg1.requiresInversion ? tick1.getBestAskQty() * tick1.getBestAskPrice() : tick1.getBestBidQty();
    const StartingNotional leg1StartingNotional = {firstLegValue, leg1.symbol};

    const auto& leg2 = path.getSecondLeg();
    const auto& tick2 = pairToPriceMap.at(leg2.symbol);
    const double secondLegIntermediaryValue = leg2.requiresInversion ? tick2.getBestAskQty() * tick2.getBestAskPrice() : tick2.getBestBidQty();
    const double secondLegValue = leg1.requiresInversion ? secondLegIntermediaryValue * tick1.getBestAskPrice() : secondLegIntermediaryValue / tick1.getBestBidPrice();
    const StartingNotional leg2StartingNotional = {secondLegValue, leg2.symbol};

    const auto& leg3 = path.getThirdLeg();
    const auto& tick3 = pairToPriceMap.at(leg3.symbol);
    const double thirdLegValue = leg3.requiresInversion ? tick3.getBestAskQty() : tick3.getBestBidQty() * tick3.getBestBidPrice();
    const StartingNotional leg3StartingNotional = {thirdLegValue, leg3.symbol};

    // std::cout << "leg1 starting notional is " << leg1StartingNotional.notional << "\n";
    // std::cout << "leg2 starting notional is " << leg2StartingNotional.notional << "\n";
    // std::cout << "leg3 starting notional is " << leg3StartingNotional.notional << "\n";

    return std::min({leg1StartingNotional, leg2StartingNotional, leg3StartingNotional});
}

double getEffectiveRate(const TradeLeg& leg,const OrderBookTick& tick, double current_notional_in_previous_leg_currency) {
        if (current_notional_in_previous_leg_currency <= 0 || tick.bids.empty() || tick.asks.empty()) {
            return 0.0;
        }

        double rate = 0.0;

        if (leg.requiresInversion) { 
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

double calculateVwapBid(const std::vector<PriceLevel>& levels, double desired_quantity) {
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

    const double EPSILON = std::numeric_limits<double>::epsilon() * desired_quantity;

    if ((desired_quantity - total_quantity_filled) > EPSILON || total_quantity_filled <= 0) {
        // std::cerr << "Warning: Insufficient liquidity. Desired: " << desired_quantity 
        //         << ", Filled: " << total_quantity_filled << ". Cannot fulfill trade.\n";
        return 0.0;
    }

    return total_price_x_quantity / total_quantity_filled;
}

double calculateVwapAsk(const std::vector<PriceLevel>& levels, double old_currency) {
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

    if (total_eth_acquired > 0.0 && remaining_usdt_to_spend <= EPSILON) {
        // The average price is the total USDT spent divided by the total ETH acquired.
        return usdt_spent_actual / total_eth_acquired;
    } 

    // std::cerr << "Warning: Trade not fully executed due to insufficient liquidity. "
    //     << "Desired : " << old_currency
    //     << ", Actually spent: " << usdt_spent_actual
    //     << ", Remaining : " << remaining_usdt_to_spend << "\n";

    return 0.0;
}