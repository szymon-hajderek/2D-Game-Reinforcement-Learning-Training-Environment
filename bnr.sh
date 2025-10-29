#!/usr/bin/env bash
set -e                      # stop on first error

# configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release "$@"

# compile (--parallel uses all cores with CMake ≥3.27)
cmake --build build --config Release --parallel

./build/game
