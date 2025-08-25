#include "gtest/gtest.h"
#include "common/trade_leg.h"
#include "server/arbitrage_calculator.h"
#include <memory> 

class TradeLegTest : public ::testing::Test {
protected:
    std::unique_ptr<OrderBookTick> tick; 

    std::unique_ptr<OrderBookTick> ethusdt_tick; 

    void SetUp() override {
        tick = std::make_unique<OrderBookTick>(); 

        // btcusdt -> ethusdt -> ethbtc 

        tick->symbol = "btcusdt";
        tick->asks = {
            PriceLevel(100.0, 1.0),
            PriceLevel(101.0, 2.0),
            PriceLevel(102.0, 3.0)
        };
        tick->bids = {
            PriceLevel(99.0, 1.5),
            PriceLevel(98.0, 2.5),
            PriceLevel(97.0, 3.5)
        };
        tick->updateId = 123456789;
        tick->tickInitTime = 1609459200000; 

        ethusdt_tick = std::make_unique<OrderBookTick>();
        ethusdt_tick->symbol = "ethusdt";

        ethusdt_tick->bids = {
            PriceLevel(3742.11, 55.3849),
            PriceLevel(3742.10, 0.0015),
            PriceLevel(3742.09, 0.0015),
            PriceLevel(3742.08, 0.0015),
            PriceLevel(3742.07, 0.0015)
        };
        ethusdt_tick->asks = {
            PriceLevel(3742.12, 125.1815),
            PriceLevel(3742.13, 0.3118),
            PriceLevel(3742.14, 0.003),
            PriceLevel(3742.15, 0.5514),
            PriceLevel(3742.16, 0.0015)
        };
    }
};

/*
* Test fixture for StartingNotional calculations
*/
class StartingNotionalTest : public ::testing::Test {
protected:
    ArbitragePath path = ArbitragePath::from_string("btc:btcusdt:BUY,ethusdt:SELL,ethbtc:BUY");
    std::unordered_map<std::string, OrderBookTick> pairToPriceMap;

    void SetUp() override {
        // create btc usdt tick based on the example JSON above 

        OrderBookTick btcusdt_tick;
        btcusdt_tick.bids = {
            PriceLevel(117992.29000000, 5.61816000),
            PriceLevel(117992.30000000, 0.00433000),
            PriceLevel(117992.36000000, 0.00010000),
            PriceLevel(117992.37000000, 0.05095000),
            PriceLevel(117992.43000000, 0.00010000)
        };
        btcusdt_tick.asks = {
            PriceLevel(117992.44000000, 0.00010000),
            PriceLevel(117992.45000000, 0.05095000),
            PriceLevel(117992.46000000, 0.00010000),
            PriceLevel(117992.47000000, 0.00433000),
            PriceLevel(117992.48000000, 5.61816000)
        };

        OrderBookTick ethusdt_tick;
        ethusdt_tick.bids = {
            PriceLevel(3742.11000000, 55.38490000),
            PriceLevel(3742.10000000, 0.00150000),
            PriceLevel(3742.09000000, 0.00150000),
            PriceLevel(3742.08000000, 0.00150000),
            PriceLevel(3742.07000000, 0.00150000)
        };
        ethusdt_tick.asks = {
            PriceLevel(3742.12000000, 125.18150000),
            PriceLevel(3742.13000000, 0.31180000),
            PriceLevel(3742.14000000, 0.00300000),
            PriceLevel(3742.15000000, 0.55140000),
            PriceLevel(3742.16000000, 0.00150000)
        };

        OrderBookTick ethbtc_tick;
        ethbtc_tick.bids = {
            PriceLevel(0.03171000, 23.57890000),
            PriceLevel(0.03170000, 58.66880000),
            PriceLevel(0.03169000, 42.05050000),
            PriceLevel(0.03168000, 52.23000000),
            PriceLevel(0.03167000, 58.83160000)
        };
        ethbtc_tick.asks = {
            PriceLevel(0.03172000, 15.37580000),
            PriceLevel(0.03173000, 29.79230000),
            PriceLevel(0.03174000, 55.22210000),
            PriceLevel(0.03175000, 40.73120000),
            PriceLevel(0.03176000, 54.27910000)
        };

        pairToPriceMap = {
            {"btcusdt", btcusdt_tick},
            {"ethusdt", ethusdt_tick},
            {"ethbtc", ethbtc_tick}
        };
    }
};

