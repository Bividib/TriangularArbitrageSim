#include <boost/beast/core/error.hpp>

void fail(const boost::beast::error_code& ec, const char* category);

struct OrderBookTick {
    long long updateId;       
    std::string symbol;       
    std::string bestBidPrice; 
    std::string bestBidQty;   
    std::string bestAskPrice; 
    std::string bestAskQty;  
    long long localTimestampMs;

    OrderBookTick() : updateId(0), symbol(""), bestBidPrice(""), bestBidQty(""),
                          bestAskPrice(""), bestAskQty(""), localTimestampMs(0) {}

    OrderBookTick(const long long& u, const std::string& s, const std::string& b, const std::string& B, const std::string& a, const std::string& A, const long long& ts = 0)
        : updateId(u), symbol(std::move(s)), bestBidPrice(std::move(b)), bestBidQty(std::move(B)),
          bestAskPrice(std::move(a)), bestAskQty(std::move(A)), localTimestampMs(ts) {}

    double getBidPrice() const { return std::stod(bestBidPrice); }
    double getBidQty() const { return std::stod(bestBidQty); }
    double getAskPrice() const { return std::stod(bestAskPrice); }
    double getAskQty() const { return std::stod(bestAskQty); }
};