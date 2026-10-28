#include "antlr4-runtime.h"
#if __has_include("Version.h")
#include "Version.h"
#endif
#include <iostream>

int main() {
  const auto version = antlr4::RuntimeMetaData::getRuntimeVersion();
#if defined(ANTLRCPP_VERSION_MAJOR) && defined(ANTLRCPP_VERSION_MINOR) && \
    defined(ANTLRCPP_VERSION_PATCH)
  // Some 4.13.2 builds stringify misspelled macros in RuntimeMetaData::VERSION.
  // In that case, use the numeric version from the installed headers.
  if (version == "ANTLR4CPP_VERSION_MAJOR.ANTLR4CPP_VERSION_MINOR."
                 "ANTLR4CPP_VERSION_PATCH") {
    std::cout << ANTLRCPP_VERSION_MAJOR << '.' << ANTLRCPP_VERSION_MINOR << '.'
              << ANTLRCPP_VERSION_PATCH;
    return 0;
  }
#endif
  std::cout << version;
}
