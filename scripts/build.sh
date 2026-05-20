#!/usr/bin/env bash
set -euo pipefail

# Simple build script for the project using CMake
BUILD_DIR=build
mkdir -p "$BUILD_DIR"
cmake -S . -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --config Release

echo "Build complete: $BUILD_DIR/scanner"
