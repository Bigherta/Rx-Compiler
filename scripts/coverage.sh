#!/usr/bin/env bash
# Run the same semantic suites locally and in CI, then report handwritten code.
set -euo pipefail

cd "$(dirname "$0")/.."
PYTHON=${PYTHON:-python3}

cmake -S . -B target/coverage-build \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++ \
  -DRX_USE_SYSTEM_ANTLR=OFF -DRX_FAST_BUILD=OFF -DRX_ENABLE_COVERAGE=ON
cmake --build target/coverage-build --target rx_compiler -j2

# Each report describes this test run, not accumulated results from older runs.
find target/coverage-build -type f -name '*.gcda' -delete

test_status=0
make test PYTHON="$PYTHON" BUILD= \
  'SEMANTIC=./target/coverage-build/src/rx_compiler {source}' \
  FILTER=official:semantic,custom STAGE=semantic || test_status=$?

# Generate a partial report even when a regression test fails; keep its exit code.
mkdir -p target/coverage
"$PYTHON" -m gcovr target/coverage-build \
  --root . \
  --filter 'src/(Frontend|AST)/' --filter 'src/main\.cpp$' \
  --exclude 'src/Frontend/ASTPrinter\.(cpp|hpp)$' \
  --exclude-throw-branches --exclude-unreachable-branches \
  --html-details target/coverage/index.html \
  --txt target/coverage/branches.txt --txt-metric branch \
  --json-summary target/coverage/summary.json \
  --print-summary

exit "$test_status"
