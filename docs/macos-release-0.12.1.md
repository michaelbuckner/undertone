# Reproducing the v0.12.1 Mac preview

The Mac executable uses the unchanged v0.12.1 source commit `2e982e204227d9f4c4415ec379fefea0be5a0f91` and JUCE commit `3fb5cbc0340758596dac39f7c8de47d562b4896e`. The existing complete source ZIP and release tag remain unchanged. Download `Undertone-0.12.1-source.zip` from the [release](https://github.com/michaelbuckner/undertone/releases/tag/v0.12.1); it includes the pinned JUCE source and build instructions.

The [successful Mac workflow](https://github.com/michaelbuckner/undertone/actions/runs/41504928151) used a standard `macos-15-intel` runner. It compiled both x86_64 and arm64 with macOS 14.0 as the deployment target, passed all 13 native Intel test suites, installed the runtime component, and required both architectures through `lipo`. Independent ZIP inspection also checked both Mach-O architecture slices and their minimum OS versions, executable permissions, runtime hashes, and notices. ARM64 execution was not tested in this release build: hosted Apple silicon runners failed to acquire capacity. Interactive Mac audio/UI and physical MIDI timing remain unverified. There is no Developer ID signing or notarization.

## Packaging correction

The tagged packaging script puts the binary after `lipo -verify_arch`'s variable-length architecture arguments. Apple's tool interprets that path as another architecture. The release includes `Undertone-0.12.1-macOS-packaging-fix.patch` alongside the source ZIP. It makes exactly this correction:

```python
subprocess.run(["lipo", str(binary), "-verify_arch", "arm64", "x86_64"], check=True)
```

Both architecture checks remain mandatory. This changes only archive verification, not compilation or the app. The permanent fix and exact fallback workflow are public in [commit 221ccb1](https://github.com/michaelbuckner/undertone/commit/221ccb19ac5d0a3e9eacc46d7c5f2593e93810d9).

For an ordinary build from the extracted source ZIP, use `bash scripts/build-macos.sh` and the installation instructions in [source-and-license.md](source-and-license.md). Git metadata is not needed to compile or install.

For the release packaging procedure, use a clean Git checkout at the release commit and follow the [fixed workflow](https://github.com/michaelbuckner/undertone/blob/221ccb19ac5d0a3e9eacc46d7c5f2593e93810d9/.github/workflows/macos-preview.yml). It builds with:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix dist/Undertone --component UndertoneRuntime
```

Its packaging step applies the one-line correction in memory before running the tagged packager. This preserves the clean checkout and accurate source revision in `BUILD-MANIFEST.json`; applying a patch directly to tracked files would correctly trigger the packager's dirty-tree guard. No app source is changed. Archive timestamps and build-tool versions can affect byte-for-byte output; this documents the source and procedure, not a deterministic-build claim.
