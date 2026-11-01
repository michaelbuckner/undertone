#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace undertone {
constexpr int maxSlots = 8, maxNotes = 6;
enum class Mode { major, minor, dorian, mixolydian };
enum class Rhythm { sustain, quarter, eighth, syncopated };
enum class ToneModel { warm, ladder, dco, fm };
enum class MelodyVoice { original, softSine, feltKeys, airFlute, feltObject, bowedWire };
enum class Composition { song, still, uneasy };
enum class MotifMode { hold, vary, fresh };
enum class PhraseStyle { classic, lyrical, pocket };
enum class SwapStyle { nearby, tension, surprise };
const char* compositionName(int);
const char* melodyVoiceName(int);
struct Music {
    int key = 2;
    Mode mode = Mode::dorian;
    int slots = 4, bars = 1, complexity = 1, floor = 60, spacing = 0;
    bool rootless = false, evolve = false;
    float variation = 0.2f;
    int evolveTarget = 0, evolveBars = 2; // 0 both, 1 chords, 2 tone
    Rhythm rhythm = Rhythm::sustain;
    Composition composition=Composition::song;
};
struct Chord {
    int degree = 0, root = 0, quality = 0, extension = 0, count = 0;
    std::array<int, maxNotes> notes{};
    bool locked = false, rest = false;
    int color=0; // 0 conventional chord, 1 suspended field, 2 tense field
};
struct Phrase { std::array<Chord, maxSlots> chords{}; };
struct Patch {
    ToneModel model = ToneModel::warm;
    int tone = 0; // 0 custom/original; named tones are independent of character
    float brightness = 0.45f, motion = 0.25f, space = 0.35f, level = 0.45f;
    float attack = 0.7f, release = 2.5f, drive = 0.05f, chorus = 0.25f;
    float highpass = 110.0f, detune = 7.0f;
    float texture = .35f, resonance = .15f, decay = .7f, sustain = .8f, filterEnv = .2f;
};
struct Detail {
    float bed=1, broken=0, motif=0, wear=0, echo=0;
    bool breathe=false;
    uint32_t seed=0x4d4f5449u;
    MelodyVoice voice=MelodyVoice::softSine;
    float surface=0;
    bool phrasing=false;
    MotifMode motifMode=MotifMode::fresh;
    uint32_t variationSeed=0,figureSeed=0x4d4f5449u;
    float room=0; // Give me space: deterministic thinning and longer breaths.
    PhraseStyle style=PhraseStyle::classic; // Explicit opt-in; imported sessions keep their arrangement.
};
struct Percussion {
    bool enabled=false;
    float level=.5f,density=.45f,glitch=.3f,color=.4f;
    uint32_t seed=0x50455243u;
};
struct Cycles {
    bool composition=false, motif=false;
    int compositionBars=16, motifBars=8;
};
struct MusicalState {
    Music music;
    Patch patch;
    Detail detail;
    Percussion percussion;
    Cycles cycles;
    Phrase phrase;
    uint32_t seed = 0x4d494b45u;
    double bpm = 88.0;
    double tonePhase = 0;
    int preset = 0, generatorVersion = 1;
};
struct Scene { bool saved=false; MusicalState state; };
struct Session : MusicalState {
    std::array<Scene,2> scenes{};
    int activeScene=-1;
    bool sceneCycle=false,sceneVariations=false;
    int sceneBars=16;
    uint32_t sceneVariationSeed=0x5343454eu;
    SwapStyle swapStyle=SwapStyle::nearby;
};
const std::array<int, 7>& scale(Mode mode);
uint32_t randomNext(uint32_t& state) noexcept;
Phrase generate(const Music&, const Phrase&, uint32_t&, bool varyOnly = false) noexcept;
Phrase evolvePhrase(const Music&, const Phrase&, uint32_t&, int preferredSlot) noexcept;
Phrase replaceChord(const Music&, const Phrase&, uint32_t&, int slot) noexcept;
Phrase chooseChord(const Session&,uint32_t&,int slot,SwapStyle) noexcept;
bool storeScene(Session&,int) noexcept;
bool recallScene(Session&,int,bool vary=false) noexcept;
void advanceMotif(Detail&,bool forceNew=false) noexcept;
void alternateComposition(Session&) noexcept;
bool validCycles(const Cycles&) noexcept;
const char* modelName(int);
const char* toneName(int);
Patch makeTone(int);
std::string chordName(const Chord&);
const char* presetName(int);
Session makePreset(int index, uint32_t seed = 0x4d494b45u);
bool valid(const Session&) noexcept;
bool sameChord(const Chord&, const Chord&) noexcept;
constexpr int presetCount=12;
}
