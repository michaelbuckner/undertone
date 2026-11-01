# Studies: composition, space and texture — v0.6.0

Mike found the generated music too cheesy and requested inspiration from the Nine Inch Nails Ghosts records. The v0.5 Memory sketches both used I–vi–IV–V, regular broken-chord accompaniment and prominent melodic contours. This revision adds a different generative vocabulary, longer form and contrasting foreground/background sounds. Its musical success still needs Mike's listening judgment.

## Two original starting points

**Studies / Quiet room** is the new stopped startup state: D minor, 68 BPM, suspended/open harmonic fields, a sparse Felt object motif and a distant FM bed. The recurring gesture has five onsets per eight-bar statement, followed by an eight-bar melodic withdrawal later in the form.

**Studies / Fault line** uses F minor, 74 BPM, closer harmonic intervals, Bowed wire, darker FM and more surface texture. Its six-onset gesture returns with quieter answering figures as the background builds. Both characters leave percussion off; the existing percussion switch remains available when wanted.

These are original synthesized pieces. No NIN recordings, transcribed melodies or sample loops are included. The reference is compositional: small recurring gestures, negative space, ambiguous harmony, tactile foreground sounds and contrasting emotional directions. NIN describes [Ghosts I–IV](https://www.nin.com/music/ghosts/) as an improvised instrumental work; the [Ghosts V/VI announcement](https://www.nin.com/nine-inch-nails-ghosts-v-vi-available-now/) contrasts Together's reassurance with Locusts. The design choices here are our interpretation, not a claim to reproduce those albums' production methods or sound.

## Controls

**Composition**, beside the Evolve controls, changes generation and phrasing:

| Mode | Behavior |
| --- | --- |
| Song / familiar | Existing harmony templates, note patterns and effect routing. |
| Still / spacious | Slower-feeling, sparse gestures over open fifth/second-based fields. |
| Uneasy / sparse | Sparse repeated cells over fifth/scale-sixth-based fields, with closer intervals depending on key and mode. |

Still/Uneasy generation favors repeated harmonic centers and a limited set of neighboring fields instead of conventional dominant chains. Complexity selects Sparse/Open/Layered voicings; Omit root and Lowest note still apply. Chord cards say “open” or “tension” to avoid labeling these shapes as conventional seventh chords. Changing Composition regenerates unlocked harmony at the normal phrase boundary. Unlock the phrase before changing modes. It keeps the sound and layer levels; the Studies presets provide complete starting mixes. Generate makes another progression within the selected vocabulary. New motif changes its recurring pitch preferences and rhythmic variant.

**Let layers come and go** enables a repeating 32-bar arc in Still/Uneasy: Opening → Statement → Pressure → Withdrawal. Pad and surface levels move continuously across section boundaries; supporting figures enter after the opening, and the melody rests throughout the final eight bars. The section name is shown under Composition. Turning the option off holds the manual layer balance steady and removes the section-based melody withdrawal; the sparse rhythmic cells remain. Song retains its earlier sixteen-bar arrangement.

**Felt object** and **Bowed wire** expand Melody voice to six choices. Felt object has velocity-sensitive, slightly inharmonic partials whose upper components decay quickly. Bowed wire has a slower attack, beating partials and a restrained metallic component. They are original oscillator models, not sampled pianos or acoustic emulations. The new supporting figures use Felt object. Surface adds quiet, filtered friction-like noise and a restrained pitch-related resonance; zero adds no surface signal.

In Still/Uneasy, sustained common chord tones remain held across nominal chord changes, avoiding repeated attacks. New notes enter while departing tones release. Foreground melody/figures use a largely dry path with a small send into the shared effects; the pad and surface sit further back. Percussion, if enabled, retains its existing pulse and transport behavior rather than following the new section mutes.

## Listening and limits

The full-cycle previews in `dist/Undertone-0.6.0-audio` use the exact app engine, a common preview gain of 9 and short endpoint fades. Quiet room contains 32 bars plus an eight-second release tail, approximately 2:01; Fault line approximately 1:52. They are WAV and 192 kb/s MP3, with no added mastering, samples or external arrangement. Let the quieter final section play: it is part of the composition, not missing output.

The form and rhythmic cells are deliberately authored constraints within a generative instrument. This is not an unrestricted composer, and the same seed/settings produce the same choices. It still uses 4/4 and the shared internal/MIDI transport. Known queued harmony is included in melody planning; uncommitted future edits cannot be predicted. Sound quality and bass-jam usefulness remain subjective checks.

## Compatibility and architecture

Schema 6 stores Composition, Surface, harmonic color and six melody voice IDs. Schemas 1–5 import as Song with Surface zero; schema 5 retains its saved voice. Earlier melodic migration rules remain documented in the v0.5 guide. Old applications cannot open schema 6 saves, so keep originals when moving forward. Existing factory characters remain available.

`Composition.*` provides the bounded form calculation and surface synthesis. `Music.cpp` supplies mode-specific voicings/generation; `Detail.cpp` supplies sparse gesture scheduling and modal voices; `Engine.cpp` applies held-note continuity and depth routing. No allocation, network service or file access is required during audio processing. The hardware equivalent must budget for onset planning and surface/voice DSP; no hardware platform or purchases are selected here.

Developer renders:

```
render_sketch quiet-room.wav 10 0 0 4 9 112.941176470588
render_sketch fault-line.wav 11 0 0 5 9 103.783783783784
```

See [build status](build-status.md) for measured evidence. Windows is locally tested; Mac execution and real Elektron timing remain pending.
