**Dependency and distribution record**

Undertone uses [JUCE 9.0.4](https://github.com/juce-framework/JUCE), pinned to commit `3fb5cbc0340758596dac39f7c8de47d562b4896e`. CMake fetches that revision when `vendor/JUCE` is absent.

Undertone is released under AGPL-3.0-only, and this distribution uses JUCE's AGPLv3 option. See the full [project license](../LICENSE), [copyright notice](../COPYRIGHT), and [third-party notices](../THIRD-PARTY-NOTICES.md). Dependencies keep their own license and copyright notices.

Runtime packages include JUCE's license, SPDX dependency inventory, and a collection of original upstream notices. Windows packages also include release Microsoft Visual C++ runtime DLLs under their applicable Microsoft terms. The runtime component excludes JUCE development headers, CMake exports and tools.

The release's explicitly named source ZIP includes the exact Undertone revision and complete pinned JUCE sources with build scripts and notices. See [source and rebuilding](source-and-license.md). GitHub's automatic source downloads do not bundle JUCE.

The portable harmony and synthesis modules do not depend on JUCE. Factory presets are programmatically defined; no downloaded samples, recordings or sound packs are included in the app.
