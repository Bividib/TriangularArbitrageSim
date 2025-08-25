#include "file/trade_file_writer.h"
#include <iomanip>

TradeFileWriter::TradeFileWriter(const std::string& filePath)
    : file_stream_(filePath, std::ios::app)
{}

TradeFileWriter::~TradeFileWriter() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void TradeFileWriter::write(const ArbitrageResult& result) {
    if (file_stream_.is_open()) {
        nlohmann::json tick_json;
        tick_json["orderBookLevels"] = result.jsonStr;
        tick_json["tickReceiveTime"] = result.tickInitTime;
        tick_json["tickProcessTime"] = result.processTime;
        tick_json["tradedNotional"] = result.tradedNotional;
        tick_json["unrealisedPnl"] = result.unrealisedPnl;
        tick_json["bottleneckLeg"] = result.bottleneckLeg;
        tick_json["isArbitrageOpportunity"] = result.arbitrageOpportunity;
        tick_json["rate1"] = result.rates[0];
        tick_json["rate2"] = result.rates[1];
        tick_json["rate3"] = result.rates[2];
        file_stream_ << std::fixed << std::setprecision(10) << tick_json.dump() << "\n";
    }
}