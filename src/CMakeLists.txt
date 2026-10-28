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

  include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/AntlrRuntime.cmake")
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
target_link_libraries(rx_parser PUBLIC rx_antlr_runtime)

add_executable(rx_compiler main.cpp
  Frontend/SymbolCollector.hpp
  Frontend/SymbolCollector.cpp
  Frontend/Scope.hpp
  Frontend/SemanticAnalyzer.hpp
  Frontend/SemanticAnalyzer.cpp)
target_link_libraries(rx_compiler PRIVATE rx_parser)

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Coverage.cmake")
