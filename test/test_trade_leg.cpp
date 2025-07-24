#include "gtest/gtest.h"
#include "common/trade_leg.h"
#include <memory> 

class TradeLegTest : public ::testing::Test {
protected:
    std::unique_ptr<OrderBookTick> tick; 

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
        tick->localTimestampMs = 1609459200000; 
    }
};

TEST_F(TradeLegTest, CalculateVwapExactLiquidityBid) {
    TradeLeg leg("btcusdt", false);
    // Use the -> operator here as well
    double vwap = leg.calculateVwap(tick->bids, 1.5);
    EXPECT_DOUBLE_EQ(vwap, 99.0);
}

TEST_F(TradeLegTest, CalculateVwapPartialLiquidityWithinFirstLevelBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwap(tick->bids, 1.0); 
    EXPECT_DOUBLE_EQ(vwap, 99.0);
}

TEST_F(TradeLegTest, CalculateVwapPartialLiquidityWithinSecondLevelBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwap(tick->bids, 2.0); 
    EXPECT_DOUBLE_EQ(vwap, 98.75);
}

TEST_F(TradeLegTest, CalculateVwapMaxLiquidityBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwap(tick->bids, 7.5); 
    EXPECT_NEAR(vwap, (99.0 * 1.5 + 98.0 * 2.5 + 97.0 * 3.5) / 7.5, 1e-9);
}

TEST_F(TradeLegTest, CalculateVwapInsufficientLiquidityBid) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwap(tick->bids, 10.0); 
    EXPECT_DOUBLE_EQ(vwap, 0.0); 
}

TEST_F(TradeLegTest, CalculateVwapExactLiquidityAsk) {
    TradeLeg leg("btcusdt", true);
    double vwap = leg.calculateVwap(tick->asks, 1.0);
    EXPECT_DOUBLE_EQ(vwap, 100.0);
}

TEST_F(TradeLegTest, CalculateVwapPartialLiquidityWithinFirstLevelAsk) {
    TradeLeg leg("btcusdt", true);
    double vwap = leg.calculateVwap(tick->asks, 0.5);
    EXPECT_DOUBLE_EQ(vwap, 100.0);
}

TEST_F(TradeLegTest, CalculateVwapPartialLiquidityWithinSecondLevelAsk) {
    TradeLeg leg("btcusdt", true);
    double vwap = leg.calculateVwap(tick->asks, 1.5);
    EXPECT_NEAR(vwap,(100.0 * 2.0 + 101.0 * 1.0) / 3.0 , 1e-9); 
}

TEST_F(TradeLegTest, CalculateVwapMaxLiquidityAsk) {
    TradeLeg leg("btcusdt", true);
    double vwap = leg.calculateVwap(tick->asks, 6.0);
    EXPECT_NEAR(vwap, (100.0 * 1.0 + 101.0 * 2.0 + 102.0 * 3.0) / 6.0, 1e-9);
}

TEST_F(TradeLegTest, CalculateVwapInsufficientLiquidityAsk) {
    TradeLeg leg("btcusdt", true);
    double vwap = leg.calculateVwap(tick->asks, 10.0);
    EXPECT_DOUBLE_EQ(vwap, 0.0); 
}

TEST_F(TradeLegTest, CalculateVwapZeroQuantity) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwap(tick->bids, 0.0);
    EXPECT_DOUBLE_EQ(vwap, 0.0); 
}

TEST_F(TradeLegTest, CalculateVwapNegativeQuantity) {
    TradeLeg leg("btcusdt", false);
    double vwap = leg.calculateVwap(tick->bids, -5.0);
    EXPECT_DOUBLE_EQ(vwap, 0.0); 
}