//start with 1179.9228 USDT and convert to ETH 
TEST_F(TradeLegTest, CalculateVwapAskFirstLevel) {
    TradeLeg leg("ethusdt", true);
    double rate = calculateVwapAsk(ethusdt_tick->asks, 1179.9228);
    EXPECT_DOUBLE_EQ(rate, 3742.12); 
}

//start with (125.1815 * 3742.12) + 100 (468544.19478) USDT
TEST_F(TradeLegTest, CalculateVwapAskSecondLevel) {
    TradeLeg leg("ethusdt", true);
    double rate = calculateVwapAsk(ethusdt_tick->asks, 468544.19478);

    double eth_from_second_level = 100 / 3742.13;
    EXPECT_DOUBLE_EQ(rate, 468544.19478 / (125.1815 + eth_from_second_level)); 
}

//start with USDT that will buy all ETH exactly
TEST_F(TradeLegTest, CalculateVwapAskMaxLiquidity) {
    TradeLeg leg("ethusdt", true);
    double usdt_to_spend = 0; 
    double eth_available = 0;
    for (const auto& level : ethusdt_tick->asks) {
        usdt_to_spend += level.price * level.quantity;
        eth_available += level.quantity;
    }
    double rate = calculateVwapAsk(ethusdt_tick->asks, usdt_to_spend);
    EXPECT_DOUBLE_EQ(rate, usdt_to_spend / eth_available);
}

//start with USDT that will buy all ETH exactly, but only at the first level
TEST_F(TradeLegTest, TradeLegTest_CalculateVwapAskExactLiquidityFirstLevel){
    TradeLeg leg("ethusdt", true);
    double rate = calculateVwapAsk(ethusdt_tick->asks, 125.1815 * 3742.12);
    EXPECT_DOUBLE_EQ(rate, 3742.12); 
}

//start with an absurd amount of USDT that cannot buy any ETH
TEST_F(TradeLegTest, CalculateVwapAskNoLiquidity) {
    TradeLeg leg("ethusdt", true);
    double rate = calculateVwapAsk(ethusdt_tick->asks, 100000000);
    EXPECT_DOUBLE_EQ(rate, 0);
}

TEST_F(TradeLegTest, GetEffectiveRateAsk) {
    TradeLeg leg("ethusdt", true);
    double rate = getEffectiveRate(leg, *ethusdt_tick, 1179.9228);
    EXPECT_DOUBLE_EQ(rate, 1/3742.12); 
}

TEST_F(TradeLegTest, CalculateVwapExactLiquidityBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = calculateVwapBid(tick->bids, 1.5);
    EXPECT_DOUBLE_EQ(vwap, 99.0);
}

TEST_F(TradeLegTest, CalculateVwapPartialLiquidityWithinFirstLevelBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = calculateVwapBid(tick->bids, 1.0); 
    EXPECT_DOUBLE_EQ(vwap, 99.0);
}

TEST_F(TradeLegTest, CalculateVwapPartialLiquidityWithinSecondLevelBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = calculateVwapBid(tick->bids, 2.0); 
    EXPECT_DOUBLE_EQ(vwap, 98.75);
}

TEST_F(TradeLegTest, CalculateVwapMaxLiquidityBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = calculateVwapBid(tick->bids, 7.5); 
    EXPECT_NEAR(vwap, (99.0 * 1.5 + 98.0 * 2.5 + 97.0 * 3.5) / 7.5, 1e-9);
}

