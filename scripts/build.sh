#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=build

usage() {
    echo "Usage: $0 [--clean|-c]"
    echo "  (default)  Incremental configure + build + test"
    echo "  --clean    Remove $BUILD_DIR first (full rebuild)"
}

CLEAN=0
for arg in "$@"; do
    case "$arg" in
        --clean|-c) CLEAN=1 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $arg" >&2; usage >&2; exit 1 ;;
    esac
done

if [[ "$CLEAN" -eq 1 ]]; then
    rm -rf "$BUILD_DIR"
fi

cmake -S . -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --config Release
ctest --test-dir "$BUILD_DIR" --output-on-failure

echo "Build complete: $BUILD_DIR/scanner"
