#include "binance_client.h"
#include "common/trade_util.h"
#include <iostream>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>

// --- BaseClient / Client Implementation ---

void BaseClient::write_str_to_buf(const std::string& str, boost::beast::flat_buffer& fb) {
    auto mutable_buf = fb.prepare(str.length());
    size_t bytes_copied = boost::asio::buffer_copy(mutable_buf, boost::asio::buffer(str));
    fb.commit(bytes_copied);
}

// --- BinanceClient Implementation ---

const std::string BinanceClient::WS_CLIENT_HEADER = "TriangularArbitrageAsyncBinanceWsClient";

BinanceClient::BinanceClient(boost::asio::io_context& ioc, boost::asio::ssl::context& ssl_ctx) 
    : BaseClient(ioc, ssl_ctx), resolver(ioc) {
    ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));
    ws.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::request_type& req) {
            req.set(boost::beast::http::field::user_agent, WS_CLIENT_HEADER);
        }
    ));
    std::cout << "BinanceClient initialised" << "\n";
}

void BinanceClient::set_callback(const std::shared_ptr<Server>& server){
    callback = server;
}

void BinanceClient::async_connect(const std::string& host_in, const std::string& port_in, const std::string& target_in) {
    host = host_in;
    port = port_in;
    target = target_in;

    std::cout << "Connecting to Binance WebSocket at " << host << ":" << port << target << "\n";

    resolver.async_resolve(
        host,
        port,
        boost::beast::bind_front_handler(
            &BinanceClient::on_resolve,
            shared_from_this()
        )
    );
}

void BinanceClient::on_resolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
    if (ec) return fail(ec, "Resolve Endpoint");

    auto& lowest_layer = boost::beast::get_lowest_layer(ws);
    lowest_layer.expires_after(std::chrono::seconds(30));
    
    lowest_layer.async_connect(
        results,
        boost::beast::bind_front_handler(
            &BinanceClient::on_connect,
            shared_from_this()
        )
    );
}

void BinanceClient::on_connect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type) {
    if (ec) return fail(ec, "Client TCP Connect");

    std::cout << "TCP Connection established, starting SSL Handshake..." << "\n";
    boost::beast::get_lowest_layer(ws).expires_never();

    ws.next_layer().async_handshake(
        boost::asio::ssl::stream_base::client,
        boost::beast::bind_front_handler(
            &BinanceClient::on_ssl_handshake,
            shared_from_this()
        )
    );
}

void BinanceClient::on_ssl_handshake(boost::beast::error_code ec) {
    if (ec) return fail(ec, "Client SSL Handshake");

    std::cout << "SSL Handshake successful, starting WS Handshake..." << "\n";
    ws.async_handshake(
        host,
        target,
        boost::beast::bind_front_handler(
            &BinanceClient::on_ws_handshake,
            shared_from_this()
        )
    );
}

void BinanceClient::on_ws_handshake(boost::beast::error_code ec) {
    if (ec) return fail(ec, "Client WS Handshake");

    std::cout << "Client successfully connected to WebSocket" << "\n";
    run_client();
}

void BinanceClient::run_client() {
    std::cout << "Begin operating Binance Client..." << "\n";
    read();
}

void BinanceClient::read() {
    ws.async_read(buffer, boost::beast::bind_front_handler(
        &BinanceClient::on_read,
        shared_from_this()
    ));
}

void BinanceClient::on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec == boost::beast::websocket::error::closed) {
        std::cout << "WebSocket connection closed gracefully.\n";
        return;
    }
    if (ec){ 
        fail(ec, "read");
        std::cerr << "Restarting connection due to unexpected error ... " << std::endl;
        async_connect(host,port,target);
    }

    long long localTimestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // Process the message
    const std::string& json_string = boost::beast::buffers_to_string(buffer.data());
    // std::cout << "Received: " << json_string << std::endl;
    auto data = nlohmann::json::parse(json_string);
    auto tick_struct = to_struct(data,json_string,localTimestampNs);
    
    callback->on_update(tick_struct);
    
    buffer.consume(buffer.size());
    read(); // Listen for the next message
}

void BinanceClient::on_close(boost::beast::error_code ec) {
    if (ec) return fail(ec, "close");
    std::cout << "Client connection closed gracefully" << "\n";
}

std::vector<PriceLevel> BinanceClient::parsePriceLevels(const nlohmann::json& json_array) {
    std::vector<PriceLevel> levels_vec;

    for (const auto& level_json : json_array) {
        if (level_json.is_array() && level_json.size() == 2) {
            try {
                double price = std::stod(level_json.at(0).get<std::string>());
                double quantity = std::stod(level_json.at(1).get<std::string>());
                levels_vec.emplace_back(price, quantity);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing " << e.what() << "\n";
                throw std::runtime_error("Failed to parse price level: " + std::string(e.what()));
            }
        } else {
            std::cerr << "Invalid price level format: " << level_json.dump() << "\n";
            throw std::runtime_error("Invalid price level format in JSON data");
        }
    }
    return levels_vec;
}

OrderBookTick BinanceClient::to_struct(const nlohmann::json& json_data, const std::string& json_string, long long localTimestampNs) {
    try {
        const auto& data = json_data.at("data");
        const long long updateId = data.at("lastUpdateId").get<long long>(); 
        
        std::string full_stream_name = json_data.at("stream").get<std::string>();
        std::string symbol = full_stream_name.substr(0, full_stream_name.find('@'));

        std::vector<PriceLevel> bids_vec = parsePriceLevels(data.at("bids"));
        std::vector<PriceLevel> asks_vec = parsePriceLevels(data.at("asks"));

        return OrderBookTick(updateId, symbol,json_string, std::move(bids_vec), std::move(asks_vec), localTimestampNs);

    } catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON parsing error in BinanceClient::to_struct: " << e.what() << "\n";
        throw std::runtime_error("Failed to parse Binance order book JSON data: " + std::string(e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Standard exception in BinanceClient::to_struct: " << e.what() << "\n";
        throw; 
    }
}