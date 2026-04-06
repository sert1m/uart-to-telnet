#!/bin/bash
set -e

echo "=== Configuring ==="
cmake -B build -DPLATFORM=linux

echo "=== Building ==="
cmake --build build -j$(nproc)

echo "=== Running tests ==="
./build/test/uartTelnetBridgeTests "$@"
