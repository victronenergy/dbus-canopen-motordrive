#!/bin/bash

echo "Checking code formatting..."
find . \( -name "*.h" -o -name "*.c" \) | clang-format --dry-run --files=/dev/stdin -Werror