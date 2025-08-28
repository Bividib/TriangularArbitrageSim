#ifndef TRADE_UTIL_H
#define TRADE_UTIL_H

#include <boost/beast/core/error.hpp>

void fail(const boost::beast::error_code& ec, const char* category);

void fail(const char* message, const char* category);

void send_email(const std::string& subject, const std::string& body);

#endif // TRADE_UTIL_H