#include "file/trade_file_writer.h"
#include <fstream>
#include <string>
#include <iomanip>

TradeFileWriter::TradeFileWriter(const std::string& filePath)
    : file_stream_(filePath, std::ios::app)
{}

TradeFileWriter::~TradeFileWriter() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void TradeFileWriter::write(const OrderBookTick& tick){
    if (file_stream_.is_open()) {
        file_stream_ << tick.jsonStr << ","
                     << tick.tickInitTime << ","
                     << tick.processTime << ","
                     << std::fixed << std::setprecision(15)
                     << tick.tradedNotional << ","
                     << std::fixed << std::setprecision(15)
                     << tick.unrealisedPnl << ","
                     << tick.bottleneckLeg << ","
                     << (tick.arbitrageOpportunity ? "true" : "false") << "\n";
    }
}