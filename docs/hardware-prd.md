**Hardware PRD — generative accompaniment instrument**

Draft v0.1 · October 31, 2026 · Concept-stage equivalent of the software PRD

**Product purpose.** Turn the validated software experience into a small desktop instrument that makes chords and pads, follows Mike's Elektron setup, and can be used while playing bass without a laptop. Preserve the musical controls and saved ideas from the software where practical.

Confirmed constraints: software comes first; Mike is new to electronics and open to learning; hardware approaches must be discussed before choosing a build path. Digitone II, Digitakt I, and Syntakt are the intended companions. No processor, kit, enclosure, bill of materials, or budget is approved by this document.

**Hardware v1 scope.** A standalone digital stereo synth with one harmonic part, physical performance controls, onboard preset storage, and MIDI clock input. It shares the software MVP's keys/modes, progression rules, locks, voicing, Bass Space behavior, Loop/Evolve modes, and sound directions. The software PRD is the authoritative musical specification. Feature reductions require an explicit tradeoff decision after benchmarking, rather than quietly redefining equivalence.

The first hardware experiment can use an existing MIDI controller and an assembled computer or development kit. It need not resemble the eventual enclosure. The eventual v1 must run without a connected computer, keyboard, or mouse after setup.

| ID | Priority | Hardware requirement |
| --- | --- | --- |
| HW-01 | P0 | Generate and play chords/pads entirely on the device while offline. Preserve the software's note-event semantics and at least 16 note voices, subject to a feasibility gate before board selection. |
| HW-02 | P0 | Physical Play/Stop, Generate, Lock, Back/Undo, and master volume. Panic must be immediately reachable through a clearly labeled action. External mode distinguishes local mute/stop from control of the clock leader. |
| HW-03 | P0 | Direct access to Variation, Complexity, Brightness, and Space; selected secondary controls may share encoders with clear display feedback. Key/mode and Bass Space must take no more than two navigation actions to reach. |
| HW-04 | P0 | Display current and next chord, key/mode, tempo, bar/phrase position, clock state, lock state, and pending changes. Avoid menu-only access to basic jam functions. |
| HW-05 | P0 | Stereo line output with mono-compatible patches; headphone output with suitable output circuitry and level control. Exact connectors and output levels are selected during engineering design. |
| HW-06 | P0 | Dedicated MIDI input for clock and transport from Elektron gear. Prefer 5-pin DIN for direct connections; a TRS choice must identify its wiring standard and needed adapter. |
| HW-07 | P0 | Retain sessions after power-off. Save/load must preserve realized chords and the portable musical state. Failed or interrupted writes retain a valid prior session. |
| HW-08 | P0 | Powered by a suitable external low-voltage supply; physical power control, startup mute, controlled shutdown if needed, and clear ready/error feedback. |
| HW-09 | P0 | A documented firmware/software update and recovery process accessible to a beginner; update failure must not erase user presets. |
| HW-10 | P0 | Export/import the shared session data through a chosen service connection or removable storage. Missing sound capabilities are reported; note-event data is preserved. |
| HW-11 | P1 | MIDI output for generated notes and clock; MIDI footswitch mapping; USB MIDI device mode; additional scene controls. |
| HW-12 | Future | CV/gate, expression pedal input, battery operation, audio input/sidechain, additional layers, and custom circuit board manufacture. |

**Physical interaction proposal.** Start with a display, four endless encoders, five dedicated buttons, and a master volume knob. Encoders default to Variation, Complexity, Brightness, and Space; a settings page exposes the remaining parameters. The chord strip uses selectable slots for individual locks. Generate and Undo follow the same phrase-boundary behavior as the software. A full-phrase Lock is accessible without entering a menu.

This is a prototype layout, not a finalized panel. Test whether Mike can read the next chord while standing with a bass, and whether reaching for a control interrupts playing. If frequent changes are awkward, a footswitch may be a higher priority than more knobs. Avoid committing enclosure holes until the control layout has survived actual jams.

**Software parity addition (v0.7).** The selected chord slot needs reachable Swap and Rest / Restore actions alongside Lock. Preserve bar-quantized edits and stored voicings without shortening the phrase. Provide independent bar intervals for Still/Uneasy alternation and new motifs, with current mode and next-change feedback. Reuse the software's transport, lock protection and schema 7 state; the physical button/encoder mapping remains a prototype decision for the hardware discussion.

**Audio and MIDI connection concept.**

| Connection | Purpose |
| --- | --- |
| One Elektron MIDI Out → synth MIDI In | Supplies clock and transport; choose exactly one leader. |
| Synth L/R output → mixer or interface | Carries the generated sound. |
| Elektron audio outputs and bass signal → appropriate mixer/interface inputs | Combines drums, accompaniment, and bass for monitoring/recording. Bass input requirements depend on the existing bass rig. |
| Service connection → Windows or Mac | Transfers presets and updates when needed; not required during playing. |

