#include "core/Engine.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace undertone;
int checks=0;
void check(bool condition,const char* message) { ++checks; if(!condition) { std::cerr<<"FAIL: "<<message<<'\n'; std::exit(1); } }
void render(Engine& e,int samples) { float l[256],r[256]; while(samples>0) { const int n=std::min(256,samples); e.process(l,r,n); samples-=n; } }
int main() {
    for(int key=0;key<12;++key) for(int mode=0;mode<4;++mode) for(int complexity=0;complexity<3;++complexity)
    for(int floor:{48,60,72,84}) for(bool rootless:{false,true}) for(int spacing:{0,1}) {
        Session s; s.music.key=key; s.music.mode=Mode(mode); s.music.complexity=complexity;
        s.music.floor=floor; s.music.rootless=rootless; s.music.spacing=spacing;
        auto seed=s.seed; s.phrase=generate(s.music,{},s.seed); const auto b=generate(s.music,{},seed);
        check(valid(s),"generated scale / note floor / count invariants");
        for(int i=0;i<4;++i) { check(sameChord(s.phrase.chords[i],b.chords[i]),"deterministic generation");
            if(rootless) for(int n=0;n<s.phrase.chords[i].count;++n) check(s.phrase.chords[i].notes[n]%12!=s.phrase.chords[i].root,"root omission"); }
        s.phrase.chords[1].locked=true; auto next=generate(s.music,s.phrase,s.seed);
        check(sameChord(s.phrase.chords[1],next.chords[1]),"locked chord retained");
    }
    for(int i=0;i<presetCount;++i) check(valid(makePreset(i)),"factory preset validity");
    Engine e; e.prepare(48000);
    Command c; c.action=Action::load; c.session=makePreset(2); c.session.bpm=120; e.command(c);
    c.action=Action::play; e.command(c); render(e,1); check(e.status().notesTriggered==1,"first sample onset");
    render(e,95999); check(e.status().notesTriggered==1,"no early second chord");
    render(e,1); check(e.status().notesTriggered==2,"second chord sample-accurate onset");
    check(e.status().slot==1,"second chord selects the next voicing at the boundary");
    auto old=e.session().phrase; c.action=Action::generate; e.command(c);
    check(e.status().pending,"generation is queued while playing");
    for(int i=0;i<4;++i) check(sameChord(old.chords[i],e.session().phrase.chords[i]),"playing phrase remains unchanged");
    c.action=Action::lockAll;c.value=1;e.command(c);check(!e.status().pending,"lock all cancels pending changes");
    c.action=Action::music;c.session=e.session();c.session.music.key=7;e.command(c);
    check(e.status().rejected==1,"locked key change rejected");
    c.action=Action::panic;e.command(c);float l[256],r[256];e.process(l,r,256);
    for(auto v:l)check(v==0,"panic silences tails");
    c.action=Action::external;c.value=1;e.command(c);e.midi(0xf8,0);check(!e.status().playing,"clock alone does not start");
    auto beforeTicks=e.status().notesTriggered;
    e.midi(0xfa,0);check(e.status().playing,"MIDI start");render(e,100);
    check(e.status().notesTriggered==beforeTicks,"MIDI start waits for the first clock before sounding");render(e,23901);
    check(!e.status().playing&&e.status().clock==ClockState::lost,"lost clock stops");
    e.midi(0xf8,1);check(!e.status().playing,"clock recovery requires start");
    e.midi(0xfb,1.1);check(!e.status().playing,"continue cannot resume after lost clock");
    e.midi(0xfa,2);render(e,100);e.midi(0xfc,2.01);check(!e.status().playing,"MIDI stop");
    e.midi(0xfb,2.02);check(e.status().playing,"MIDI continue");
    // Queued key changes must not smuggle an old locked voicing into a new key.
    Engine conflict;conflict.prepare(48000);Command edit;edit.action=Action::play;conflict.command(edit);
    edit.action=Action::music;edit.session=conflict.session();edit.session.music.key=1;conflict.command(edit);
    check(conflict.status().pending,"key edit queued");edit.action=Action::lock;edit.value=0;conflict.command(edit);
    check(!conflict.status().pending&&valid(conflict.session()),"locking cancels conflicting pending key edit");
    // Evolution respects locked slots and reloads the next random state reproducibly.
    Engine evolution; evolution.prepare(48000);Command load;load.action=Action::load;load.session=makePreset(2);
    load.session.bpm=200;load.session.music.slots=2;load.session.music.evolve=true;load.session.music.variation=1;
    load.session.phrase.chords[0].locked=true;evolution.command(load);auto locked=evolution.session().phrase.chords[0];
    auto savedPhrase=evolution.session().phrase;auto savedSeed=evolution.session().seed;
    load.action=Action::play;evolution.command(load);render(evolution,1);
    for(int i=0;i<2;++i)check(sameChord(savedPhrase.chords[i],evolution.session().phrase.chords[i]),"Evolve starts with the loaded realized phrase");
    check(evolution.session().seed==savedSeed,"Play does not consume next evolution state prematurely");
    render(evolution,48000*10);
    check(sameChord(locked,evolution.session().phrase.chords[0]),"evolution preserves anchor voicing");check(valid(evolution.session()),"evolved session remains valid");
    load.action=Action::load;load.session.music.evolve=true;evolution.command(load);
    load.action=Action::music;load.session=evolution.session();load.session.music.variation=.3f;evolution.command(load);
    for(int i=0;i<2;++i)check(sameChord(savedPhrase.chords[i],evolution.session().phrase.chords[i]),"Variation edit while stopped retains current phrase");
    load.action=Action::external;load.value=1;evolution.command(load);evolution.midi(0xfa,10);evolution.midi(0xf8,10.01);
    for(int i=0;i<2;++i)check(sameChord(savedPhrase.chords[i],evolution.session().phrase.chords[i]),"MIDI Start also preserves loaded Evolve phrase");
    // Sample scheduling at a fractional samples-per-beat tempo on both supported rates.
    for(int rate:{44100,48000}){
        Engine timing;timing.prepare(rate);Command setup;setup.action=Action::load;setup.session=makePreset(2);setup.session.bpm=88;timing.command(setup);
        setup.action=Action::play;timing.command(setup);uint64_t count=0;
        for(int sample=0;sample<rate*12;++sample){float a,b;timing.process(&a,&b,1);auto state=timing.status();
            if(state.notesTriggered!=count){const double expected=double(count)*rate*60.0/88*4;check(std::abs(double(sample)-std::ceil(expected-1e-7))<=1,"fractional tempo onset accuracy");++count;}}
        check(count>=4,"timing test traverses a full phrase");
    }
    // Full preset audio exercises: output finite, limited, stereo and non-silent.
    for(int p=0;p<presetCount;++p) {
        Engine synth;synth.prepare(48000);Command load;load.action=Action::load;load.session=makePreset(p);synth.command(load);
        load.action=Action::play;synth.command(load);double energy=0,stereo=0;
        for(int block=0;block<375;++block) { synth.process(l,r,256);for(int n=0;n<256;++n) {
            check(std::isfinite(l[n])&&std::isfinite(r[n])&&std::abs(l[n])<=1&&std::abs(r[n])<=1,"finite bounded audio");
            energy+=l[n]*l[n]+r[n]*r[n];stereo+=std::abs(l[n]-r[n]);
        }}
        check(energy>1,"audible preset energy");check(stereo>.01,"stereo output");
    }
    Queue<int,64> queue; std::thread producer([&]{for(int n=0;n<100000;++n)while(!queue.push(n))std::this_thread::yield();});
    for(int n=0;n<100000;++n){int v=-1;while(!queue.pop(v))std::this_thread::yield();check(v==n,"SPSC ordered delivery");}producer.join();
    std::cout<<"PASS "<<checks<<" checks\n";
}
