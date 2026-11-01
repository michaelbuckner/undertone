#include "core/Engine.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace undertone;
void check(bool value,const char* message){if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
void render(Engine& engine,int frames){float l[256],r[256];while(frames>0){int n=std::min(frames,256);engine.process(l,r,n);frames-=n;}}
int main(){
    for(uint32_t seed=0;seed<10000;++seed)check(nextMotifSeed(seed)%4!=seed%4,"New motif always chooses a different melodic contour");
    for(int key=0;key<12;++key)for(int mode=0;mode<4;++mode)for(int floor:{48,60,84})for(bool rootless:{false,true}){
        auto s=makePreset(8);s.detail.style=PhraseStyle::classic;s.music.key=key;s.music.mode=Mode(mode);s.music.floor=floor;s.music.rootless=rootless;
        s.phrase=generate(s.music,{},s.seed);
        for(int step=0;step<128;++step){const auto events=detailStep(s,step);const auto repeat=detailStep(s,step);
            check(events.count<=2&&events.count==repeat.count,"bounded repeatable scheduling");
            for(int i=0;i<events.count;++i){const auto& event=events.events[i];bool inScale=false;
                for(int pc:scale(s.music.mode))inScale|=(event.note-s.music.key+120)%12==pc;
                check(inScale&&event.note>=floor&&event.note<=108,"all added notes remain in scale and above bass floor");
                check(event.note==repeat.events[i].note&&event.velocity==repeat.events[i].velocity,"same seed yields same phrase detail");
                if(rootless)check(event.note%12!=s.phrase.chords[(step/(s.music.bars*8))%s.music.slots].root,"new layers respect root omission");
                if(s.detail.breathe&&step>=96)check(event.role==0,"last four bars leave room after the melody");
                if(s.detail.breathe&&step>=64&&step<96)check(event.role==1,"middle passage drops broken-chord figure");
            }
        }
    }
    auto s=makePreset(8);s.detail.style=PhraseStyle::classic;s.detail.breathe=false;
    for(int step:{0,3,8,14,18,22}){
        auto first=detailStep(s,step),answer=detailStep(s,step);
        int a=-1,b=-1;for(int n=0;n<first.count;++n)if(first.events[n].role==1)a=first.events[n].note;
        for(int n=0;n<answer.count;++n)if(answer.events[n].role==1)b=answer.events[n].note;
        check(a>=0&&a==b,"planned motif is deterministic for the same harmonic context");
    }
    for(int rate:{44100,48000,96000}){
        Engine rich,plain;rich.prepare(rate);plain.prepare(rate);Command c;c.action=Action::load;c.session=s;rich.command(c);
        c.session.detail={};plain.command(c);c.action=Action::play;rich.command(c);plain.command(c);
        double difference=0;float a[256],b[256],x[256],y[256];
        for(int at=0;at<rate*8;at+=256){rich.process(a,b,256);plain.process(x,y,256);for(int n=0;n<256;++n){
            check(std::isfinite(a[n])&&std::abs(a[n])<1&&std::isfinite(b[n]),"rich sketch finite");difference+=std::abs(a[n]-x[n]);}}
        check(rich.status().detailNotes>12&&plain.status().detailNotes==0&&difference>100,"complexity adds actual notes and audio, not only filters");
        const auto old=rich.session().phrase;const auto seed=rich.session().detail.seed;c.action=Action::newMotif;rich.command(c);
        check(rich.session().detail.seed==nextMotifSeed(seed),"manual motif advances current identity");
        for(int i=0;i<4;++i)check(sameChord(old.chords[i],rich.session().phrase.chords[i]),"new motif retains harmonic identity");
        c.action=Action::stop;rich.command(c);const auto stoppedNotes=rich.status().detailNotes;render(rich,rate);
        check(rich.status().detailNotes==stoppedNotes,"stop schedules no new layer notes");
        c.action=Action::panic;rich.command(c);rich.process(a,b,256);for(float value:a)check(value==0,"panic clears added voices and echo tails");
    }
    Engine midi;midi.prepare(48000);Command command;command.action=Action::load;command.session=s;midi.command(command);
    command.action=Action::external;command.value=1;midi.command(command);midi.midi(0xfa,0);
    check(midi.status().detailNotes==0,"MIDI Start waits for first clock for layers");
    uint64_t expected=0;
    for(int tick=0;tick<192;++tick){midi.midi(0xf8,tick/48.0);if(tick%12==0)expected+=detailStep(s,tick/12).count;
        check(midi.status().detailNotes==expected,"layer onsets follow exact MIDI eighth-note ticks");render(midi,32);}
    std::cout<<"PASS motifs, breathing form, deterministic notes, scale/floor/root omission, stereo audio, transport and MIDI grid\n";
}
