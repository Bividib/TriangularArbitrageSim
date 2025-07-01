#include "binance_client.h"
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
    : BaseClient(ioc, ssl_ctx) {
    ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));
    ws.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::request_type& req) {
            req.set(boost::beast::http::field::user_agent, WS_CLIENT_HEADER);
        }
    ));
    std::cout << "BinanceClient initialised" << "\n";
}

void BinanceClient::async_connect(const std::string& host_in, const std::string& port_in, const std::string& target_in) {
    host = host_in;
    port = port_in;
    target = target_in;

    // try catch the read() 
    try {
        read();
    } catch (const std::exception& e) {
        std::cerr << "Error during read: " << e.what() << "\n";
        return;
    }

    const std::string& json_string = boost::beast::buffers_to_string(buffer.data());
    std::cout << "Received: " << json_string << std::endl;

    std::cout << "success!" << "\n";

    // resolver.async_resolve(
    //     host,
    //     port,
    //     boost::beast::bind_front_handler(
    //         &BinanceClient::on_resolve,
    //         shared_from_this()
    //     )
    // );
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
    if (ec) return fail(ec, "read");

    // Process the message
    const std::string& json_string = boost::beast::buffers_to_string(buffer.data());
    std::cout << "Received: " << json_string << std::endl;
    // auto data = nlohmann::json::parse(json_string);
    // auto tick_struct = to_struct(data);
    // Call observers here


    buffer.clear();
    read(); // Listen for the next message
}

void BinanceClient::write() {
    buffer.clear();
    write_str_to_buf("Test Message", buffer);
    
    ws.async_write(
        buffer.data(),
        boost::beast::bind_front_handler(
            &BinanceClient::on_write,
            shared_from_this()
        )
    );
}

void BinanceClient::on_write(boost::beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) return fail(ec, "write");

    std::cout << "Client Write successful" << "\n";
    // For testing, we can close after writing. In a real app, you wouldn't.
    ws.async_close(
        boost::beast::websocket::close_code::normal,
        boost::beast::bind_front_handler(
            &BinanceClient::on_close,
            shared_from_this()
        )
    );
}

void BinanceClient::on_close(boost::beast::error_code ec) {
    if (ec) return fail(ec, "close");
    std::cout << "Client connection closed gracefully" << "\n";
}

OrderBookTick BinanceClient::to_struct(const nlohmann::json& json_data) {
    const auto& data = json_data.at("data");
    const long long& updateId = data.at("u").get<long long>();
    const std::string& symbol = data.at("s").get<std::string>();
    const std::string& bid_price = data.at("b").get<std::string>();
    const std::string& bid_qty = data.at("B").get<std::string>();
    const std::string& ask_price = data.at("a").get<std::string>();
    const std::string& ask_qty = data.at("A").get<std::string>();
    
    return OrderBookTick(updateId, symbol, bid_price, bid_qty, ask_price, ask_qty);
}