# Minimum CMake version required for this project
# Always set this to a version you know works with the features you use.
# Modern CMake often recommends 3.15 or higher.
cmake_minimum_required(VERSION 3.15)

# Define your project.
# PROJECT_NAME is the name of your project.
# LANGUAGES specifies the programming languages used (CXX for C++).
project(TriangularArbitrageSim LANGUAGES CXX)

# Specify the C++ standard to use.
# It's good practice to explicitly set this.
set(CMAKE_CXX_STANDARD 17) # Or 20, 23, etc.
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
# The first argument is the name of your executable (e.g., MyExecutable).
# The subsequent arguments are the source files that make up this executable.
add_executable(Main src/main.cpp src/websocket/websocket.cpp)

cmake_policy(SET CMP0167 NEW)

find_package(Boost REQUIRED COMPONENTS beast)

target_link_libraries(Main PRIVATE Boost::beast)

# (Optional) Set output directory for executables/libraries (good practice)
# This places your build artifacts in a predictable location.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# --- Unit Test Setup ---
enable_testing() # Crucial: enables CTest infrastructure

# Define a separate executable for tests
add_executable(UnitTests test/test.cpp) 

# Link your test executable to Google Test libraries
target_link_libraries(UnitTests PRIVATE
    GTest::gtest_main  # Links to gtest and gtest_main for convenient test execution
)

# If your tests need to include headers from your main application source (src/)
# so they can test your application's code, add the include directory:
target_include_directories(UnitTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src # Allows tests to include headers from src/
)

include(GoogleTest)

# Discover Google Tests within the UnitTests executable
# CTest will run 'UnitTests' with a special flag to list all individual tests
gtest_discover_tests(UnitTests) # <-- Use the name of your test executable here!

# Add this block to your CMakeLists.txt
# This ensures coverage flags are only added when -DCOVERAGE=ON is passed to CMake
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