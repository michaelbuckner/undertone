# Third-party notices

Undertone's original work is copyright © 2026 Michael Buckner and is distributed under **AGPL-3.0-only**. See [LICENSE](LICENSE) and [COPYRIGHT](COPYRIGHT).

## JUCE and its dependencies

This release uses JUCE 9.0.4 at commit `3fb5cbc0340758596dac39f7c8de47d562b4896e` under JUCE's **AGPLv3 option**. No commercial JUCE license is asserted. JUCE is copyright Raw Material Software Limited and its contributors.

Binary packages include `licenses/JUCE-LICENSE.md`, the upstream inventory `licenses/JUCE.spdx.json`, and `licenses/JUCE-THIRD-PARTY-NOTICES.txt`. The notice collection preserves upstream license files, README notices and copyright-bearing source comments, including those for optional components that Undertone does not compile. It is intentionally broader than the runtime dependency set. These original notices do not mean that all optional SDKs or JUCE tools are included in the app.

JUCE incorporates third-party code with its own terms, including MIT, BSD, Apache 2.0, ISC, zlib, libpng and IJG notices. Those copyrights and terms are retained; the project license does not replace them. The corresponding source archive includes the complete pinned JUCE source tree and its notices. Optional AAX and ASIO source is included under its available GPLv3 option; Undertone does not build an AAX plug-in or enable ASIO.

## Windows runtime libraries

Windows packages include the release Microsoft Visual C++ runtime DLLs selected by CMake's `InstallRequiredSystemLibraries` from the build runner's Visual Studio toolchain. These Microsoft system runtime components are proprietary distributable code, not Undertone source and not relicensed under AGPL. Their filenames and SHA-256 hashes appear in `BUILD-MANIFEST.json`.

Their use and redistribution remain subject to the applicable [Microsoft Visual Studio license terms](https://visualstudio.microsoft.com/license-terms/) and [redistributable-file guidance](https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files). No debug runtimes or Microsoft development tools are included. The corresponding-source archive covers Undertone and its non-system dependency JUCE; it does not claim to provide Microsoft system-library source.

## Source and rebuilding

Download `Undertone-0.12.1-source.zip` alongside the binaries on the [v0.12.1 release page](https://github.com/michaelbuckner/undertone/releases/tag/v0.12.1). It includes Undertone, the pinned JUCE dependency, build scripts and license notices. See [source and license details](docs/source-and-license.md). There are no third-party sample packs or recordings in the application; its factory sounds are synthesized.
