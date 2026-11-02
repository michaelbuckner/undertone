#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
if [[ "$(uname -s)" != Darwin ]]; then
  printf 'This verification script must run on macOS.\n' >&2
  exit 1
fi
if ! command -v cmake >/dev/null || ! xcrun --find clang++ >/dev/null 2>&1; then
  printf 'CMake 3.22+ and the Xcode Command Line Tools are required. See docs/mac-verification.md.\n' >&2
  exit 1
fi
report="$root/build-macos/verification"
mkdir -p "$report"
exec > >(tee "$report/verification.log") 2>&1
printf 'Host architecture: %s\n' "$(uname -m)"
sw_vers
cmake --version
bash "$root/scripts/build-macos.sh"
app="$root/build-macos/Undertone_artefacts/Release/Undertone.app"
binary="$app/Contents/MacOS/Undertone"
printf 'Built architectures: '
lipo -archs "$binary"
lipo "$binary" -verify_arch arm64 x86_64

# Keep verification recovery/audio settings separate from normal user sessions.
export UNDERTONE_DATA_DIR="$report/test-user-data"
"$binary" --smoke "$report/native-smoke" &
app_pid=$!
completed=false
for ((attempt=0; attempt<30; attempt++)); do
  if ! kill -0 "$app_pid" 2>/dev/null; then
    wait "$app_pid"
    completed=true
    break
  fi
  sleep 1
done
if [[ "$completed" != true ]]; then
  kill "$app_pid" 2>/dev/null || true
  wait "$app_pid" 2>/dev/null || true
  printf 'Native app did not complete its smoke check within 30 seconds.\n' >&2
  exit 1
fi
test -s "$report/native-smoke/window.png"
test -s "$report/native-smoke/smoke.txt"
cat "$report/native-smoke/smoke.txt"
if [[ "$(uname -m)" == arm64 ]] && arch -x86_64 /usr/bin/true 2>/dev/null; then
  printf 'Testing Intel test binaries through existing Rosetta support.\n'
  arch -x86_64 "$root/build-macos/core_tests"
  arch -x86_64 "$root/build-macos/evolution_tests"
  arch -x86_64 "$root/build-macos/detail_tests"
  arch -x86_64 "$root/build-macos/percussion_tests"
  arch -x86_64 "$root/build-macos/melody_tests"
  arch -x86_64 "$root/build-macos/composition_tests"
  arch -x86_64 "$root/build-macos/cycle_tests"
  arch -x86_64 "$root/build-macos/refinement_tests"
  (cd "$root/build-macos" && arch -x86_64 export_tests_artefacts/Release/export_tests)
  arch -x86_64 "$root/build-macos/midi_tests"
  (cd "$root/build-macos" && arch -x86_64 session_tests_artefacts/Release/session_tests)
  (cd "$root/build-macos" && arch -x86_64 library_tests_artefacts/Release/library_tests)
else
  printf 'Intel execution not checked. No Rosetta installation was attempted.\n'
fi
cmake --install "$root/build-macos" --prefix "$root/dist/Undertone-macOS" --component UndertoneRuntime
ditto -c -k --sequesterRsrc --keepParent "$root/dist/Undertone-macOS" "$root/dist/Undertone-macos-universal.zip"
printf '\nNative build/tests and smoke launch completed.\nThis does not certify listening quality, real-time stability, or Elektron timing.\n'
printf 'Results: %s\nPackage: %s\n' "$report" "$root/dist/Undertone-macos-universal.zip"
