cmake_minimum_required(VERSION 3.15)

project(TriangularArbitrageSim LANGUAGES CXX)

cmake_policy(SET CMP0167 NEW)

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer standard-compliant code

if(MSVC)
  add_compile_options(/bigobj)
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main 
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add an executable target.
add_executable(Main 
src/main.cpp 
src/exchange/binance/binance_client.cpp
src/common/common.cpp
src/server/arbitrage_server.cpp
)

target_include_directories(Main PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

find_package(Boost REQUIRED COMPONENTS beast asio)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(Main PRIVATE Boost::beast Boost::asio OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# # --- Unit Test Setup ---
enable_testing() # Crucial: enables CTest infrastructure

# Define a separate executable for tests
add_executable(UnitTests 
test/test.cpp 
test/test_trade_leg.cpp
src/exchange/binance/binance_client.cpp
src/common/common.cpp
src/server/arbitrage_server.cpp
)

# Link test executable to Google Test libraries
target_link_libraries(UnitTests PRIVATE
    GTest::gtest_main  
    Boost::beast
    Boost::asio
    OpenSSL::SSL 
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

target_include_directories(UnitTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src # Allows tests to include headers from src/
)

include(GoogleTest)

# Discover Google Tests within the UnitTests executable
# CTest will run 'UnitTests' with a special flag to list all individual tests
gtest_discover_tests(UnitTests) 

# Ensures coverage flags are only added when -DCOVERAGE=ON is passed to CMake
if (COVERAGE)
    message(STATUS "Enabling code coverage instrumentation")
    # For GCC/Clang (Ubuntu runner)
    target_compile_options(Main PRIVATE --coverage)
    target_link_libraries(Main PRIVATE --coverage)
    target_compile_options(UnitTests PRIVATE --coverage)
    target_link_libraries(UnitTests PRIVATE --coverage)

    # Define build output paths for consistency, crucial for coverage tools to find artifacts
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE PATH "Output directory for static libraries")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE PATH "Output directory for dynamic libraries")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" CACHE PATH "Output directory for executables")

endif()
# # --- End Unit Test Setup ---