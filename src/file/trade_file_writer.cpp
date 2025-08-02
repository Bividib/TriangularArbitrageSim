#include "file/trade_file_writer.h"
#include <fstream>
#include <string>

TradeFileWriter::TradeFileWriter(const std::string& filePath)
    : file_stream_(filePath, std::ios::app)
{}

TradeFileWriter::~TradeFileWriter() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

    /**
     * Writes an OrderBookTick to a text file
     * Inserts the following comma separated details per tick on a new line: 
     * 
     * - JSON string representation of the tick
     * - Time the tick was created 
     * - Time the server took to process the tick 
     * - Unrealised p&l without taker fees
     * 
     * @param tick The OrderBookTick to write to the file
     */
void TradeFileWriter::write(const OrderBookTick& tick){
    if (file_stream_.is_open()) {
        file_stream_ << tick.jsonStr << ","
                     << tick.tickInitTime << ","
                     << tick.processTime << ","
                     << tick.unrealisedPnl << "\n";
    }
}