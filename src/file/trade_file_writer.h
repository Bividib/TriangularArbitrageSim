#pragma once

#include <string>
#include <fstream>
#include "common/order_book.h"

class TradeFileWriter {
public:
    // Constructor takes file path, opens file in append mode
    explicit TradeFileWriter(const std::string& file_path);

    /**
     * Writes an OrderBookTick to a text file
     * Inserts the following comma separated details per tick on a new line: 
     * 
     * - JSON string representation of the tick
     * - Time the tick was created 
     * - Time the server took to process the tick 
     * - Unrealised p&l
     * - Traded Notional
     * - Bottleneck Leg Identifier (minimum of the 3 for getting started notional)
     * 
     * @param tick The OrderBookTick to write to the file
     */
    void write(const OrderBookTick& tick);

    // Destructor closes the file
    ~TradeFileWriter();

private:
    std::ofstream file_stream_;
};