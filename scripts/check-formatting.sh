#!/bin/bash

echo "Checking code formatting..."
find ./src \( -name "*.h" -o -name "*.c" -o -name "*.cpp" -o -name "*.hpp" \) | clang-format --dry-run --files=/dev/stdin -Werror && \
find ./tests \( -name "*.h" -o -name "*.c" -o -name "*.cpp" -o -name "*.hpp" \) | clang-format --dry-run --files=/dev/stdin -Werror && \
find ./inc \( -name "*.h" -o -name "*.c" -o -name "*.cpp" -o -name "*.hpp" \) | clang-format --dry-run --files=/dev/stdin -Werror