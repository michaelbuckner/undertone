**Dependency record**

The desktop app uses [JUCE](https://github.com/juce-framework/JUCE), release 9.0.4, commit `3fb5cbc0340758596dac39f7c8de47d562b4896e`. CMake fetches that exact commit if a local development checkout is absent. The checked-out source used for the initial Windows build matches that commit.

JUCE's bundled `LICENSE.md` describes its AGPLv3/commercial licensing options. The package includes that file under `licenses/JUCE-LICENSE.md`. This is a local development prototype; no commercial license purchase, public release, or choice of license for Mike's original source has been made. Settle the project's distribution/license choice before publishing binaries or source. See the [upstream license file](https://github.com/juce-framework/JUCE/blob/3fb5cbc0340758596dac39f7c8de47d562b4896e/LICENSE.md).

The portable harmony and synthesis modules do not depend on JUCE. Factory presets are programmatically defined in this repository; no downloaded samples, recordings, or sound packs are included.
