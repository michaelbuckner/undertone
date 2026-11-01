#include "core/Engine.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace undertone;
void expect(bool ok,const char* why){if(!ok){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
void render(Engine& e,int samples){float l[256],r[256];while(samples>0){int n=std::min(samples,256);e.process(l,r,n);samples-=n;}}
void load(Engine& e,const Session& s){Command c;c.action=Action::load;c.session=s;e.command(c);c.action=Action::play;e.command(c);}
int main(){
    // A low positive amount must still produce an audible harmonic decision, in every scale/register.
    for(int key=0;key<12;++key)for(int mode=0;mode<4;++mode)for(int floor:{48,60,84}){
        auto s=makePreset(2);s.music.key=key;s.music.mode=Mode(mode);s.music.floor=floor;s.music.variation=.001f;
        s.phrase=generate(s.music,{},s.seed);s.phrase.chords[0].locked=true;
        auto old=s.phrase;auto copy=s.seed;s.phrase=evolvePhrase(s.music,old,s.seed,1);
        auto repeat=evolvePhrase(s.music,old,copy,1);
        expect(valid(s),"evolution preserves scale and voicing invariants");
        expect(!sameChord(old.chords[1],s.phrase.chords[1]),"positive amount changes next unlocked chord");
        expect(sameChord(old.chords[0],s.phrase.chords[0]),"lock freezes exact notes");
        for(int i=0;i<4;++i)expect(sameChord(repeat.chords[i],s.phrase.chords[i]),"seeded evolution deterministic");
    }
    auto s=makePreset(2);s.bpm=120;s.music.evolve=true;s.music.evolveBars=1;s.music.evolveTarget=1;s.music.variation=.01f;
    Engine e;e.prepare(48000);load(e,s);render(e,96000);
    expect(e.status().evolutions==0,"no early harmonic evolution");render(e,1);
    expect(e.status().evolutions==1,"evolves at one bar rather than whole phrase");
    expect(!sameChord(s.phrase.chords[1],e.session().phrase.chords[1]),"boundary changes sounding slot");
    expect(e.session().tonePhase==0,"chords-only never moves tone phase");
    expect(e.status().lastOnsetSample==96000,"evolved chord articulates at boundary");
    // A manual phrase landing between slower evolution boundaries must not skip the next evolution.
    Engine priority;priority.prepare(48000);auto manual=s;manual.music.slots=2;manual.music.evolveBars=4;load(priority,manual);
    Command queued;queued.action=Action::generate;priority.command(queued);render(priority,192001);
    expect(priority.status().pending,"automatic preview restored after manual phrase applies");
    render(priority,192000);expect(priority.status().evolutions==1,"manual edit does not skip the next automatic boundary");

    // Tone-only leaves the phrase intact; chord locks do not prevent audible sound movement.
    s.music.evolveTarget=2;s.music.variation=1;s.patch=makeTone(3);
    for(auto& chord:s.phrase.chords)chord.locked=true;
    Engine moving,still;moving.prepare(48000);still.prepare(48000);load(moving,s);
    auto frozen=s;frozen.music.evolve=false;load(still,frozen);
    double difference=0;float a[256],b[256],c[256],d[256];
    for(int i=0;i<1000;++i){moving.process(a,b,256);still.process(c,d,256);for(int j=0;j<256;++j)difference+=std::abs(a[j]-c[j]);}
    expect(difference>100,"tone-only produces a substantial audio difference with all chords locked");
    for(int i=0;i<4;++i)expect(sameChord(s.phrase.chords[i],moving.session().phrase.chords[i]),"tone-only preserves chords");
    expect(moving.status().evolutions==0&&moving.session().tonePhase>0,"tone evolution independent of harmonic events");
    Command cmd;cmd.action=Action::evolution;cmd.session=moving.session();cmd.session.music.variation=0;moving.command(cmd);
    const double phase=moving.session().tonePhase;render(moving,96001);
    expect(moving.session().tonePhase==phase&&!moving.status().pending,"zero amount stops both evolution targets immediately");
    cmd.session.music.variation=.6f;cmd.session.music.evolveTarget=0;moving.command(cmd);
    expect(!moving.status().pending,"locked chords do not plan replacements");render(moving,24000);
    expect(moving.session().tonePhase!=phase,"both mode still morphs tone with chord locks");
    cmd.session=moving.session();cmd.session.music.evolve=false;moving.command(cmd);
    expect(!moving.session().music.evolve,"Evolve toggle takes effect without waiting for a phrase");

    // Changing sound is orthogonal to musical state, including pending manual generation.
    const auto before=moving.session();cmd.action=Action::generate;moving.command(cmd);
    cmd.action=Action::patch;cmd.session.patch=makeTone(7);moving.command(cmd);render(moving,1024);
    expect(moving.session().music.key==before.music.key&&moving.session().bpm==before.bpm,"tone selection retains key and tempo");
    for(int i=0;i<4;++i)expect(sameChord(before.phrase.chords[i],moving.session().phrase.chords[i]),"tone selection retains phrase");

    // External evolution is driven by the MIDI tick grid, including a mid-cycle tempo change.
    Engine midi;midi.prepare(48000);s=makePreset(2);s.music.evolve=true;s.music.evolveTarget=1;s.music.evolveBars=1;
    cmd.action=Action::load;cmd.session=s;midi.command(cmd);cmd.action=Action::external;cmd.value=1;midi.command(cmd);
    midi.midi(0xfa,0);double time=0;
    for(int tick=0;tick<=96;++tick){midi.midi(0xf8,time);if(tick<96)expect(midi.status().evolutions==0,"no early MIDI evolution");time+=tick<48?1.0/48:1.0/36;render(midi,100);}
    expect(midi.status().evolutions==1,"MIDI evolution on tick 96 regardless of tempo estimate");

    // Every topology and control extreme must stay finite at supported rates, including live switches.
    std::vector<std::vector<float>> sounds;
    for(int rate:{44100,48000,96000})for(int model=0;model<4;++model)for(bool extreme:{false,true}){
        Engine synth;synth.prepare(rate);auto sound=makePreset(2);sound.patch.model=ToneModel(model);
        sound.patch.attack=.005f;sound.music.rhythm=Rhythm::eighth;
        if(extreme){sound.patch.brightness=1;sound.patch.resonance=1;sound.patch.texture=1;sound.patch.filterEnv=1;sound.patch.drive=1;sound.patch.space=1;sound.patch.chorus=1;sound.patch.release=8;}
        load(synth,sound);double energy=0;float peak=0;std::vector<float> signature;
        for(int at=0;at<rate*3;at+=256){synth.process(a,b,256);for(int n=0;n<256;++n){
            expect(std::isfinite(a[n])&&std::isfinite(b[n])&&std::abs(a[n])<=1&&std::abs(b[n])<=1,"finite limited audio at parameter extremes");
            energy+=a[n]*a[n]+b[n]*b[n];peak=std::max(peak,std::abs(a[n]));
        }if(rate==48000&&!extreme)signature.insert(signature.end(),a,a+256);}
        expect(energy>1&&peak>.005f,"each topology produces useful output energy");
        if(!signature.empty())sounds.push_back(std::move(signature));
        for(int nextModel=0;nextModel<4;++nextModel){cmd.action=Action::patch;cmd.session=synth.session();cmd.session.patch.model=ToneModel(nextModel);synth.command(cmd);
            float previous=0;for(int block=0;block<20;++block){synth.process(a,b,256);for(int n=0;n<256;++n){expect(std::isfinite(a[n])&&std::abs(a[n])<=1,"live engine switching finite");if(n||block)expect(std::abs(a[n]-previous)<.8f,"no full-scale switching discontinuity");previous=a[n];}}}
    }
    for(size_t i=0;i<sounds.size();++i)for(size_t j=0;j<i;++j){double delta=0;for(size_t k=0;k<sounds[i].size();++k)delta+=std::abs(sounds[i][k]-sounds[j][k]);expect(delta/sounds[i].size()>.003,"different architectures produce distinct waveforms on the same phrase");}
    std::cout<<"PASS evolution timing/targets/locks/determinism; audible tone motion; 4 engines at 3 rates and extremes; live switches\n";
}
