#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=build

usage() {
    echo "Usage: $0 [--clean|-c] [--reset-db]"
    echo "  (default)  Incremental configure + build + test"
    echo "  --clean    Remove $BUILD_DIR first (full rebuild)"
    echo "  --reset-db Delete api/data/lan.db (fresh SQLite on next API start)"
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

CLEAN=0
RESET_DB=0
for arg in "$@"; do
    case "$arg" in
        --clean|-c) CLEAN=1 ;;
        --reset-db) RESET_DB=1 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $arg" >&2; usage >&2; exit 1 ;;
    esac
done

if [[ "$RESET_DB" -eq 1 ]]; then
    python3 "$SCRIPT_DIR/lan_db.py" reset
fi

if [[ "$CLEAN" -eq 1 ]]; then
    rm -rf "$ROOT/$BUILD_DIR"
fi

cmake -S "$ROOT" -B "$ROOT/$BUILD_DIR"
cmake --build "$ROOT/$BUILD_DIR" --config Release
ctest --test-dir "$ROOT/$BUILD_DIR" --output-on-failure

echo "Build complete: $ROOT/$BUILD_DIR/scanner"
