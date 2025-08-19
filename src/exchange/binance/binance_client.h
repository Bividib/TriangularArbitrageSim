#ifndef BINANCE_CLIENT_H
#define BINANCE_CLIENT_H

#include "exchange/abstract/market_data_client.h"
#include <nlohmann/json.hpp>

/**
 * @class BinanceClient
 * @brief Concrete implementation for a Binance WebSocket client.
 *
 * Inherits from BaseClient and provides concrete implementations for all
 * pure virtual functions using the `override` specifier.
 */
class BinanceClient : public BaseClient, public std::enable_shared_from_this<BinanceClient> {
public:
    BinanceClient(boost::asio::io_context& ioc, boost::asio::ssl::context& ssl_ctx);

    void set_callback(const std::shared_ptr<Server>& server) override;
    void async_connect(const std::string& host, const std::string& port, const std::string& target) override;
private:
    // Use the 'override' keyword to ensure we are correctly implementing
    // the virtual functions from the base classes.
    void on_resolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results) override;
    void on_connect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type endpoint) override;
    void on_ssl_handshake(boost::beast::error_code ec) override;
    void on_ws_handshake(boost::beast::error_code ec) override;
    void on_close(boost::beast::error_code ec) override;
    
    void run_client() override;

    void read() override;
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred) override;

    static std::vector<PriceLevel> parsePriceLevels(const nlohmann::json& json_array);

    // Binance-specific implementation details
    OrderBookTick to_struct(const nlohmann::json& json_data, const std::string& json_string, long long localTimestampNs);
    boost::asio::ip::tcp::resolver resolver;
    static const std::string WS_CLIENT_HEADER;
};

#endif // BINANCE_CLIENT_H