#include "core/Engine.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace undertone;
void check(bool value,const char* message){if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
void render(Engine& e,int frames){float l[256],r[256];while(frames>0){const int n=std::min(frames,256);e.process(l,r,n);for(int i=0;i<n;++i)check(std::isfinite(l[i])&&std::abs(l[i])<=1&&std::isfinite(r[i]),"finite audio");frames-=n;}}
int main(){
    for(int style=1;style<=2;++style)for(int key=0;key<12;++key)for(int mode=0;mode<4;++mode)
    for(int floor:{48,60,84})for(bool rootless:{false,true})for(int complexity=0;complexity<3;++complexity)for(int spacing:{0,1}){
        auto s=makePreset(10);s.music.composition=Composition(style);s.music.key=key;s.music.mode=Mode(mode);s.music.floor=floor;
        s.music.rootless=rootless;s.music.complexity=complexity;s.music.spacing=spacing;auto seed=s.seed;s.phrase=generate(s.music,{},s.seed);auto repeated=generate(s.music,{},seed);
        check(valid(s),"field voicings valid for key/mode/floor/root omission");
        for(int i=0;i<s.music.slots;++i){const auto& chord=s.phrase.chords[i];check(chord.color==style&&sameChord(chord,repeated.chords[i]),"deterministic field colors");
            check(chord.degree!=4&&chord.degree!=1&&chord.degree!=2,"field generation avoids functional dominant chains");
            if(rootless)for(int n=0;n<chord.count;++n)check(chord.notes[n]%12!=chord.root,"fields omit root");}
        s.phrase.chords[1].locked=true;s.music.variation=1;auto evolved=evolvePhrase(s.music,s.phrase,s.seed,0);
        check(sameChord(s.phrase.chords[1],evolved.chords[1])&&!sameChord(s.phrase.chords[0],evolved.chords[0]),"field evolution preserves lock and changes target");
    }
    for(int preset:{10,11}){auto s=makePreset(preset);int melody[4]{},figures[4]{};int longest=0,gap=0;MelodyContext context;
        for(int step=0;step<256;++step){const auto events=detailStep(s,step,context);bool hit=false;
            for(int n=0;n<events.count;++n){const auto& event=events.events[n];check(event.note>=s.music.floor&&event.note<=108,"study register bounds");
                if(event.role==1){++melody[step/64];hit=true;context.previousNote=event.note;context.previousPassing=!chordContains(s.phrase.chords[(step/(s.music.bars*8))%s.music.slots],event.note);}
                else ++figures[step/64];}
            gap=hit?0:gap+1;longest=std::max(longest,gap);
        }
        check(melody[0]>=3&&melody[1]>=3&&melody[2]>=3&&melody[3]==0,"gesture returns then withdraws for eight bars");
        check(figures[0]==0&&figures[1]>0&&figures[2]>0,"foreground/background roles enter deliberately");
        check(longest>=64,"extended melodic negative space");
        auto opening=compositionFrame(s,0),pressure=compositionFrame(s,64),ending=compositionFrame(s,127.99);
        check(pressure.bed>opening.bed*2&&pressure.surface>opening.surface*2,"long-form dynamic contrast");
        check(std::abs(ending.bed-opening.bed)<.001&&std::abs(ending.surface-opening.surface)<.001,"smooth arc return");
        for(int boundary:{32,64,96,128}){auto a=compositionFrame(s,boundary-.0001),b=compositionFrame(s,boundary+.0001);
            check(std::abs(a.bed-b.bed)<.001&&std::abs(a.surface-b.surface)<.001,"continuous bed and surface section transitions");}
        s.detail.breathe=false;check(compositionFrame(s,100).bed==1,"form toggle preserves manual layer balance");
        std::cout<<preset<<" melody by section: "<<melody[0]<<','<<melody[1]<<','<<melody[2]<<','<<melody[3]<<" figures: "<<figures[0]<<','<<figures[1]<<','<<figures[2]<<','<<figures[3]<<'\n';
    }
    for(int rate:{44100,48000,96000})for(int preset:{10,11}){Engine engine;engine.prepare(rate);Command c;c.action=Action::load;c.session=makePreset(preset);engine.command(c);c.action=Action::play;engine.command(c);render(engine,rate*8);
        check(engine.status().detailNotes>0,"study sounds at supported rates");c.action=Action::stop;engine.command(c);auto notes=engine.status().detailNotes;render(engine,rate);check(engine.status().detailNotes==notes,"stop schedules no further gestures");
        c.action=Action::panic;engine.command(c);float l[256],r[256];engine.process(l,r,256);for(float value:l)check(value==0,"panic clears modal instruments and surface");}
    // Exact shared-grid transport remains the source of the slow form, including external MIDI.
    {
        Engine a,b;a.prepare(48000);b.prepare(48000);Command c;c.action=Action::load;c.session=makePreset(10);
        c.session.music.evolve=false;c.session.detail={};c.session.music.bars=1;
        for(auto& chord:c.session.phrase.chords)chord=c.session.phrase.chords[0];a.command(c);c.session.music.bars=4;b.command(c);
        c.action=Action::play;a.command(c);b.command(c);float al[256],ar[256],bl[256],br[256];
        for(int block=0;block<1800;++block){a.process(al,ar,256);b.process(bl,br,256);for(int n=0;n<256;++n)check(al[n]==bl[n]&&ar[n]==br[n],"common held field tones do not retrigger at nominal chord boundaries");}
    }
    Engine midi;midi.prepare(48000);Command c;c.action=Action::load;c.session=makePreset(11);midi.command(c);c.action=Action::external;c.value=1;midi.command(c);midi.midi(0xfa,0);
    for(int tick=0;tick<3072;++tick){midi.midi(0xf8,tick/48.);check(midi.status().section==(tick/768)%4,"MIDI form follows actual ticks");}
    midi.midi(0xfc,65);check(!midi.status().playing,"MIDI stop stops form");midi.midi(0xfa,66);midi.midi(0xf8,66.01);check(midi.status().section==0,"MIDI start returns to opening");
    SurfaceEngine surface;surface.prepare(48000);float l=0,r=0;const auto chord=makePreset(10).phrase.chords[0];
    for(int i=0;i<48000;++i){l=r=0;surface.process(l,r,chord,0,true);check(l==0&&r==0,"surface zero adds no signal");}
    std::cout<<"PASS field generation, locks, form, negative space, modal voices, surface, transport\n";
}
