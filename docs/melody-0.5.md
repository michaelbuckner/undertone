# Melody phrasing and voices — v0.5.0

The melody now considers upcoming harmony before selecting a note. This addresses the previous implementation's fixed scale contour, which only adjusted its final note to the current chord. The recurring rhythmic shape and seed still supply identity, but pitches adapt to the progression.

## Playing

In Arrangement, choose **Melody voice** beside New motif:

| Voice | Character |
| --- | --- |
| Original / Hollow | The earlier, brighter oscillator character. |
| Soft sine | Rounded fundamental with a small second harmonic and gentler attack; the new default. |
| Felt keys | A soft struck tone with upper partials that fade quickly. |
| Air flute | Slower onset, light harmonics and subtle amplitude movement. |

These are synthesized voices, not samples or acoustic instrument models. A voice change affects newly triggered melody notes; existing notes finish in their original voice. It keeps the melody seed, note decisions, progression, broken-chord sound and percussion. The Melody slider still controls its level. New motif changes the melodic preferences and broken-chord pattern together, as before.

## Phrasing behavior

At each melody onset, a bounded backward search evaluates that note and the next 32 melody events. This covers at least sixteen bars and several upcoming chords even at the slowest harmonic pace. The search considers current harmony, the known next queued progression/evolution change, register, recurring contour preferences, movement between notes and the wrap back through the progression. It revises its forecast as harmony changes; it cannot know uncommitted future user edits or later, not-yet-generated Evolve choices.

Accented notes and motif endings land on chord tones. Weak offbeats may use scale passing notes when they can resolve by step to a chord tone. Movement costs discourage large leaps, and the previous played note anchors each new decision. Root omission and the bass note floor still apply. The fixed rhythmic motif and alternating ending remain recognizable; identical pitches are no longer forced over different chords. The chord generator itself is unchanged.

Notes that do not belong to an upcoming chord receive a shorter gate. On an actual chord change, incompatible melody voices release over approximately 35 ms while common tones can continue. Melody echoes are now two quiet, pitch-specific taps attached to each voice, so they obey the same harmonic release. Broken chords retain their feedback delay; the shared room/reverb can retain a diffuse tail. This reduces conflicting pitched overlap without pretending that every effect tail disappears instantly.

The planner uses fixed arrays and no allocation. It runs at melody onsets rather than on every audio sample. It is a musical heuristic, not a guarantee of satisfying composition; listening with bass remains the acceptance test.

## Sessions and compatibility

Schema 5 saves the melody voice. Schemas 1–4 import with Original / Hollow selected; their saved chords, seeds and mixes remain intact. The new phrasing and melody-tail handling apply to imported sessions too, so existing melody-enabled sessions will sound different. Older app versions cannot read schema 5 saves. Retain the original session and older app if you want the earlier result.

## Auditions

`dist/Undertone-0.5.0-audio` contains four Sun-faded voice comparisons and an Afterimage example with Felt keys. Each is 52 seconds with percussion enabled. All use the same preview gain of 5, the same endpoint fades and the app's actual engine. The four Sun-faded files have the same generated pitches, harmony and rhythm; only the melody timbre differs. The voices are deliberately not individually loudness-normalized, so the backing stays at a consistent level.

Developer command: `render_sketch output.wav 8 0 1 2 5` selects Sun-faded, layered arrangement, percussion on, Felt keys and a fixed preview gain. Voice IDs are 0 Original, 1 Soft sine, 2 Felt keys, 3 Air flute; omitting the final gain retains the previous automatic preview normalization.

See [build status](build-status.md) for test evidence and remaining platform/device gates. Windows is locally verified; the Mac build and physical Elektron timing still need testing on those devices.
