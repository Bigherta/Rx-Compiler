# Older CMake releases do not know GCC 14's C++26 flags. This override is
# loaded after compiler detection, including inside try_compile/try_run.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14
   AND NOT DEFINED CMAKE_CXX26_STANDARD_COMPILE_OPTION)
  set(CMAKE_CXX26_STANDARD_COMPILE_OPTION "-std=c++26")
  set(CMAKE_CXX26_EXTENSION_COMPILE_OPTION "-std=gnu++26")
endif()
