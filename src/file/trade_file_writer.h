#pragma once

#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

#include "common/arbitrage_result.h"

class TradeFileWriter {
public:
    // Constructor takes file path, opens file in append mode
    explicit TradeFileWriter(const std::string& file_path);

    /**
     * Writes an ArbitrageResult to a text file
     * Inserts the following comma separated details per tick on a new line: 
     * 
     * - JSON string representation of the tick
     * - Time the tick was created 
     * - Time the server took to process the tick 
     * - Unrealised p&l
     * - Traded Notional
     * - Bottleneck Leg Identifier (minimum of the 3 for getting started notional)
     * 
     * @param result The ArbitrageResult to write to the file
     */
    void write(const ArbitrageResult& result);

    // Destructor closes the file
    ~TradeFileWriter();

private:
    std::ofstream file_stream_;
    std::string filePath_;
};