cmake_minimum_required(VERSION 3.15)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialize dependencies when src is used as the top-level project.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  # Select Clang before project() enables compiler detection.
  if(NOT DEFINED CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER clang CACHE STRING "C compiler")
  endif()
  if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER clang++ CACHE STRING "C++ compiler")
  endif()

  project(rx_compiler LANGUAGES CXX)

  include(CTest)

  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "Build ANTLR C++ runtime tests")
  set(ANTLR_BUILD_SHARED OFF CACHE BOOL "Build shared ANTLR C++ runtime")
  set(ANTLR_BUILD_STATIC ON CACHE BOOL "Build static ANTLR C++ runtime")
  add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/antlr4-cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/vendor/antlr4-cpp"
  )
endif()

add_library(rx_parser STATIC
  Parser/RxLexer.cpp
  Parser/RxLexer.h
  Parser/RxParser.cpp
  Parser/RxParser.h
  Parser/RxParserListener.cpp
  Parser/RxParserListener.h
  Parser/RxParserBaseListener.cpp
  Parser/RxParserBaseListener.h
  Parser/RxParserVisitor.cpp
  Parser/RxParserVisitor.h
  Parser/RxParserBaseVisitor.cpp
  Parser/RxParserBaseVisitor.h
  AST/AST.hpp
  AST/AST.cpp
  Frontend/ASTBuilder.hpp
  Frontend/ASTBuilder.cpp
  Frontend/ASTVisitor.hpp
  Frontend/ASTBaseVisitor.hpp
  Frontend/ASTPrinter.hpp
  Frontend/ASTPrinter.cpp
)
target_include_directories(rx_parser PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/Parser"
  "${CMAKE_CURRENT_SOURCE_DIR}/AST"
  "${CMAKE_CURRENT_SOURCE_DIR}/Frontend"
)
target_link_libraries(rx_parser PUBLIC antlr4_static)

add_executable(rx_compiler main.cpp
  Frontend/SymbolCollector.hpp
  Frontend/SymbolCollector.cpp
  Frontend/Scope.hpp
  Frontend/SemanticAnalyzer.hpp
  Frontend/SemanticAnalyzer.cpp)
target_link_libraries(rx_compiler PRIVATE rx_parser)

if(BUILD_TESTING)
  add_executable(symbol_collector_test
    tests/symbol_collector_test.cpp
    Frontend/SymbolCollector.cpp
  )
  target_link_libraries(symbol_collector_test PRIVATE rx_parser)
  add_test(NAME symbol_collector COMMAND symbol_collector_test)
endif()
