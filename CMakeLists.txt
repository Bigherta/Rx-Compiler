cmake_minimum_required(VERSION 3.15)

# Select Clang before project() enables compiler detection.
if(NOT DEFINED CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER clang CACHE STRING "C compiler")
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER clang++ CACHE STRING "C++ compiler")
endif()

project(rx_parser LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/AntlrRuntime.cmake)

add_subdirectory(src)

option(RX_FAST_BUILD "Reduce compiler build time for semantic judging" OFF)
if(RX_FAST_BUILD AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # Target options follow the Release flags, including ANTLR's added -O3.
  # This changes the compiler executable's optimization, not its generated code.
  foreach(target IN ITEMS antlr4_static rx_parser rx_compiler)
    if(TARGET ${target})
      target_compile_options(${target} PRIVATE -O0 -g0)
    endif()
  endforeach()
endif()
