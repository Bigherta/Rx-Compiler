cmake_minimum_required(VERSION 3.15)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialize dependencies when src is used as the top-level project.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(rx_compiler LANGUAGES CXX)

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
  AST/ASTBuilder.hpp
  AST/ASTBuilder.cpp
  ASTVisitor/ASTVisitor.hpp
  ASTVisitor/ASTPrinter.hpp
  ASTVisitor/ASTPrinter.cpp
)
target_include_directories(rx_parser PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/Parser"
  "${CMAKE_CURRENT_SOURCE_DIR}/AST"
  "${CMAKE_CURRENT_SOURCE_DIR}/ASTVisitor"
)
target_link_libraries(rx_parser PUBLIC antlr4_static)

add_executable(rx_compiler main.cpp)
target_link_libraries(rx_compiler PRIVATE rx_parser)
