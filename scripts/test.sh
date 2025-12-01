#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/.."

BUILD_DIR="$PROJECT_ROOT/build-tests"

mkdir -p "$BUILD_DIR"

cmake -S "$PROJECT_ROOT/tests" -B "$BUILD_DIR" -DENABLE_COVERAGE=ON

cmake --build "$BUILD_DIR"

ctest --test-dir "$BUILD_DIR" --output-on-failure

lcov --capture --directory "$BUILD_DIR" --output-file "$BUILD_DIR/coverage.info" --ignore-errors inconsistent --rc branch_coverage=1 --exclude '/usr/*' --exclude '*/googletest/*'  --exclude '*/velib/*'  --exclude '*/tests/*' --quiet

genhtml "$BUILD_DIR/coverage.info" --output-directory "$PROJECT_ROOT/coverage" --branch-coverage --demangle-cpp --quiet

lcov -l "$BUILD_DIR/coverage.info"