# Display scale and preset refresh — v0.12.0

The interface now scales as a complete instrument. Auto selects 100–200% in 25-point steps from the usable display area in logical pixels; operating-system DPI scaling is already included. A 2560×1440 logical display selects 150%; a 3840×2160 logical display selects 200%. This is an area-based default, not a measurement of physical monitor inches or viewing distance.

The footer offers Auto and fixed percentages. The choice is stored in `interface.json` beside the audio settings, separately from musical sessions. Manual choices survive restart. Auto updates when the window moves or resizes onto another display. Undersized windows retain legible controls and scroll in both directions as needed.

## Factory bank

These are new starting points; loading an existing saved session does not replace its sound with a factory preset.

| Starting point | Arrangement and sound |
| --- | --- |
| Ambient / Slow light | 72 BPM D Dorian; warm sustained fields, air-flute gestures and a restrained surface layer. |
| Ambient / Glass tide | 78 BPM G major; soft FM glass with felt-object replies and long transitions. |
| Neo-soul / Sunday | 82 BPM D Dorian; rootless extended harmony, rounded FM keys and a lower felt-key melody. |
| Neo-soul / Velvet | 76 BPM A minor; slower rootless changes, warm Ladder bed and soft-sine answers. |
| Funk / Pocket | 104 BPM D Mixolydian; dry Ladder comping, tonic repetition and short felt-key offbeats. |
| Funk / Offbeat | 110 BPM G Dorian; DCO comping, a different harmonic loop and soft-sine gestures. |
| Dark ambient / Ash room | 64 BPM D minor; uneasy fields, bowed-wire melody and restrained friction texture. |
| Dark ambient / Slow fracture | 70 BPM F minor; open Ladder fields, felt-object interruptions and sparse development. |
| Memory / Sun-faded | 82 BPM F major; DCO bed, felt-key phrasing, lighter figures and less echo. |
| Memory / Afterimage | 76 BPM D minor; slow Ladder bed, air-flute phrases and quieter supporting replies. |
| Studies / Quiet room | Existing 68 BPM D minor reference retained. |
| Studies / Fault line | Existing 74 BPM F minor reference retained. |

Ambient and Dark ambient use the existing 32-bar Still/Uneasy form, including common-tone pad continuity and an eight-bar melodic withdrawal. Neo-soul, Funk and Memory use Song harmony with the new Lyrical or Pocket arrangement. Percussion stays off in all factory starting points, leaving room for Elektron drums.

## Song phrase styles

Sound > Layers offers Classic, Lyrical and Pocket. Classic preserves the previous arrangement. Lyrical and Pocket use a lower, narrower lead register, quieter supporting figures and a 16-bar layer arc when Let layers come and go is enabled. The melody withdraws in the final four bars. Supporting figures avoid simultaneous lead attacks.

With Statement / answer phrasing enabled, Lyrical uses spaced five-event phrases; Pocket uses five offbeat events with shorter gates. The existing bounded lookahead planner still considers future harmony, pending edits and loop returns; weak-beat passing notes must resolve by step. Still and Uneasy use their own sparse patterns, so the Song phrase selector is disabled there.

The preset refresh also replaces the six Song starting progressions with designed loops and voices them through two passes, including the return. Gentle tone evolution develops the sound without changing the factory harmony.

## Compatibility and verification

Session schema 9 stores phrase style in the live state and both scenes. Schema 1–8 imports default to Classic; older realized chords, timbres and layer settings remain intact. Invalid styles are rejected without changing the destination session. The engine bootstrap is independent of the editable factory bank so changing Slow light does not alter another session's initial parameter smoothing.

Windows verification includes all thirteen CTest suites, full 32-bar offline renders of all twelve starting points, and native captures at Auto and each fixed scale. New checks cover phrase density, deliberate rests, register, root omission, articulation, passing-note resolution, repeatability and scene serialization. Native interaction confirms saved scale preference survives restarting; navigation and scaling do not mark the musical session edited. Both Studies 32-second reference WAVs are byte-identical to the previous renderer at the same settings.

These checks establish implementation behavior, not a subjective judgment that every preset is equally musical. The sampler is for listening with bass and Elektron beats. Interactive Mac UI/audio, mixed-DPI multi-monitor movement and physical MIDI-clock checks still need device testing.

## Local auditions

The rendered files are generated artifacts under `build/preset-refresh/after`: `preset-0.wav` through `preset-11.wav`, following the table order. Each covers 32 bars at its factory tempo plus a tail, using the actual engine at 48 kHz stereo. Preview gain is a fixed 5×, capped only if necessary for headroom; it is not a change to the application's master gain.

`build/preset-refresh/auditions/preset-sampler.wav` contains twenty-second excerpts from Slow light, Sunday, Pocket, Ash room and Sun-faded, with one-second gaps. Each excerpt starts at bar 9 and receives short boundary fades; no EQ, compression or external sounds are added. The sampler is a quick comparison; the complete renders show the full form.

## v0.12.1: chord symbols for sparse voicings

Chord cards, Now/Next and queued previews now name Still/Uneasy voicings by their actual intervals instead of using “open” or “tension.” For example, a Bb-rooted F–C voicing is Bbsus2 (the root is omitted), and F–C–Db is F5(addb6). A sparse field has no third, so calling it major or minor would imply a note that is not there. Existing Song symbols such as Fmaj9 are retained. Imported fields outside the generated shapes show an explicit root-relative interval list. This changes display text only; notes, sound and session format are unchanged.
