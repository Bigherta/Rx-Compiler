# ANTLR C++ runtime

Source: https://www.antlr.org/download/antlr4-cpp-runtime-4.13.2-source.zip

Version: 4.13.2. This directory contains the upstream C++ runtime sources,
its CMake build files, `VERSION`, `README.md`, and `LICENSE.txt`. Upstream
examples, tests, platform-specific IDE projects, and packaged build artifacts
are omitted. The code is licensed under the BSD 3-Clause license in
`LICENSE.txt`.

From the repository root, build the bundled runtime and the generated Rx
parser with `cmake -S . -B target/antlr4-build -DCMAKE_BUILD_TYPE=Release`
and `cmake --build target/antlr4-build -j 2`. The `rx_parser` target exposes
the generated headers and links the static runtime transitively. Regenerate
the C++ sources after editing the grammar with
`python3 scripts/generate_antlr_cpp.py` (requires the ANTLR 4.13.2 JAR in
`tools/`). The generator renames the grammar copies to `RxLexer` and
`RxParser` because plain `Lexer` and `Parser` conflict with C++ runtime types.
