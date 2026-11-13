.DEFAULT_GOAL := test

include config.mk

PYTHON ?= python3
VERBOSE ?= false
# Directory paths below tests, joined with ':'; separate selections with ','.
FILTER ?=
# Optional single stage: semantic, ir, codegen, or optimization.
STAGE ?=
COMPILE_TIMEOUT ?= 30
RUN_TIMEOUT ?= 10

# Export commands as data, so shell quoting survives Make's recipe expansion.
export RX_TEST_BUILD = $(BUILD)
export RX_TEST_SEMANTIC = $(SEMANTIC)
export RX_TEST_IR = $(IR)
export RX_TEST_CODEGEN = $(CODEGEN)
export RX_TEST_RUN = $(RUN)
export FILTER STAGE CLANG COMPILE_TIMEOUT RUN_TIMEOUT VERBOSE

.PHONY: test runtime
test:
	@$(PYTHON) scripts/test.py

# Regenerate and commit this assembly after changing vendor/runtime/runtime.c.
# The OJ C++ build image has no Clang or RV32 cross compiler.
runtime:
	$(CLANG) --target=riscv32-unknown-elf -march=rv32im -mabi=ilp32 \
		-O2 -fno-builtin -S vendor/runtime/runtime.c -o vendor/runtime/runtime.s
