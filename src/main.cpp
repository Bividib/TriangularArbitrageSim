#define _WIN32_WINNT 0x0601 
#define NOMINMAX          

#include <iostream>
#include "client/market_data_client.h"
#include "server/arbitrage_server.h"
#include "server/arbitrage_session.h" 

int main() {

    // Smart Pointers POC 

    // UNIQUE POINTERS 

    // A unique_ptr models an object that has exactly one owner at any given point in time
    // When destroyed it has responsibility in deleting the object it owns 
    // auto server = std::make_unique<Server>();
    // Server* underlyingPointerToServer = server.get();

    // SHARED POINTERS 

    // A shared_ptr extends unique ptr by allowing it to get copied to other shared_ptrs
    // Makes use of reference counting, when this reaches 0, the memory is freed 
    // Shared pointers make it also a strong pointer, this is because only when all variables
    // go out of scope will the memory get deallocated 

    // WEAK POINTERS

    // To be used in conjunction with shared pointers to account for the scenario when we
    // want some code to have a pointer to the object, but not a shared one, the object 
    // should NOT hold back dellocation of the memory 
    // We could infact simply call .get() on a shared pointer and use the Object* reference but
    // we will have no idea at what point and WHEN this will become deallocated 

    // Weak references have a lock() method to return a shared pointer of the object 
    // This is to handle multi-threaded environments where the last shared_pointer goes out
    // of scope, if we never made our own strong reference temporarily out of the weak pointer
    // then all of a sudden we have a pointer that is referencing a bunch of nonsense bytes 

    // Weak references also have a expired() method that returns true/false
    // Weak references also have a use_count() method that return reference count! 

    // With assignments when you say one object is equal to another, you are always copying 
    // This is with the exclusion of pointers, as a copy of the pointer to the same address is made 

    // Always pass objects by const Class& to avoid copying

    // Object o = std::move(object) will use the move constructor 
    // o = std::move(object) will use the move assignment operator 


    // Create local host string and ports as const strings 
    const std::string localHost = "127.0.0.1";
    const std::string port = "8080";

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(localHost), std::stoi(port));
    boost::asio::io_context io_context; 

    // -- BEGIN SERVER CODE (Performs the algo trading) -- 

    // Async start up
    auto server = std::make_shared<Server>(io_context,endpoint);
    server->run();

    // // -- END SERVER CODE -- 

    // // -- BEGIN CLIENT CODE (Sends the market data) -- 

    auto client = std::make_shared<Client>(io_context);
    client->async_connect(endpoint);

    // -- END CLIENT CODE --

    io_context.run();
    return 0;
}