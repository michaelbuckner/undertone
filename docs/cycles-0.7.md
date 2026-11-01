**Chord edits and automatic cycles — v0.7.0**

Every chord card now has **Swap** and **Rest / Restore** buttons. Clicking the card itself still locks its voicing.

| Action | Result |
| --- | --- |
| Swap | Replaces only that slot with a different scale-derived chord. Swapping a rest makes it sound again. |
| Rest | Stops new pad, melody, broken-chord and surface notes for that slot, keeping its length. Existing tails decay naturally; percussion continues. |
| Restore | Brings back the stored chord and voicing. |
| Lock | Protects the slot from Swap, Rest, Generate and automatic harmonic changes. Unlock to edit it. |

Stopped edits apply immediately. During playback, Swap and Rest / Restore land on the next bar. If a whole-phrase edit is already queued, they merge into that edit and keep its existing loop boundary. Multiple quick edits can be queued together. Undo restores the previous realized harmonic state at the loop boundary. Manual edits take priority over automatic cycles; missed automatic changes are skipped, not fired in a burst afterward.

Rest slots survive Generate and Evolve. Generate can update the underlying voicing; Restore uses that currently stored voicing. Evolve and the Still/Uneasy cycle leave the underlying notes of a rest unchanged. Changing key or register regenerates unlocked underlying notes so restored notes remain valid for the new settings.

**Automatic changes.** Scroll below Arrangement for two independent switches and intervals:

- **Alternate Still / Uneasy** behaves like a stepped, tempo-synced square LFO: Still → Uneasy → Still. The interval is the time between switches; a complete round trip takes twice that time. It revoices the existing chord roots and changes phrasing, retaining the selected sounds and mix. Locked voicings and stored rests stay intact. In Song mode this cycle pauses.
- **Generate new motifs** creates a new recurring motif at each interval, like a clocked sample-and-hold control. It does not alternate between two saved motifs. The New motif button still makes a change immediately.

Both offer 1, 2, 4, 8, 16, 32 or 64 bars and default off. They are independent of Evolve, Variation and Let layers come and go. Try Quiet room with composition every **16 bars** and motif every **8 bars**; use **8 / 4** for a more active passage. These are starting points for listening, not a guarantee of musical fit.

The cycles use the same transport as the chords: internal tempo or incoming MIDI Clock. Play/restart and MIDI Start reset the cycle grid, starting from the current composition and motif. Stop pauses; MIDI Continue resumes. Clock loss stops playback and requires a fresh MIDI Start. Changing an interval schedules its next change on that interval's global bar grid without restarting the other control. A visible countdown shows the next switch while playing.

**Phrasing.** The melody planner includes the next committed manual, evolution or cycle boundary and the upcoming progression in its lookahead. Simultaneous evolution, composition and motif changes are combined in that forecast. Later automatic boundaries are replanned as they approach; the planner does not forecast an unlimited chain of future changes. Rests produce no new pitched events, and even an entirely silent future has a bounded planning cost.

**Sessions.** Schema 7 saves each rest flag, its underlying realized notes, both cycle switches and their intervals. Schemas 1–6 load with rests and cycles off. Save captures the currently realized state, not queued edits or the exact transport phase. Earlier app versions cannot open schema 7; keep old session files if needed.

**Platform boundary.** The implementation is shared C++17/JUCE for Windows and macOS. Windows builds, automated tests and native UI are checked locally. Mac execution and actual Elektron synchronization still require testing on those devices.

**Audition.** `dist/Undertone-0.7.0-audio/alternating-room.mp3` (also WAV) uses Quiet room with Still/Uneasy switching every eight bars and new motifs every four bars. It plays 32 bars at 68 BPM plus an eight-second tail, about 2:01. The composition changes near 0:28, 0:56 and 1:25; the final section retains the existing withdrawal in the 32-bar form. It uses the app engine, fixed preview gain 9 and short endpoint fades. No external samples or mastering were added.

Reproduce it with `render_sketch output.wav 10 0 0 4 9 112.941176470588 1`. The final argument enables the audition's 8/4-bar cycle configuration; it is an offline developer renderer, not an export feature in the UI.
