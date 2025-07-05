#!/bin/bash

echo "Checking code formatting..."
if ! find . \( -name "*.h" -o -name "*.c" \) -exec clang-format --dry-run --Werror {} \;; then
    echo "::error::Code is not formatted."
    exit 1
fi