MIDI and audio are distinct paths. USB device mode connects the synth to a host computer; USB host mode connects supported USB peripherals to the synth. One does not imply the other. Direct DIN MIDI avoids making USB hosting a prerequisite for the initial standalone jam. Do not assume a microcontroller's audio pins are finished headphone or line-output jacks; use a development carrier or properly designed output stage.

**Build approaches to discuss.** These are engineering judgments based on the platform types, not measured results for this instrument.

| Approach | What it is | Why it fits | Main learning and tradeoffs | Decision status |
| --- | --- | --- | --- | --- | --- |
| Controller plus software | Existing MIDI controls operate the desktop prototype; later reuse the control layout. | Fastest way to learn which knobs/buttons matter, with little electronics work. | Requires a computer and does not satisfy standalone hardware v1. MIDI mapping is an additional prototype task. | Suggested first physical experiment. |
| Small Linux computer | Raspberry Pi-class computer, supported audio interface/board, MIDI interface, display, and controls. | Potential to reuse much of the desktop C++ engine and iterate easily. | OS/audio setup, startup time, storage durability, shutdown, power, thermal behavior, and control wiring. Desktop UI still needs adaptation. | Leading candidate for a first standalone prototype, pending measured audio performance. |
| Embedded audio development board | Daisy-class microcontroller audio platform, preferably in an assembled carrier/dev kit. | Closer to a dedicated instrument with a compact embedded program. | Tighter processing/memory budgets, embedded debugging, audio/control hardware, and more porting. Polyphony and effects must be measured. | Compare after the software engine's resource needs are known. |
| Custom electronics and PCB | Design a dedicated board and enclosure around a chosen processor and audio circuitry. | Can provide the final layout and connections once requirements settle. | Highest electronics and fabrication effort; multiple revisions may be needed. | Defer until an assembled prototype proves useful. |

