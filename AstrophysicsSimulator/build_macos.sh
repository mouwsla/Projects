#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
if ! command -v cmake >/dev/null 2>&1; then
  echo 'Нужен CMake: brew install cmake'
  exit 1
fi
if [[ -d /opt/homebrew/opt/qt ]]; then
  export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt${CMAKE_PREFIX_PATH:+;$CMAKE_PREFIX_PATH}"
elif [[ -d /usr/local/opt/qt ]]; then
  export CMAKE_PREFIX_PATH="/usr/local/opt/qt${CMAKE_PREFIX_PATH:+;$CMAKE_PREFIX_PATH}"
fi
args=(-S . -B build -DCMAKE_BUILD_TYPE=Release)
if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
  args+=("-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH")
fi
cmake "${args[@]}"
cmake --build build --config Release -j 4
ctest --test-dir build --output-on-failure
open build/AstrophysicsSimulator.app
