cmake_minimum_required(VERSION 3.15)

project(TriangularArbitrageSim LANGUAGES CXX)

cmake_policy(SET CMP0167 NEW)

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer standard-compliant code

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
src/client/market_data_client.cpp
src/common/common.cpp
src/server/arbitrage_server.cpp
src/server/arbitrage_session.cpp
)

target_include_directories(Main PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

find_package(Boost REQUIRED COMPONENTS beast asio)

target_link_libraries(Main PRIVATE Boost::beast Boost::asio)

# (Optional) Set output directory for executables/libraries (good practice)
# This places your build artifacts in a predictable location.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# --- Unit Test Setup ---
enable_testing() # Crucial: enables CTest infrastructure

# Define a separate executable for tests
add_executable(UnitTests 
test/test.cpp 
src/client/market_data_client.cpp
src/common/common.cpp
src/server/arbitrage_server.cpp
src/server/arbitrage_session.cpp
)

# Link test executable to Google Test libraries
target_link_libraries(UnitTests PRIVATE
    GTest::gtest_main  
    Boost::beast
    Boost::asio
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
# --- End Unit Test Setup ---