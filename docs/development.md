# Developing Undertone

Confirmed requirements: software first; Windows and Mac support; generative chords/pads; room for a live bass player; integration with Digitone II, Digitakt I, and Syntakt. Musical interests include ambient, neo-soul, funk, and the slower dark ambient side of later Nine Inch Nails. Apple silicon is the available Mac test target; interactive Mac verification is still pending.

The musical core is plain C++17. The native application uses JUCE 9.0.4, pinned to commit `3fb5cbc0340758596dac39f7c8de47d562b4896e`. No network service is used at runtime. Hardware component selection and spending remain open for a later discussion.

**Repository.** [michaelbuckner/undertone](https://github.com/michaelbuckner/undertone) is the public source repository. `main` tracks the current prototype. Generated builds, exports, local settings and the vendored dependency checkout are excluded.

**Build on Windows.** Install Python 3.10+, CMake 3.22+ and Visual Studio C++ Build Tools with a Windows SDK. Run `./scripts/build-windows.ps1` in PowerShell. The executable is `build/Undertone_artefacts/Release/Undertone.exe`. The script discovers the Visual Studio-bundled CMake when it is not on PATH. This is a native Windows build, not WSL.

**Build on macOS.** Install Xcode Command Line Tools, Python 3.10+ and CMake 3.22+, then run `bash scripts/build-macos.sh`. This requests a universal Intel/Apple silicon application with macOS 14 as its minimum target. The release universal build and all 13 native Intel test suites passed. ARM64 execution and interactive testing on a physical Mac remain unverified for this release. The output path is `build-macos/Undertone_artefacts/Release/Undertone.app`.

Both builds fetch the pinned JUCE commit when `vendor/JUCE` is absent. An initial internet connection is needed to obtain this build dependency; the finished application runs offline.

**Build the portable engine only.** Run `cmake -S . -B build-core -DUNDERTONE_APP=OFF`, then `cmake --build build-core --config Release` and `ctest --test-dir build-core -C Release --output-on-failure`. This does not need JUCE. `render_demo` creates a WAV from the exact synth engine used by the app; arguments are output path, preset index 0-7, duration in seconds, optional tone index 0-8, and optional Evolve override 0/1 (sets Variation to 0.65 and interval to two bars). `soak_test 120 1` accelerates two minutes of dense chords on the Ladder engine; model IDs 0-3 select Warm/Ladder/DCO/FM.

**Package.** After a full Release build, run `cmake --install build --config Release --prefix dist/Undertone --component UndertoneRuntime`. Windows packaging includes the required Microsoft runtime DLLs. The packages are unsigned previews; no installer, Developer ID signing, or notarization is configured. The GitHub Actions workflow builds and tests Windows/macOS and produces runtime ZIPs plus the complete corresponding source. Verified archives are published on the [Releases page](https://github.com/michaelbuckner/undertone/releases). Check the [Actions page](https://github.com/michaelbuckner/undertone/actions) for the current result; CI does not replace physical audio/MIDI verification.

**Source map.** `src/core/Music.*` contains scales, seeded generation, and voicing. `src/core/Engine.*` owns transport, queued edits, 16 voices, and effects. `src/core/MidiTimeline.*` maps incoming timestamps to audio samples through a bounded queue. `src/app` contains the native UI, audio/MIDI device adapters, and versioned session files. `tests` exercises the core, serialization, MIDI timing, rendering, and stress processing. No hardware-specific code is present.
