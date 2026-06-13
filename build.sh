#!/bin/bash
# build.sh — fast incremental build for Linux target
#
# Usage:
#   ./build.sh           # incremental build (default)
#   ./build.sh --clean   # wipe build dir and rebuild from scratch
#   ./build.sh --debug   # build with Debug symbols (default is Release)
#   ./build.sh --run     # build then run the bridge with config.json
#
# The script always regenerates compile_commands.json so IDE navigation
# stays up to date.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
BUILD_TYPE="Release"
CLEAN=0
RUN=0

# ── Parse arguments ────────────────────────────────────────────────────────
for arg in "$@"; do
    case "$arg" in
        --clean)  CLEAN=1 ;;
        --debug)  BUILD_TYPE="Debug" ;;
        --run)    RUN=1 ;;
        --help|-h)
            sed -n '2,12p' "$0" | sed 's/^# //'
            exit 0
            ;;
        *)
            echo "Unknown option: $arg  (use --help for usage)"
            exit 1
            ;;
    esac
done

# ── Clean ──────────────────────────────────────────────────────────────────
if [ "$CLEAN" -eq 1 ]; then
    echo "=== Cleaning build directory ==="
    rm -rf "$BUILD_DIR"
fi

# ── Configure (only if CMakeCache is missing or after a clean) ─────────────
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "=== Configuring (${BUILD_TYPE}) ==="
    cmake -B "$BUILD_DIR" \
          -DPLATFORM=linux \
          -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          "$SCRIPT_DIR"
else
    # Reconfigure only if CMakeLists.txt changed since last configure
    if [ "$SCRIPT_DIR/CMakeLists.txt" -nt "$BUILD_DIR/CMakeCache.txt" ]; then
        echo "=== Re-configuring (CMakeLists.txt changed) ==="
        cmake -B "$BUILD_DIR" \
              -DPLATFORM=linux \
              -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              "$SCRIPT_DIR"
    fi
fi

# ── Build ──────────────────────────────────────────────────────────────────
echo "=== Building (${BUILD_TYPE}, $(nproc) jobs) ==="
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo ""
echo "=== Build complete ==="
echo "  Binary : $BUILD_DIR/uartTelnetBridge"
echo "  CompDB : $BUILD_DIR/compile_commands.json"

# ── Optionally run ─────────────────────────────────────────────────────────
if [ "$RUN" -eq 1 ]; then
    echo ""
    echo "=== Running bridge ==="
    exec "$BUILD_DIR/uartTelnetBridge" "$SCRIPT_DIR/config.json"
fi
