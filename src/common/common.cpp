#include "common.h"
#include <iostream>
#include <boost/beast/core/error.hpp>

void fail(const boost::beast::error_code& ec, const char* category){
    std::cerr << category << ": " << ec.message() << "\n"; 
}