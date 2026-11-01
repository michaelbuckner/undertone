#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cmake -S "$root" -B "$root/build-macos" -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
cmake --build "$root/build-macos" --parallel 3
ctest --test-dir "$root/build-macos" --output-on-failure
printf 'Built: %s/build-macos/Undertone_artefacts/Release/Undertone.app\n' "$root"
