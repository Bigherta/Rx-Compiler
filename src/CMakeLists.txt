cmake_minimum_required(VERSION 3.28)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialize dependencies when src is used as the top-level project.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  # Select GCC 14 before project() enables compiler detection.
  if(NOT DEFINED CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER gcc-14 CACHE STRING "C compiler")
  endif()
  if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER g++-14 CACHE STRING "C++ compiler")
  endif()
  set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
    "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/GccCxx26Compat.cmake")

  project(rx_compiler LANGUAGES CXX)

  include(CTest)

  set(CMAKE_CXX_STANDARD 26)
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
  Frontend/SemanticAnalyzer.cpp
  Frontend/SemanticResult.hpp
  IR/IRPrinter.hpp
  IR/IRGenerator.cpp
  IR/IRVisitor.hpp)
target_link_libraries(rx_compiler PRIVATE rx_parser)

option(RX_BUILD_IR_UNIT_TESTS "Build focused IR generator unit tests" OFF)
if(BUILD_TESTING AND RX_BUILD_IR_UNIT_TESTS)
  add_executable(rx_emit_place_tests
    ../tests/unit/emit_place.cpp
    Frontend/SymbolCollector.cpp
    Frontend/SemanticAnalyzer.cpp
    IR/IRGenerator.cpp)
  target_include_directories(rx_emit_place_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/IR")
  target_link_libraries(rx_emit_place_tests PRIVATE rx_parser)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rx_emit_place_tests PRIVATE -O0 -g0)
  endif()
  find_program(RX_LLVM_AS NAMES llvm-as-22 llvm-as)
  foreach(case IN ITEMS
      place_i32 place_array temporary_i32 temporary_bool temporary_array
      temporary_reference deref_i32 deref_bool deref_array deref_twice
      temporary_deref borrow_i32 borrow_array temporary_borrow deref_then_borrow
      mut_to_shared mut_to_shared_deref diverging never_to_any)
    set(ir_path "${CMAKE_CURRENT_BINARY_DIR}/tests/emit-place/${case}.ll")
    add_test(NAME "emit_place.${case}" COMMAND rx_emit_place_tests "${case}" "${ir_path}")
    if(RX_LLVM_AS)
      add_test(NAME "emit_place.verify.${case}"
        COMMAND "${RX_LLVM_AS}" "${ir_path}" -o "${ir_path}.bc")
      set_tests_properties("emit_place.verify.${case}" PROPERTIES DEPENDS "emit_place.${case}")
    endif()
  endforeach()
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Coverage.cmake")
