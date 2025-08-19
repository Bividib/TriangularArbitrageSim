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

void TradeFileWriter::write(const OrderBookTick& tick) {
    if (file_stream_.is_open()) {
        nlohmann::json tick_json;
        tick_json["orderBookLevels"] = tick.jsonStr; 
        tick_json["tickReceiveTime"] = tick.tickInitTime;
        tick_json["tickProcessTime"] = tick.processTime;
        tick_json["tradedNotional"] =  tick.tradedNotional;
        tick_json["unrealisedPnl"] = tick.unrealisedPnl;
        tick_json["bottleneckLeg"] = tick.bottleneckLeg;
        tick_json["isArbitrageOpportunity"] = tick.arbitrageOpportunity;
        file_stream_ << std::fixed << std::setprecision(10) << tick_json.dump() << "\n";
    }
}