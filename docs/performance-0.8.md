**Performance, phrasing and capture — v0.8.0**

The app now opens in **Performance**, with chords, scenes, layer levels, motif controls and cycles visible together. **Sound** exposes the existing synthesis, voicing, arrangement and device controls. Both views edit the same instrument. At smaller window sizes the content scrolls.

**Phrasing and development**

New starting characters enable **Statement / answer phrasing**. Song uses a statement followed by an answering phrase and a full-bar melodic breath; Still and Uneasy retain their sparse rhythm while gaining directed contours and answering endings. The melody planner follows the progression and up to eight scheduled change boundaries, including scene, composition and motif changes. It replans when you intervene. This is a bounded musical heuristic; the listening test remains whether it supports your bass part.

The automatic motif selector controls what **Cycle motif** does:

| Mode/action | Behavior |
| --- | --- |
| Hold motif | Prevent automatic motif development. Notes still adapt to changing harmony. |
| Vary motif | Keep the opening identity and develop later notes. |
| New motifs | Choose a new melodic identity each cycle. |
| Vary motif button | Develop the ending immediately. |
| New motif button | Replace the melodic identity immediately, even while Hold is selected. |
| New figure button | Change only the broken-chord pattern. |

**Give me space** thins melody and figure events and extends breaths without changing the saved layer levels. Zero restores the original activity. It does not listen to your bass or change tempo.

**A/B scenes**

1. Shape a passage, then **Store A**.
2. Change harmony, motif, sound or mix—or select another starting character—and **Store B**. Selecting a character retains the scene bank and stops playback.
3. Use **Recall A/B** to return to those snapshots. Stopped recalls are immediate; playing recalls happen at the next bar and restart the recalled phrase at slot one.
4. Enable **Alternate A/B** with a 1–64-bar interval to return automatically. Both slots must be stored. **Vary on return** adds motif development without changing either saved snapshot; a scene whose motif mode is Hold remains held.

Scenes contain their own harmony, locks, rests, sound, melody/figure identities, layer mix, percussion and evolution settings. A recall intentionally restores those locks rather than carrying unrelated live locks into a different key. Tempo and the global cycle controls remain live. A scene recall takes priority over composition/motif cycles at the same boundary; normal cycles can run between scene recalls. Manual edits take priority over automatic recalls. Store replaces that slot's snapshot; Save or Keep this moment preserves both slots to disk. MIDI Start resets the cycle grid; MIDI Continue resumes it.

**Deliberate chord swaps**

Select **Nearby swap**, **More tension**, or **Surprise swap** above the chord strip, then press Swap under a slot. Candidate scoring considers movement from both neighbors and the motif's opening pitch preference. More tension favors tighter interval relationships and richer fields; Surprise introduces a seeded random preference. These are different search preferences, not guarantees of an emotional result. **Prev** toggles the selected slot between the latest swap and its preceding chord. Locks protect Swap, Rest and Prev. Key/register/phrase edits and scene recalls clear comparison history. Rest / Restore retains its existing meaning and timing.

**Louder percussion**

Percussion level now ranges from **0 to 2**. **1.0 is the old maximum; 2.0 doubles the percussion signal before the master output stage**, up to roughly +6 dB. The master still controls the complete mix and limits its peaks, so the final measured increase depends on the rest of the mix. Old saved values retain their level. Try 1.2–1.5 for a modest lift, then balance Master level for your monitoring setup.

**Keep, record and export**

- **Keep this moment** writes a uniquely named `.undertone` snapshot in `%APPDATA%/Undertone/Moments` under the normal Windows configuration. The status line reports its exact path. It saves the realized state and both scenes, excluding uncommitted edits and effect tails. It is a snapshot, not a retrospective audio recording.
- **Record WAV** asks for a file and captures the app's actual stereo output as 24-bit WAV at the selected device sample rate. It includes live controls, incoming-clock playback and tails. Press **Stop recording** to finish; transport Stop alone leaves recording running so you can capture the release. Device changes stop the take. Buffer overflow or a mismatched output file is reported as incomplete. It records Undertone, not your bass input or the Elektron's audio.
- **Export MIDI** renders a new 4/8/16/32/64/128-bar performance from the currently realized state, starting at the beginning. Active evolution and cycles are included. It writes a tempo track and separate Pad, Broken figure, Melody and Percussion tracks at 960 PPQ. MIDI channels are 1, 2, 3 and 10; percussion uses GM note numbers 36/37/42/39. Assign your own instruments after import. Surface, reverb and the synth timbres are audio properties and are not encoded in MIDI. All note endings are written at or before the selected endpoint. This does not export an earlier live performance or send MIDI to an external instrument.

**Recall and platform scope**

Schema 8 stores the new controls and both scenes. Schemas 1–7 import with the new phrasing disabled, the old shared figure identity split into matching starting values, and no scene bank. They retain their saved percussion levels. Later melody and figure edits are independent. Earlier app versions cannot open schema 8; keep original session files when comparing versions.

This release is verified on Windows. The source and build scripts retain macOS support, but Mac execution and physical Elektron interoperability are deferred at Mike's request. Simulated MIDI timing and local offline renders do not certify a physical interface, another computer, or an embedded board.
