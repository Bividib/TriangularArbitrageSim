#include "common/trade_util.h"
#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>

void fail(const boost::beast::error_code& ec, const char* category){
    std::cerr << category << ": " << ec.message() << std::endl; 
}

void fail(const char* message, const char* category) {
    std::cerr << category << ": " << message << std::endl;
}