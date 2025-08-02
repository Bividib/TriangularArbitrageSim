#include "gtest/gtest.h"
#include "common/trade_leg.h"
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

//start with 1179.9228 USDT and convert to ETH 
TEST_F(TradeLegTest, CalculateVwapAskFirstLevel) {
    TradeLeg leg("ethusdt", true);
    double rate = leg.calculateVwapAsk(ethusdt_tick->asks, 1179.9228);
    EXPECT_DOUBLE_EQ(rate, 3742.12); 
}

//start with (125.1815 * 3742.12) + 100 (468544.19478) USDT
TEST_F(TradeLegTest, CalculateVwapAskSecondLevel) {
    TradeLeg leg("ethusdt", true);
    double rate = leg.calculateVwapAsk(ethusdt_tick->asks, 468544.19478);

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
    double rate = leg.calculateVwapAsk(ethusdt_tick->asks, usdt_to_spend);
    EXPECT_DOUBLE_EQ(rate, usdt_to_spend / eth_available); 
}

//start with USDT that will buy all ETH exactly, but only at the first level
TEST_F(TradeLegTest, TradeLegTest_CalculateVwapAskExactLiquidityFirstLevel){
    TradeLeg leg("ethusdt", true);
    double rate = leg.calculateVwapAsk(ethusdt_tick->asks, 125.1815 * 3742.12);
    EXPECT_DOUBLE_EQ(rate, 3742.12); 
}

//start with an absurd amount of USDT that cannot buy any ETH
TEST_F(TradeLegTest, CalculateVwapAskNoLiquidity) {
    TradeLeg leg("ethusdt", true);
    double rate = leg.calculateVwapAsk(ethusdt_tick->asks, 100000000);
    EXPECT_DOUBLE_EQ(rate, 0);
}

TEST_F(TradeLegTest, GetEffectiveRateAsk) {
    TradeLeg leg("ethusdt", true);
    double rate = leg.getEffectiveRate(*ethusdt_tick, 1179.9228);
    EXPECT_DOUBLE_EQ(rate, 1/3742.12); 
}

TEST_F(TradeLegTest, CalculateVwapExactLiquidityBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwapBid(tick->bids, 1.5);
    EXPECT_DOUBLE_EQ(vwap, 99.0);
}

TEST_F(TradeLegTest, CalculateVwapPartialLiquidityWithinFirstLevelBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwapBid(tick->bids, 1.0); 
    EXPECT_DOUBLE_EQ(vwap, 99.0);
}

TEST_F(TradeLegTest, CalculateVwapPartialLiquidityWithinSecondLevelBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwapBid(tick->bids, 2.0); 
    EXPECT_DOUBLE_EQ(vwap, 98.75);
}

TEST_F(TradeLegTest, CalculateVwapMaxLiquidityBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwapBid(tick->bids, 7.5); 
    EXPECT_NEAR(vwap, (99.0 * 1.5 + 98.0 * 2.5 + 97.0 * 3.5) / 7.5, 1e-9);
}

TEST_F(TradeLegTest, CalculateVwapInsufficientLiquidityBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwapBid(tick->bids, 10.0); 
    EXPECT_DOUBLE_EQ(vwap, 0.0); 
}

TEST_F(TradeLegTest, CalculateVwapZeroQuantity) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwapAsk(tick->bids, 0.0);
    EXPECT_DOUBLE_EQ(vwap, 0.0); 
}

TEST_F(TradeLegTest, CalculateVwapNegativeQuantity) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwapAsk(tick->bids, -5.0);
    EXPECT_DOUBLE_EQ(vwap, 0.0); 
}

TEST_F(TradeLegTest, GetEffectiveRateBid) {
    TradeLeg leg("btcusdt", false);
    double rate = leg.getEffectiveRate(*tick, 1.0);
    EXPECT_DOUBLE_EQ(rate, 99.0);
}

TEST_F(TradeLegTest, GetEffectiveRateWithinSecondLevelBid) {
    TradeLeg leg("btcusdt", false);
    double rate = leg.getEffectiveRate(*tick, 2.0);
    EXPECT_DOUBLE_EQ(rate, 98.75);
}

TEST_F(TradeLegTest, GetEffectiveRateZeroTradeSize) {
    TradeLeg leg("btcusdt", false);
    double rate = leg.getEffectiveRate(*tick, 0.0);
    EXPECT_DOUBLE_EQ(rate, 0.0);
}

TEST_F(TradeLegTest, GetEffectiveRateNegativeTradeSize) {
    TradeLeg leg("btcusdt", false);
    double rate = leg.getEffectiveRate(*tick, -1.0);
    EXPECT_DOUBLE_EQ(rate, 0.0);
}