The desktop framework candidate, [JUCE](https://juce.com/features/), supports Linux as well as Windows/macOS. This supports investigating a small-computer port, but does not establish performance or driver compatibility on a selected board. Raspberry Pi documents USB audio output in its [configuration guide](https://www.raspberrypi.com/documentation/computers/configuration.html). The current [Daisy audio platform product page](https://daisy.audio/products/seed3) provides an embedded audio development option; the exact revision, carrier, available ports, development workflow, and cost need checking at selection time.

My proposed learning route is software → controller experiment → assembled standalone prototype → optional custom panel/PCB. A small computer is likely the gentler first standalone path if reusing the software is the priority. An embedded audio kit may be more appealing if learning firmware and electronics is the main goal. Neither choice is committed here, and a custom analog polyphonic synth is not required to achieve the desired sound.

**Shared software boundary.** Preserve a portable harmony/voicing module, seeded generation, event model, and versioned session schema. Separate audio-device access, controls/display, and storage adapters. A Linux build may reuse synthesis code; an embedded build may need a smaller DSP implementation. Establish a reference corpus of saved phrases and expected note events so a hardware port can demonstrate musical equivalence even when the sound engine differs.

The v0.2 software adds independent tone selection and Evolve targets/intervals. Carry those concepts into hardware controls: chord locks must allow tone evolution, and the display should show its next boundary and whether the target is chords, tone, or both. Compare the measured cost of Original Warm, Ladder, DCO and FM before selecting a board or promising all four engines at full polyphony. Desktop stress measurements do not establish microcontroller feasibility. Hardware design, component purchases and approach selection remain a later discussion with Mike.

The v0.3 arrangement revision adds optional melody and broken-chord roles with twelve shared detail voices, separate from the sixteen bed voices. Preserve their seeded note semantics and mix controls in the hardware equivalent, including phrase gaps and root omission. Profile the extra voices and delay memory before committing to a processor or polyphony target. Existing sessions with the additions off remain a useful minimum baseline.

The v0.4 revision adds optional percussion with twelve additional fixed voices, an independent mix and seeded four-bar patterns. Hardware equivalence includes the stable pulse, Density/Glitch/Color, New pattern, saved seed, shared transport and off switch. Benchmark the combined 40-voice capacity and effects before choosing hardware; simultaneous active counts vary. Keep access to the percussion level straightforward and assess low-frequency overlap with live bass. This does not settle the electronics platform, controls or component purchases; those remain a later learning and design discussion.

The v0.5 melody revision adds a fixed-size lookahead search at melody onsets, harmonic release of melody voices/echoes, and four selectable melody timbres. Preserve this behavior in the hardware musical core and include voice selection in the arrangement controls and saved session. Benchmark worst-case onset computation as well as average DSP load before choosing an embedded processor; this change does not select or purchase hardware.

The v0.6 revision adds Still/Uneasy composition modes, open/tension field generation, a 32-bar form, held common tones, two modal voices and a quiet surface-texture path. Include Composition, Surface and section feedback in the hardware interaction design, preserving shared MIDI transport and session fields. Benchmark these paths together with the existing voices/effects on candidate hardware. The software Windows stress results are not embedded performance guarantees.

The required hardware sound capability is warm sustained pads, richer chord stabs, sparse funk articulation, and driven rhythmic textures. If the candidate board cannot support 16 voices plus the effects at the required sample rate, compare a stronger board with a documented reduction in effects or voice count before proceeding. A MIDI-only generator can be a useful experiment but is not the hardware synth described by this PRD.

**Learning and delivery milestones.**

1. Musical checkpoint: finish the software's jam tests and identify the controls Mike actually uses. Output: selected feature set and saved reference sessions.
2. Control experiment: map an existing controller or use a simple assembled control surface. Output: tested panel layout and a list of actions that need dedicated buttons or foot control.
3. Architecture discussion: compare a small computer and an embedded kit using measured software demands, desired boot time, portability, budget, and appetite for soldering. Output: selected route, representative parts list, complete cost estimate, and known compromises.
4. Bench prototype: run one pad, then the worst-case voice/effects load; add MIDI clock; add basic controls. Output: audio/timing/resource measurements before enclosure work.
5. Playable enclosure: fit controls and connections, protect wiring, implement storage/update recovery, and use an appropriate external supply. Output: stable standalone instrument.
6. Optional refinement: consider custom PCB, improved enclosure, footswitch, and expanded I/O only after repeated jams reveal a need.

The first assembly should favor prebuilt audio/MIDI modules, documented connectors, and a low-voltage external supply. Learn soldering and basic multimeter use on practice components before modifying the working audio prototype. The purpose is a manageable learning sequence; detailed wiring instructions come only after the actual board and interfaces are chosen.

**Hardware acceptance targets.** These are proposed goals, not verified specifications.

| Gate | Required evidence |
| --- | --- |
| Independence | Cold boot, load a preset, receive Elektron Start, and make sound without a computer or network. Suggested boot-to-ready target: 15 seconds; confirm feasibility during architecture selection. |
| Musical equivalence | The portable reference corpus produces matching chord/note events and lock/evolution behavior between software and hardware for the supported generator version. |
| Audio stability | Two-hour worst-case run at 48 kHz with 16 note voices and agreed effects, no underruns/crashes, bounded CPU/memory use, and no thermal throttling that interrupts audio. |
| Audio quality | With the selected output hardware, measure noise, headroom, and startup/shutdown transients; inspect recordings and listen on headphones/monitors. Quantitative output/noise limits must be set after choosing the codec and output stage, before final hardware design. |
| Sync | Each of Digitone II, Digitakt I, and Syntakt can lead a 20-minute jam at 60, 120, and 180 BPM without bar drift. Meet the software PRD's calibrated onset-jitter target; test tempo changes, Stop/Continue, and cable loss. |
| Usability | Mike can generate, freeze, adjust tone/space, save, and restore a session without documentation after one guided session. Display remains readable at the actual playing position. |
| Storage and recovery | Repeated power cycles retain saved sessions; interrupted-save tests preserve a valid prior copy. An unsuccessful update has a documented recovery path and preserves session backups. |
| Transfer | A session can move from Windows to hardware to macOS without losing supported musical settings; unsupported fields produce an explicit notice. |

**Budget and decisions for the hardware conversation.** Set a total spending cap before recommending purchases. The estimate must include compute/board, audio I/O, MIDI circuitry or interface, controls, display, power supply, enclosure, cables, tools, shipping/tax, and allowance for one revision. Board price alone is not a project budget. No cost or build-time estimate is asserted yet.

Discuss whether Mike prefers fast results from assembly or more electronics learning; whether it needs to travel; acceptable enclosure size and boot time; available mixer/interface/headphones; soldering comfort; and whether a footswitch is essential. Select components only after those answers and the software resource measurements are available. Commercial manufacturing and certification are outside this personal-prototype PRD.

**v0.8 software parity update.** A hardware approach should preserve the portable statement/answer planner, independent melody/figure identities, A/B snapshots and their recall priorities, deliberate chord swaps, space control and the extended percussion range. Map performance actions to reachable controls before committing to an enclosure. Snapshot storage is required for parity; on-device WAV recording and MIDI-file export depend on storage and measured processing headroom and remain architecture decisions. No board, wiring, parts purchase or embedded performance claim follows from the Windows implementation. Discuss the build approach and learning path with Mike before hardware work.
