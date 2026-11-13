# Fill in these five commands. See README-EN.md or README-ZH.md for details.

# OJ reads this configuration without the root Makefile's defaults.
CLANG ?= clang-22

# Optional: build your compiler once before testing. Leave empty if prebuilt.
# {runtime} will be linked into every program.
# Write shared runtime assembly there if your IR/CODEGEN output references it.
# Large frontend/IR translation units need substantial memory; opt in to parallelism.
RX_BUILD_JOBS ?= 1
BUILD = cmake -S . -B target/oj-build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++-14 \
    -DRX_FAST_BUILD=ON -DRX_USE_SYSTEM_ANTLR=ON && \
    cmake --build target/oj-build --target rx_compiler -j$(RX_BUILD_JOBS) && \
    $(CLANG) --target=riscv32-unknown-elf -march=rv32im -mabi=ilp32 \
    -O2 -fno-builtin -S vendor/runtime/runtime.c -o {runtime}

# Required for semantic tests: exit 0 to accept {source}, 1 to reject it.
SEMANTIC = ./target/oj-build/src/rx_compiler --check {source}

# Required for IR tests: write RV32 LLVM IR to {output} (.ll).
IR = ./target/oj-build/src/rx_compiler {source} -o {output}

# Required for codegen/optimization tests: write RV32IM assembly to {output} (.s).
CODEGEN = RUST_MIN_STACK=16777216 RX_SOURCE={source} $(REFERENCE_RUSTC) --crate-type=staticlib \
    --emit=asm={output},link={output}.a -C opt-level=2 -C lto=fat \
    -C llvm-args=-riscv-no-aliases crates/rx/src/entry.rs && \
    $(PYTHON) scripts/strip_asm_debug.py {output}

# Required alongside IR/CODEGEN: run the assembly with the BUILD runtime in REIMU.
RUN = xmake run -P vendor/REIMU reimu --memory=256M --stack=1M \
    -f {output},{runtime} -o {stdout} -p {profile} 1>&2

# Rust reference helper; remove once your commands no longer use it.
# v0 symbols avoid quoted section names that REIMU does not recognize.
REFERENCE_RUSTC = rustc \
    --edition=2021 \
    --target=riscv32im-unknown-none-elf \
    --crate-name=rx_test \
    -Awarnings -Aarithmetic_overflow \
    -C overflow-checks=off -C panic=abort -C symbol-mangling-version=v0 \
    --extern rx=target/reference/riscv32im-unknown-none-elf/release/librx.rlib \
    -L dependency=target/reference/riscv32im-unknown-none-elf/release/deps