TEST_F(TradeLegTest, CalculateVwapInsufficientLiquidityBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = calculateVwapBid(tick->bids, 10.0); 
    EXPECT_DOUBLE_EQ(vwap, 0.0); 
}

TEST_F(TradeLegTest, CalculateVwapZeroQuantity) {
    TradeLeg leg("btcusdt", false);
    double vwap = calculateVwapAsk(tick->bids, 0.0);
    EXPECT_DOUBLE_EQ(vwap, 0.0); 
}

TEST_F(TradeLegTest, CalculateVwapNegativeQuantity) {
    TradeLeg leg("btcusdt", false);
    double vwap = calculateVwapAsk(tick->bids, -5.0);
    EXPECT_DOUBLE_EQ(vwap, 0.0); 
}

TEST_F(TradeLegTest, GetEffectiveRateBid) {
    TradeLeg leg("btcusdt", false);
    double rate = getEffectiveRate(leg,*tick, 1.0);
    EXPECT_DOUBLE_EQ(rate, 99.0);
}

TEST_F(TradeLegTest, GetEffectiveRateWithinSecondLevelBid) {
    TradeLeg leg("btcusdt", false);
    double rate = getEffectiveRate(leg,*tick, 2.0);
    EXPECT_DOUBLE_EQ(rate, 98.75);
}

TEST_F(TradeLegTest, GetEffectiveRateZeroTradeSize) {
    TradeLeg leg("btcusdt", false);
    double rate = getEffectiveRate(leg,*tick, 0.0);
    EXPECT_DOUBLE_EQ(rate, 0.0);
}

TEST_F(TradeLegTest, GetEffectiveRateNegativeTradeSize) {
    TradeLeg leg("btcusdt", false);
    double rate = getEffectiveRate(leg,*tick, -1.0);
    EXPECT_DOUBLE_EQ(rate, 0.0);
}

TEST_F(StartingNotionalTest, CalculateStartingNotional) {
    StartingNotional expected = {3.9976446695521055, "ethusdt"};
    StartingNotional actual = calculateStartingNotional(path,pairToPriceMap);
    
    EXPECT_DOUBLE_EQ(actual.notional, expected.notional);
    EXPECT_EQ(actual.bottleneckLeg, expected.bottleneckLeg);
}

TEST_F(StartingNotionalTest, CalculateStartingNotionalWithFirstLevelOnly) {
    StartingNotional expected = {0.03171000 * 23.57890000, "ethbtc"};
    StartingNotional actual = calculateStartingNotionalWithFirstLevelOnly(path,pairToPriceMap);

    EXPECT_DOUBLE_EQ(actual.notional, expected.notional);
    EXPECT_EQ(actual.bottleneckLeg, expected.bottleneckLeg);

}

TEST_F(StartingNotionalTest, BottleneckIsLeg1) {
    // To make leg1 the bottleneck, we reduce its available quantity.
    pairToPriceMap["btcusdt"].bids[0] = PriceLevel(117992.29000000, 0.0001);

    StartingNotional expected = {0.0001, "btcusdt"};
    StartingNotional actual = calculateStartingNotionalWithFirstLevelOnly(path, pairToPriceMap);

    EXPECT_DOUBLE_EQ(actual.notional, expected.notional);
    EXPECT_EQ(actual.bottleneckLeg, expected.bottleneckLeg);
}

TEST_F(StartingNotionalTest, BottleneckIsLeg2) {
    pairToPriceMap["ethusdt"].asks[0] = PriceLevel(3742.11000000, 0.000001);
    
    const double expected_notional = (3742.11000000 * 0.000001) / pairToPriceMap.at("btcusdt").getBestBidPrice();
    StartingNotional expected = {expected_notional, "ethusdt"};
    StartingNotional actual = calculateStartingNotionalWithFirstLevelOnly(path, pairToPriceMap);

    EXPECT_DOUBLE_EQ(actual.notional, expected.notional);
    EXPECT_EQ(actual.bottleneckLeg, expected.bottleneckLeg);
}
