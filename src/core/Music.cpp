#include "Music.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace undertone {
const std::array<int, 7>& scale(Mode mode) {
    static constexpr std::array<std::array<int, 7>, 4> scales{{
        {{0,2,4,5,7,9,11}}, {{0,2,3,5,7,8,10}},
        {{0,2,3,5,7,9,10}}, {{0,2,4,5,7,9,10}}}};
    return scales[static_cast<size_t>(mode)];
}
uint32_t randomNext(uint32_t& state) noexcept {
    // Specified integer PRNG: independent of standard-library distribution implementations.
    if (state == 0) state = 0x6d2b79f5u;
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return state;
}
bool sameChord(const Chord& a, const Chord& b) noexcept {
    return a.degree == b.degree && a.root == b.root && a.quality == b.quality
        && a.extension == b.extension && a.count == b.count && a.notes == b.notes && a.color==b.color && a.rest==b.rest;
}
static Chord voice(const Music& music, int degree, const Chord& previous) noexcept {
    Chord chord;
    const auto& s = scale(music.mode);
    chord.degree = degree;
    chord.root = (music.key + s[degree]) % 12;
    const int third = (s[(degree+2)%7] - s[degree] + 12) % 12;
    const int fifth = (s[(degree+4)%7] - s[degree] + 12) % 12;
    chord.quality = fifth == 6 ? 2 : (third == 3 ? 1 : 0);
    chord.extension = music.complexity;
    std::array<int, 6> pcs{};
    int count = 0;
    for (int i = music.rootless ? 1 : 0; i < 3 + music.complexity; ++i)
        pcs[count++] = (music.key + s[(degree + i*2)%7]) % 12;
    if(music.composition!=Composition::song){
        chord.color=int(music.composition);count=0;
        const int calm[]{0,4,1,5},tense[]{0,4,5,1};
        const int* offsets=music.composition==Composition::still?calm:tense;
        const int total=std::max(2+music.complexity,music.rootless?3:2);
        for(int i=music.rootless?1:0;i<total;++i)pcs[count++]=(music.key+s[(degree+offsets[i])%7])%12;
    }
    // Search inversions and octave placements with bounded stack storage.
    int best = std::numeric_limits<int>::max();
    for (int inversion = 0; inversion < count; ++inversion) {
        for (int octave = 0; octave < 2; ++octave) {
            std::array<int, 6> notes{};
            int minimum = music.floor + 12*octave;
            for (int n = 0; n < count; ++n) {
                int note = pcs[(n+inversion)%count];
                while (note < minimum) note += 12;
                notes[n] = note;
                minimum = note + 1;
            }
            if (music.spacing && count > 2) {
                notes[1] += 12;
                std::sort(notes.begin(), notes.begin()+count);
            }
            if (notes[count-1] > 108) continue;
            int cost = (notes[count-1] - notes[0]) / 2;
            for (int n = 0; n < count; ++n) {
                int nearest = std::abs(notes[n] - (music.floor + 7 + n*3));
                if (previous.count) {
                    nearest = 127;
                    for (int p = 0; p < previous.count; ++p)
                        nearest = std::min(nearest, std::abs(notes[n] - previous.notes[p]));
                }
                cost += nearest * 4;
            }
            if (cost < best) { best = cost; chord.notes = notes; chord.count = count; }
        }
    }
    return chord;
}
Phrase replaceChord(const Music& music,const Phrase& old,uint32_t& seed,int slot) noexcept {
    auto result=old;
    if(slot<0||slot>=music.slots||old.chords[slot].locked)return result;
    int degree=(old.chords[slot].degree+1+int(randomNext(seed)%6))%7;
    if(music.composition!=Composition::song){
        const int fields[]{0,3,5,6};int at=0;
        while(at<3&&fields[at]!=old.chords[slot].degree)++at;
        degree=fields[(at+1+randomNext(seed)%3)%4];
    }
    result.chords[slot]=voice(music,degree,old.chords[slot]);
    return result;
}
Phrase chooseChord(const Session& s,uint32_t& seed,int slot,SwapStyle style) noexcept {
    if(slot<0||slot>=s.music.slots||s.phrase.chords[slot].locked)return s.phrase;
    auto result=s.phrase;const auto& old=result.chords[slot];
    const auto& before=result.chords[(slot+s.music.slots-1)%s.music.slots];
    const auto& after=result.chords[(slot+1)%s.music.slots];
    float best=1.e9f;Chord selected=old;
    auto distance=[](const Chord& a,const Chord& b){float cost=0;if(b.rest)return cost;
        for(int i=0;i<a.count;++i){int closest=24;for(int j=0;j<b.count;++j)closest=std::min(closest,std::abs(a.notes[i]-b.notes[j]));cost+=closest;}
        return cost/std::max(1,a.count);};
    for(int degree=0;degree<7;++degree){if(degree==old.degree)continue;
        auto music=s.music;
        if(style==SwapStyle::tension&&music.composition!=Composition::song){music.composition=Composition::uneasy;music.complexity=std::max(1,music.complexity);}
        if(style==SwapStyle::tension&&music.composition==Composition::song)music.complexity=std::max(1,music.complexity);
        auto candidate=voice(music,degree,before);float tension=0;
        for(int i=0;i<candidate.count;++i)for(int j=i+1;j<candidate.count;++j){const int gap=(candidate.notes[j]-candidate.notes[i])%12;
            if(gap==1||gap==11||gap==6)tension+=1;}
        // Preserve nearby motion into AND out of the slot; favor the motif's opening pitch class.
        const int motifDegree=std::array<int,4>{0,4,2,0}[s.detail.seed%4];
        const int melodyPc=(s.music.key+scale(s.music.mode)[motifDegree])%12;
        bool supports=false;for(int i=0;i<candidate.count;++i)supports|=candidate.notes[i]%12==melodyPc;
        float score=distance(candidate,before)+distance(candidate,after)+.6f*distance(candidate,old)+(supports?0.f:1.5f);
        if(style==SwapStyle::tension)score-=4.f*tension;
        if(style==SwapStyle::surprise)score=score*.25f+float(randomNext(seed)%1000)/100.f;
        if(score<best){best=score;selected=candidate;}
    }
    result.chords[slot]=selected;return result;
}
void advanceMotif(Detail& d,bool forceNew) noexcept {
    if(forceNew||d.motifMode==MotifMode::fresh){
        const auto previous=d.seed%4;randomNext(d.seed);d.seed=(d.seed&~3u)|((previous+1+(d.seed>>2)%3)%4);d.variationSeed=0;
    }else if(d.motifMode==MotifMode::vary)randomNext(d.variationSeed);
}
bool storeScene(Session& s,int index) noexcept {
    if(index<0||index>1)return false;
    s.scenes[index].state=static_cast<const MusicalState&>(s);s.scenes[index].saved=true;s.activeScene=index;return true;
}
bool recallScene(Session& s,int index,bool vary) noexcept {
    if(index<0||index>1||!s.scenes[index].saved)return false;
    const auto bpm=s.bpm;const auto cycles=s.cycles;
    static_cast<MusicalState&>(s)=s.scenes[index].state;s.bpm=bpm;s.cycles=cycles;s.activeScene=index;
    if(vary&&s.detail.motifMode!=MotifMode::hold){randomNext(s.sceneVariationSeed);s.detail.variationSeed=s.sceneVariationSeed;}
    return true;
}
void alternateComposition(Session& s) noexcept {
    if(s.music.composition==Composition::song)return;
    s.music.composition=s.music.composition==Composition::still?Composition::uneasy:Composition::still;
    for(int i=0;i<s.music.slots;++i){auto& chord=s.phrase.chords[i];if(chord.locked||chord.rest)continue;
        chord=voice(s.music,chord.degree,chord);}
}
bool validCycles(const Cycles& c) noexcept {
    auto interval=[](int v){return v>=1&&v<=64&&(v&(v-1))==0;};
    return interval(c.compositionBars)&&interval(c.motifBars);
}
Phrase generate(const Music& music, const Phrase& old, uint32_t& seed, bool varyOnly) noexcept {
    static constexpr int templates[6][8] = {
        {0,5,3,4,0,2,3,4}, {0,3,5,4,0,3,1,4}, {1,4,0,5,1,4,0,0},
        {0,0,3,4,0,5,3,4}, {0,6,3,0,0,3,6,4}, {0,3,0,4,0,5,3,4}};
    Phrase result = old;
    const int pattern = static_cast<int>(randomNext(seed) % 6);
    for (int i = 0; i < music.slots; ++i) {
        if (old.chords[i].locked) continue;
        if (varyOnly && static_cast<double>(randomNext(seed)) / 4294967296.0 >= music.variation) continue;
        static constexpr int fields[6][8]={{0,0,3,0,0,5,3,0},{0,0,5,3,0,0,3,0},{0,3,0,0,5,3,0,0},
            {0,0,6,0,3,0,6,0},{0,5,0,0,0,3,5,0},{0,0,3,5,0,0,6,0}};
        int degree = music.composition==Composition::song?templates[pattern][i]:fields[pattern][i];
        if (music.composition==Composition::song&&varyOnly && i != 0 && randomNext(seed)%3 == 0) degree = static_cast<int>(randomNext(seed)%7);
        const auto& previous = i ? result.chords[i-1] : old.chords[music.slots-1];
        result.chords[i] = voice(music, degree, previous);
        result.chords[i].rest=old.chords[i].rest;
    }
    return result;
}
Phrase evolvePhrase(const Music& m, const Phrase& old, uint32_t& seed, int preferredSlot) noexcept {
    if(m.variation<=0) return old;
    auto result=old;
    // Always change the next sounding unlocked chord; amount controls additional changes.
    int chosen=-1;
    for(int n=0;n<m.slots;++n) { const int i=(preferredSlot+n)%m.slots; if(!old.chords[i].locked&&!old.chords[i].rest){chosen=i;break;} }
    if(chosen<0)return old;
    for(int i=0;i<m.slots;++i) {
        if(old.chords[i].locked||old.chords[i].rest)continue;
        if(i!=chosen && double(randomNext(seed))/4294967296.0>=m.variation*.55)continue;
        int degree=(old.chords[i].degree+1+int(randomNext(seed)%6))%7;
        if(m.composition!=Composition::song){const int fields[]{0,3,5,6};int at=0;while(at<3&&fields[at]!=old.chords[i].degree)++at;degree=fields[(at+1+randomNext(seed)%3)%4];}
        result.chords[i]=voice(m,degree,old.chords[i]);
    }
    return result;
}
const char* modelName(int i) {
    static const char* names[]{"Original Warm","Ladder / three VCOs","DCO / pulse + chorus","FM / four operators"};
    return names[std::clamp(i,0,3)];
}
const char* toneName(int i) {
    static const char* names[]{"Original Warm","Ladder / Amber pad","Ladder / Pocket clav","DCO / Silk ensemble","DCO / Dry comp","FM / Felt keys","FM / Glass veil","FM / Ash room","Ladder / Slow fracture"};
    return names[std::clamp(i,0,8)];
}
Patch makeTone(int i) {
    Patch p; p.tone=std::clamp(i,0,8);
    switch(p.tone) {
        case 1:p.model=ToneModel::ladder;p.attack=.8f;p.brightness=.35f;p.texture=.6f;p.space=.5f;p.detune=9;break;
        case 2:p.model=ToneModel::ladder;p.attack=.005f;p.decay=.16f;p.sustain=.22f;p.release=.14f;p.brightness=.18f;p.filterEnv=.75f;p.resonance=.35f;p.space=.07f;p.chorus=.06f;p.texture=.65f;break;
        case 3:p.model=ToneModel::dco;p.attack=.7f;p.chorus=.75f;p.space=.5f;p.texture=.6f;p.brightness=.4f;break;
        case 4:p.model=ToneModel::dco;p.attack=.008f;p.decay=.24f;p.sustain=.3f;p.release=.18f;p.filterEnv=.5f;p.brightness=.3f;p.chorus=.2f;p.space=.08f;break;
        case 5:p.model=ToneModel::fm;p.attack=.008f;p.decay=1.3f;p.sustain=.25f;p.release=1.2f;p.texture=.15f;p.brightness=.35f;p.space=.3f;break;
        case 6:p.model=ToneModel::fm;p.attack=1.3f;p.release=5;p.decay=3;p.sustain=.7f;p.texture=.55f;p.brightness=.42f;p.space=.75f;p.chorus=.5f;break;
        case 7:p.model=ToneModel::fm;p.attack=2.4f;p.release=7;p.decay=4;p.sustain=.65f;p.texture=.8f;p.brightness=.18f;p.motion=.15f;p.space=.85f;p.chorus=.55f;p.drive=.12f;break;
        case 8:p.model=ToneModel::ladder;p.attack=3;p.release=6;p.decay=3;p.sustain=.7f;p.brightness=.12f;p.texture=.7f;p.resonance=.45f;p.space=.8f;p.detune=14;p.motion=.18f;p.drive=.2f;break;
    }
    return p;
}
std::string chordName(const Chord& c) {
    if(c.rest)return "Rest";
    static constexpr const char* names[]{"C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B"};
    std::string name = names[static_cast<size_t>(c.root) % 12];
    if(c.color)return name+(c.color==1?" open":" tension");
    if (c.quality == 1) name += "m";
    if (c.quality == 2) name += c.extension ? "m" : "dim";
    if (c.extension) {
        bool major7 = false;
        for (int n = 0; n < c.count; ++n) major7 |= (c.notes[n]-c.root+120)%12 == 11;
        name += major7 ? (c.extension==2 ? "maj9" : "maj7") : (c.extension==2 ? "9" : "7");
        if (c.quality == 2) name += "b5";
    }
    return name;
}
const char* presetName(int i) {
    static constexpr const char* names[]{"Ambient / Slow light", "Ambient / Glass tide", "Neo-soul / Sunday", "Neo-soul / Velvet", "Funk / Pocket", "Funk / Offbeat", "Dark ambient / Ash room", "Dark ambient / Slow fracture", "Memory / Sun-faded", "Memory / Afterimage", "Studies / Quiet room", "Studies / Fault line"};
    return names[static_cast<size_t>(std::clamp(i,0,presetCount-1))];
}
Session makePreset(int index, uint32_t seed) {
    Session s; s.seed=seed; s.preset=std::clamp(index,0,presetCount-1);
    switch(s.preset) {
        case 0: s.music.bars=4;s.music.composition=Composition::still;s.bpm=72;s.patch=makeTone(0);
            s.patch.attack=3.2f;s.patch.release=4.2f;s.patch.brightness=.24f;s.patch.space=.62f;s.patch.level=.64f;s.patch.highpass=150;
            s.detail={.62f,.2f,.52f,.12f,.22f,true,0x534c4f57u,MelodyVoice::airFlute,.08f};break;
        case 1: s.music.key=7;s.music.mode=Mode::major;s.music.bars=4;s.music.composition=Composition::still;s.bpm=78;s.patch=makeTone(6);
            s.patch.attack=2.6f;s.patch.release=4.8f;s.patch.brightness=.23f;s.patch.texture=.22f;s.patch.space=.66f;s.patch.chorus=.18f;s.patch.level=.66f;
            s.detail={.52f,.24f,.63f,.08f,.32f,true,0x474c4153u,MelodyVoice::feltObject,.1f};break;
        case 2: s.music.complexity=2;s.music.rootless=true;s.bpm=82;s.patch=makeTone(5);
            s.patch.attack=.045f;s.patch.release=1.1f;s.patch.brightness=.29f;s.patch.texture=.16f;s.patch.space=.28f;s.patch.level=.64f;
            s.detail={.68f,.18f,.48f,.12f,.16f,true,0x53554e44u,MelodyVoice::feltKeys};s.detail.style=PhraseStyle::lyrical;break;
        case 3: s.music.key=9;s.music.mode=Mode::minor;s.music.complexity=2;s.music.bars=2;s.music.rootless=true;s.bpm=76;s.patch=makeTone(1);
            s.patch.attack=.32f;s.patch.release=1.8f;s.patch.brightness=.22f;s.patch.chorus=.28f;s.patch.space=.38f;s.patch.level=.61f;
            s.detail={.62f,.22f,.48f,.2f,.24f,true,0x56454c56u,MelodyVoice::softSine};s.detail.style=PhraseStyle::lyrical;break;
        case 4: s.music.mode=Mode::mixolydian;s.music.rhythm=Rhythm::syncopated;s.music.rootless=true;s.bpm=104;s.patch=makeTone(2);
            s.patch.release=.22f;s.patch.space=.12f;s.patch.level=.68f;
            s.detail={.82f,.14f,.36f,.08f,.08f,true,0x504f434bu,MelodyVoice::feltKeys};s.detail.style=PhraseStyle::pocket;break;
        case 5: s.music.key=7;s.music.rhythm=Rhythm::syncopated;s.music.rootless=true;s.music.complexity=2;s.bpm=110;s.patch=makeTone(4);
            s.patch.brightness=.4f;s.patch.release=.28f;s.patch.space=.18f;s.patch.level=.72f;
            s.detail={.78f,.2f,.4f,.16f,.12f,true,0x4f464642u,MelodyVoice::softSine};s.detail.style=PhraseStyle::pocket;break;
        case 6: s.music.mode=Mode::minor;s.music.bars=4;s.music.composition=Composition::uneasy;s.bpm=64;s.patch=makeTone(7);
            s.patch.attack=3.4f;s.patch.release=5.2f;s.patch.brightness=.12f;s.patch.texture=.38f;s.patch.space=.7f;s.patch.drive=.08f;s.patch.level=.64f;
            s.detail={.58f,.18f,.58f,.24f,.24f,true,0x41534852u,MelodyVoice::bowedWire,.48f};break;
        case 7: s.music.key=5;s.music.mode=Mode::minor;s.music.bars=4;s.music.spacing=1;s.music.composition=Composition::uneasy;s.bpm=70;s.patch=makeTone(8);
            s.patch.attack=2.8f;s.patch.release=4.6f;s.patch.brightness=.16f;s.patch.resonance=.22f;s.patch.space=.64f;s.patch.level=.59f;
            s.detail={.56f,.3f,.55f,.32f,.3f,true,0x46524143u,MelodyVoice::feltObject,.34f};break;
        case 8: s.music.key=5;s.music.mode=Mode::major;s.music.bars=2;s.bpm=82;s.patch=makeTone(3);
            s.patch.attack=.8f;s.patch.brightness=.24f;s.patch.chorus=.3f;s.patch.space=.4f;s.patch.level=.65f;
            s.detail={.48f,.27f,.52f,.42f,.3f,true,0x63726565u,MelodyVoice::feltKeys};s.detail.style=PhraseStyle::lyrical;break;
        case 9: s.music.key=2;s.music.mode=Mode::minor;s.music.bars=2;s.bpm=76;s.patch=makeTone(1);
            s.patch.attack=1.3f;s.patch.brightness=.17f;s.patch.space=.48f;s.patch.level=.65f;
            s.detail={.45f,.22f,.48f,.52f,.38f,true,0x74726566u,MelodyVoice::airFlute};s.detail.style=PhraseStyle::lyrical;break;
        case 10:s.music.key=2;s.music.mode=Mode::minor;s.music.bars=4;s.music.composition=Composition::still;s.bpm=68;s.patch=makeTone(6);
            s.patch.attack=3.8f;s.patch.brightness=.13f;s.patch.texture=.12f;s.patch.space=.7f;s.patch.chorus=.08f;s.patch.level=.7f;s.patch.highpass=130;
            s.detail={.48f,.28f,.78f,.18f,.25f,true,0x51554945u,MelodyVoice::feltObject,.24f};
            s.music.evolve=true;s.music.evolveTarget=2;s.music.variation=.12f;s.music.evolveBars=16;break;
        case 11:s.music.key=5;s.music.mode=Mode::minor;s.music.bars=4;s.music.composition=Composition::uneasy;s.bpm=74;s.patch=makeTone(7);
            s.patch.attack=2.8f;s.patch.brightness=.16f;s.patch.texture=.46f;s.patch.space=.72f;s.patch.chorus=.12f;s.patch.level=.68f;s.patch.drive=.09f;
            s.detail={.56f,.38f,.7f,.32f,.35f,true,0x4641554cu,MelodyVoice::bowedWire,.72f};
            s.music.evolve=true;s.music.evolveTarget=2;s.music.variation=.2f;s.music.evolveBars=16;break;
    }
    if(s.preset<10){
        s.music.evolve=true;s.music.evolveTarget=2;s.music.variation=s.preset>=6?.16f:.1f;s.music.evolveBars=16;
    }
    s.phrase=generate(s.music,{},s.seed);
    if((s.preset>=2&&s.preset<=5)||s.preset==8||s.preset==9){
        // Designed loops, voiced through the return as well as each forward transition.
        static constexpr int progressions[6][4]={{0,3,1,4},{0,5,3,4},{0,0,3,4},{0,3,0,6},{0,5,3,4},{0,5,3,4}};
        const auto& degrees=progressions[s.preset>=8?s.preset-4:s.preset-2];
        for(int pass=0;pass<2;++pass)for(int i=0;i<4;++i)s.phrase.chords[i]=voice(s.music,degrees[i],s.phrase.chords[(i+3)%4]);
    }
    if(s.preset==9){s.percussion.density=.55f;s.percussion.glitch=.5f;s.percussion.color=.3f;s.percussion.seed=0x53484144u;}
    s.detail.figureSeed=s.detail.seed;
    s.detail.phrasing=true;
    return s;
}
const char* melodyVoiceName(int index) {
    static constexpr const char* names[]{"Original / Hollow","Soft sine","Felt keys","Air flute","Felt object","Bowed wire"};
    return names[std::clamp(index,0,5)];
}
const char* compositionName(int index){static constexpr const char* names[]{"Song / familiar","Still / spacious","Uneasy / sparse"};return names[std::clamp(index,0,2)];}
static bool validState(const MusicalState& s) noexcept {
    if(!validCycles(s.cycles))return false;
    const auto& m=s.music; const auto& p=s.patch;
    auto range=[](double v,double a,double b){return std::isfinite(v)&&v>=a&&v<=b;};
    if(m.key<0||m.key>11||int(m.mode)<0||int(m.mode)>3||int(m.rhythm)<0||int(m.rhythm)>3
       ||(m.slots!=2&&m.slots!=4&&m.slots!=8)||(m.bars!=1&&m.bars!=2&&m.bars!=4)
       ||m.complexity<0||m.complexity>2||m.floor<48||m.floor>84||m.spacing<0||m.spacing>1
       ||!range(m.variation,0,1)||!range(s.bpm,40,200)||s.preset<0||s.preset>=presetCount||s.generatorVersion!=1
       ||m.evolveTarget<0||m.evolveTarget>2||(m.evolveBars!=1&&m.evolveBars!=2&&m.evolveBars!=4&&m.evolveBars!=8&&m.evolveBars!=16&&m.evolveBars!=32)
       ||!range(s.tonePhase,0,1)||int(p.model)<0||int(p.model)>3||p.tone<0||p.tone>8) return false;
    for(float v:{p.brightness,p.motion,p.space,p.level,p.drive,p.chorus,p.texture,p.resonance,p.sustain,p.filterEnv}) if(!range(v,0,1)) return false;
    if(!range(p.decay,.05f,8))return false;
    for(float value:{s.detail.bed,s.detail.broken,s.detail.motif,s.detail.wear,s.detail.echo})if(!range(value,0,1))return false;
    if(int(s.detail.style)<0||int(s.detail.style)>2)return false;
    if(int(s.detail.voice)<0||int(s.detail.voice)>5||int(m.composition)<0||int(m.composition)>2||!range(s.detail.surface,0,1))return false;
    if(!range(s.percussion.level,0,2)||!range(s.detail.room,0,1)||int(s.detail.motifMode)<0||int(s.detail.motifMode)>2)return false;
    for(float value:{s.percussion.density,s.percussion.glitch,s.percussion.color})if(!range(value,0,1))return false;
    if(!range(p.attack,.005f,5)||!range(p.release,.05f,8)||!range(p.highpass,20,500)||!range(p.detune,0,25)) return false;
    for(int i=0;i<m.slots;++i) {
        const auto& c=s.phrase.chords[i];
        if(c.count<2||c.count>6||c.root<0||c.root>11||c.degree<0||c.degree>6||c.quality<0||c.quality>2||c.extension<0||c.extension>2||c.color<0||c.color>2) return false;
        for(int n=0;n<c.count;++n) {
            if(c.notes[n]<m.floor||c.notes[n]>108||(n>0&&c.notes[n]<=c.notes[n-1])) return false;
            bool inScale=false; for(int pitch:scale(m.mode)) inScale |= (c.notes[n]-m.key+120)%12==pitch;
            if(!inScale) return false;
        }
    }
    return true;
}
bool valid(const Session& s) noexcept {
    if(!validState(s)||s.activeScene< -1||s.activeScene>1||s.sceneBars<1||s.sceneBars>64||(s.sceneBars&(s.sceneBars-1))||int(s.swapStyle)<0||int(s.swapStyle)>2)return false;
    if(s.activeScene>=0&&!s.scenes[s.activeScene].saved)return false;
    for(const auto& scene:s.scenes)if(scene.saved&&!validState(scene.state))return false;
    return true;
}
}
