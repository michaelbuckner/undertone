**Apple silicon build and verification**

Mike confirmed an M-series Mac is available; execution and physical sync checks are deferred while development focuses on Windows. No commands have yet run on that Mac from this Windows workspace. The prepared build remains universal (Apple silicon and Intel), with macOS 14 as its minimum deployment target. Apple silicon is the first physical Mac verification target.

Copy the source bundle to the Mac and unzip it. The bundle contains the project source and scripts, not a compiled Mac application. The first build downloads the pinned JUCE source from GitHub. CMake 3.22 or later and Xcode Command Line Tools are prerequisites; the script checks for them and exits with a message if either is missing. It does not install tools or change system settings.

In Terminal, change into the extracted project directory and run:

```sh
bash scripts/verify-macos.sh
```

The script builds both CPU architectures, runs all eleven suites (core, evolution, detail, melody, composition, cycles, refinements, percussion, MIDI, sessions and export) on the native CPU, verifies the application contains both architectures, launches the app stopped for a smoke check, and prepares a Mac ZIP that preserves application executable permissions. On an Apple silicon Mac with Rosetta already available, it also runs the Intel test binaries; it does not install Rosetta. Translated tests are not a substitute for checking Intel hardware/audio behavior.

Results are saved under `build-macos/verification/`. The log, `native-smoke/smoke.txt`, and `native-smoke/window.png` show the actual outcome. The application is under `build-macos/Undertone_artefacts/Release/Undertone.app`; the packaged copy is `dist/Undertone-macos-universal.zip`. Neither build nor package has been signed/notarized for public distribution.

Close any existing Undertone instance before running the script: the app permits only one instance, so an existing copy would prevent the smoke instance from writing its own report. The smoke test opens an audio device but does not press Play. Its settings/recovery files are isolated from ordinary sessions.

After automated checks pass, open the application normally, select the intended audio output, and verify Generate, Play, Stop/Panic, locks, Evolve, and save/reopen. Then test USB/DIN clock from each Elektron box. A successful build or visible MIDI port does not establish measured sync, sound quality, or the twenty-minute jam acceptance criteria.

If the script fails, its log identifies the failed step. Do not report Mac support as verified until the native tests and application launch have actually passed. A missing audio device is a separate runtime issue even if the smoke app exits successfully.
