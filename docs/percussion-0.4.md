# Optional percussion — v0.4.0

Mike requested a basic generative rhythm that keeps time while fitting the Sun-faded and Afterimage sketches, with later Radiohead as an aesthetic reference. This implementation uses original synthesized thumps, resonant noise rims, short hats and tiny digital ticks. It contains no sampled recordings or transcribed artist patterns. Whether it has the desired feel remains a listening decision.

## Playing

Enable **Percussion** near the upper performance controls. Scroll below Arrangement for:

| Control | Behavior |
| --- | --- |
| Level | Independent percussion mix. The main sound Level also scales the complete output. |
| Density | Adds hats and quieter offbeat hits around four fixed pulse anchors per bar. |
| Glitch | Adds short 32nd-note accents, with more opportunities near bar endings. Zero removes these accents. |
| Color | Changes the frequency character of new hits. |
| New pattern | Chooses a new seeded offbeat skeleton without replacing harmony or motif. At zero Density and Glitch, the stable pulse intentionally remains the same. |

Patterns repeat over four bars. A seed fixes their choices; this is controlled generative variation, not continuous random replacement. Percussion runs independently of Evolve, chord locks and the sixteen-bar melodic arrangement. All existing characters default to percussion off. Sun-faded has gentler accent settings than Afterimage. Changing character stops playback and loads that character's percussion settings, including off.

## Sound and transport

The portable core has twelve fixed percussion voices, separate from the sixteen chord and twelve detail voices. Sounds use oscillators, filtered noise and short envelopes. No allocation or file access is needed while rendering. Percussion bypasses the pad reverb and Bass cut; its own 75 Hz high-pass removes deep sub energy but does not eliminate all overlap with a bass guitar. Mix it to suit the bass part. Harmonic note-floor and root-omission rules apply to the pitched accompaniment, not percussion.

Internal onsets share the engine's beat counter. External onsets use every third MIDI clock tick for the 32nd-note grid, retaining the existing timestamp-buffer delay. Start resets the pattern; Continue resumes; Stop and lost clock stop new hits and release active voices. Panic clears voices and filter tails. The enable/level path is smoothed over approximately 15 ms.

## Sessions and compatibility

Schema 4 stores enabled state, level, density, glitch, color and the unsigned pattern seed. Schemas 1–3 import with percussion off. Older apps cannot open new schema 4 saves; keep an original copy when moving forward. Playback position and ringing voices are not saved.

With percussion off, the full 52-second Sun-faded and Afterimage WAV comparisons match the v0.3 previews byte for byte. This verifies those reference sketches, not every possible edited session.

## Auditions and developer rendering

`dist/Undertone-0.4.0-audio` contains 52-second Sun-faded and Afterimage mixes plus a percussion-only demonstration, each as WAV and 192 kb/s MP3. Preview-only level adjustment and short endpoint fades are applied; the musical engine is the same as the app. The solo demonstration is normalized independently, so its loudness does not represent its level inside the full mix.

`render_sketch <output.wav> <8|9> <plain:0|1> <percussion:0|1|2>` selects off, on or solo percussion. Use plain=0 for these layered or solo previews. `soak_test 120 1 1 1` stresses Ladder, the detail layers and percussion together.

There is no drum sequencer editor, swing control, sample import, separate percussion output or drum MIDI output in this revision. Mac execution and physical Elektron timing remain unverified. See [build status](build-status.md) for measured Windows checks.
