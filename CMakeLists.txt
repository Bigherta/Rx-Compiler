cmake_minimum_required(VERSION 3.15)
project(rx_parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build the bundled runtime locally, without fetching its optional tests.
set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "Build ANTLR C++ runtime tests")
set(ANTLR_BUILD_SHARED OFF CACHE BOOL "Build shared ANTLR C++ runtime")
set(ANTLR_BUILD_STATIC ON CACHE BOOL "Build static ANTLR C++ runtime")
add_subdirectory(vendor/antlr4-cpp)

add_library(rx_parser STATIC
  src/Parser/RxLexer.cpp
  src/Parser/RxParser.cpp
  src/Parser/RxParserListener.cpp
  src/Parser/RxParserBaseListener.cpp
  src/Parser/RxParserVisitor.cpp
  src/Parser/RxParserBaseVisitor.cpp
)
target_include_directories(rx_parser PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/Parser")
target_link_libraries(rx_parser PUBLIC antlr4_static)
