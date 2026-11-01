**Refinement completion audit / v0.8.0**

Authorized scope: clearer melodic statements and answers; Hold/Vary/New with independent melody and figure identities; recallable A/B scenes and optional variations on return; context-aware Nearby/Tension/Surprise chord swaps with comparison; compact Performance view and Give me space; Keep this moment, recording and MIDI export; higher percussion level range. Windows is the current verification target at Mike's request. Mac and physical Elektron validation are deferred, not release claims.

Completed implementation and verification:

- Musical phrasing: statement/answer/breath behavior, multi-boundary harmony forecast, deterministic replay, actual rendered comparisons.
- Motifs: Hold suppresses automatic identity changes; Vary retains opening identity; New replaces identity; broken figure independence; recall.
- Scenes: two stored states, quantized recall, optional automatic A/B returns and variation, exact clean recall, serialization/migration, live UI.
- Chords: three intentional swap choices, neighboring harmony/melody input, per-slot comparison, locks/rests/queued timing.
- Performance view: visible transport/chords/scenes/layer mix/cycles at normal window size; detail controls in Sound view; Give me space reduces activity and preserves saved base mix.
- Capture: one-click durable snapshot; real output recording; reproducible MIDI export of engine events with correct tempo, note-offs and track roles; file failure handling.
- Percussion: increased upper range, old-session levels unchanged, finite output and measured gain difference.
- Delivery: relevant tests, full Windows suite, native UI inspection, packaged-app launch, documentation and versioned Windows/source artifacts.

Implementation and functional verification are complete for the authorized Windows scope. `build/verification-0.8.0.txt` records 11/11 passing suites. `RefinementTests.cpp` and `ExportTests.cpp` cover the new contracts, alongside existing melody, cycle, DSP, session and MIDI suites. `build/auditions-0.8` contains fixed-gain statement/variation/new-motif and scene renders; distributable MP3s are in `dist/Undertone-0.8.0-audio`.

Native UI evidence: stored A/B levels 0.5/1.5 and recalled A; Keep this moment saved both scenes; actual output recording finalized a valid 24-bit stereo 96 kHz WAV (24.16 seconds); a 16-bar type-1 MIDI file with five tracks/960 PPQ exported successfully, with all emitted note-ons paired. Performance and Sound views were inspected. QA data is isolated under `build/isolated-0.8-ui`.

Final delivery checks: versioned Windows package, packaged-app smoke launch, source/archive equality and archive integrity are recorded in `build/package-audit-0.8.0.txt` after packaging. Platform boundary: Windows only for this delivery. Mac execution, clean-machine installation, physical Elektron sync, long-duration device qualification and Mike's subjective musical assessment remain explicitly unverified; none is presented as completed.
