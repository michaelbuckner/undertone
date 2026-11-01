**Undertone / focused Sound editor / v0.10.0**

Sound now occupies the same compact surface as Performance. Select a section on the left; the pad engine and tone selectors stay above it, while transport, harmony, tempo, output level and audio settings remain available. Returning from Performance restores the last Sound section for this app launch. Smaller windows still scroll.

| Section | Controls |
| --- | --- |
| Tone | Brightness, Motion, engine-specific texture, Resonance, Detune |
| Envelope | Attack, Decay, Sustain, Release, Filter envelope |
| Effects | Drive, Chorus, Space, Bass cut |
| Layers | Melody voice, layer breathing, New motif / New figure, Pad bed, Broken chords, Melody, Drift / wear, Figure echo, Surface |
| Percussion | Level, Density, Glitch, Color, New pattern; enable Percussion in the top strip |
| Phrase & MIDI | Complexity, Variation, Lowest note, phrase/chord length, articulation, composition, Evolve target/interval, clock input and Refresh MIDI |

Texture has a more useful name for each engine: **Oscillator weight** for Ladder, **Pulse character** for DCO, and **FM depth** for FM. Unsupported controls are dimmed and disabled. Warm uses Attack and Release rather than the full envelope; DCO does not use Detune; FM does not use Resonance, Detune or Filter envelope. These reflect the existing synthesis implementations.

Page selection changes the layout only. Existing parameters, ranges, smoothing and musical behavior are retained. Layer and percussion levels are shared with Performance; editing either view changes the same sound. Scene recall, capture and automatic motif/composition cycles remain on Performance. Schema 8 sessions remain compatible; no synthesis, session serialization or MIDI-export changes are included.

**Windows verification.** All eleven CTest suites pass (43.67 seconds). Native smoke captures cover four- and eight-chord phrases, all six sections, and 1124×850 / 1080×850 content sizes. Navigation preserves the encoded session and sends no engine commands. Live UI edits to Attack, Pad bed and Percussion level survive view switches; recovery-file comparison shows only those values and the expected Custom tone marker changed. Evidence is saved in `build/verification-0.10.0.txt`, `build/editor-interaction-0.10.0.txt`, the smoke directories, and `build/package-audit-0.10.0.txt`. Mac audio/UI testing and physical Elektron timing remain pending.
