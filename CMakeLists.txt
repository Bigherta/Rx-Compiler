cmake_minimum_required(VERSION 3.15)
project(rx_parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build the bundled runtime locally, without fetching its optional tests.
set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "Build ANTLR C++ runtime tests")
set(ANTLR_BUILD_SHARED OFF CACHE BOOL "Build shared ANTLR C++ runtime")
set(ANTLR_BUILD_STATIC ON CACHE BOOL "Build static ANTLR C++ runtime")
add_subdirectory(vendor/antlr4-cpp)

add_subdirectory(src)
