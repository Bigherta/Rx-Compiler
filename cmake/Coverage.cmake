option(RX_ENABLE_COVERAGE "Instrument the compiler for GCC/gcov coverage" OFF)

if(RX_ENABLE_COVERAGE)
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "RX_ENABLE_COVERAGE requires GCC and a matching gcov")
  endif()
  # Do not instrument the third-party runtime. Generated parser code is filtered
  # out of the report by scripts/coverage.sh.
  foreach(target IN ITEMS rx_parser rx_compiler)
    target_compile_options(${target} PRIVATE --coverage -O0 -g)
  endforeach()
  target_link_options(rx_compiler PRIVATE --coverage)
endif()
