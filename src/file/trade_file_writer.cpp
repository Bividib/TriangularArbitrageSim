#include "file/trade_file_writer.h"
#include "common/trade_util.h"
#include <iomanip>

TradeFileWriter::TradeFileWriter(const std::string& filePath)
    : file_stream_(filePath, std::ios::app), filePath_(filePath)
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

        if (!file_stream_.good()) {
            const std::string error_message = "Error writing to file. Stream state: "
                      "badbit=" + std::to_string(file_stream_.bad()) +
                      " eofbit=" + std::to_string(file_stream_.eof()) +
                      " failbit=" + std::to_string(file_stream_.fail());
            fail("TradeFileWriter Error", error_message.c_str());
            file_stream_.clear();
            file_stream_.close();
            file_stream_.open(filePath_, std::ios::app);
        }
    }
}