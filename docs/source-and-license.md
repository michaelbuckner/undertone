# Source, licensing and rebuilding

Undertone is free software under the GNU Affero General Public License, version 3 only (`AGPL-3.0-only`). You may use, study, modify and redistribute it under that license. It comes without warranty. Read the complete [license](../LICENSE); third-party components retain the notices described in [THIRD-PARTY-NOTICES.md](../THIRD-PARTY-NOTICES.md).

The [v0.12.1 preview release](https://github.com/michaelbuckner/undertone/releases/tag/v0.12.1) provides `Undertone-0.12.1-source.zip` at the same location as its Windows and Mac binaries, without charge. This is the corresponding-source download: it contains the exact Undertone Git revision, the complete JUCE 9.0.4 revision at `vendor/JUCE`, source and build scripts, and copyright/license notices. GitHub's automatically generated source ZIP excludes JUCE; use the explicitly named source asset for the complete dependency source. `SOURCE-MANIFEST.json` and each runtime's `BUILD-MANIFEST.json` record the project and dependency revisions.

To build the source archive, extract it and install Python 3.10+, CMake 3.22+, and the platform compiler. On Windows use Visual Studio C++ Build Tools and a Windows SDK, then run `scripts/build-windows.ps1` in PowerShell. On macOS use Xcode Command Line Tools, then run `bash scripts/build-macos.sh`. Both commands build and run the tests. The archive already contains JUCE, so CMake does not need to fetch it. General-purpose compilers, operating-system frameworks and system runtime libraries are separate prerequisites.

To install only the runtime files, run:

```sh
cmake --install build --config Release --prefix dist/Undertone --component UndertoneRuntime
```

Use `build-macos` instead of `build` for the Mac script's build directory. The installed directory contains the app, documentation, notices, and Windows runtime DLLs where applicable. It does not contain JUCE development headers or `juceaide`.

Maintainers build release archives from committed Git checkouts using `scripts/package-release.py`; `.github/workflows/build.yml` records the exact platform configure, build, test and packaging commands. It generates the complete source archive on the Mac job, preserving executable file modes. This packaging script requires Git metadata; ordinary compilation and installation from the downloaded source archive do not. Runtime ZIPs carry SHA-256 manifests, and the release includes `SHA256SUMS.txt` for all downloadable archives.

The binaries are preview builds. Windows is unsigned; macOS has no Developer ID signing or notarization. macOS requires version 14 or newer and includes Intel and Apple silicon code. CI tests do not establish interactive Mac audio/UI behavior, physical Elektron timing, or full clean-machine compatibility.
