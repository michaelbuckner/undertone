#include "core/Engine.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace undertone;
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
DetailEvent melody(const Session& s,int64_t step,const MelodyContext& c={}){
    const auto events=detailStep(s,step,c);for(int i=0;i<events.count;++i)if(events.events[i].role==1)return events.events[i];return {-1};
}
int main(){
    int passing=0,changedAhead=0,changedQueued=0,seedChanges=0;
    for(int seed=1;seed<=32;++seed){auto s=makePreset(8,uint32_t(seed));s.detail.breathe=false;s.detail.seed=uint32_t(seed);
        s.music.bars=1;s.music.slots=4;s.music.rootless=seed%2;s.music.floor=seed%3==0?84:60;
        s.phrase=generate(s.music,{},s.seed);MelodyContext c;
        for(int step=0;step<96;++step){auto event=melody(s,step,c);if(event.note<0)continue;
            const auto& chord=s.phrase.chords[(step/8)%4];const bool tone=chordContains(chord,event.note);
            check(event.note>=s.music.floor+7&&event.note<=108,"melody register");
            if(step%2==0||step%32==27)check(tone,"accent and ending must land on a chord tone");
            if(c.previousPassing)check(tone&&std::abs(event.note-c.previousNote)<=2,"passing note must resolve by step");
            if(c.previousNote>=0)check(std::abs(event.note-c.previousNote)<=7,"smooth movement including loop return");
            if(!tone)++passing;
            c.previousNote=event.note;c.previousPassing=!tone;
        }
        auto altered=s;auto future=generate(s.music,{},altered.seed);altered.phrase.chords[1]=future.chords[2];altered.phrase.chords[2]=future.chords[3];
        for(int step:{0,3,6}){check(sameChord(s.phrase.chords[0],altered.phrase.chords[0]),"fixture current chord unchanged");
            changedAhead+=melody(s,step).note!=melody(altered,step).note;
            MelodyContext queued;queued.upcoming=&altered;queued.changeStep=8;
            changedQueued+=melody(s,step).note!=melody(s,step,queued).note;
        }
        auto variant=s;variant.detail.seed=nextMotifSeed(s.detail.seed);
        for(int step:{0,3,6,10,15,19,22,27})seedChanges+=melody(s,step).note!=melody(variant,step).note;
    }
    check(changedAhead>0&&changedQueued>0,"future harmony must causally change notes before the chord change");
    std::cout<<"Phrase counters: passing="<<passing<<" seed changes="<<seedChanges<<" future="<<changedAhead<<" queued="<<changedQueued<<'\n';
    check(passing>0&&seedChanges>0,"retain passing motion and seed-dependent motif identity");
    for(int mode=0;mode<4;++mode)for(int bars:{1,2,4})for(int slots:{2,4,8}){
        auto state=makePreset(9);state.music.mode=Mode(mode);state.music.bars=bars;state.music.slots=slots;
        state.phrase=generate(state.music,{},state.seed);state.detail.breathe=true;MelodyContext context;
        for(int step=0;step<slots*bars*16;++step){const auto e=melody(state,step,context);if(e.note<0)continue;
            const auto& chord=state.phrase.chords[(step/(bars*8))%slots];const bool tone=chordContains(chord,e.note);
            if(step%2==0||step%32==27)check(tone,"accent landing across modes and harmonic paces");
            if(context.previousPassing)check(tone&&std::abs(e.note-context.previousNote)<=2,"passing resolution across form and loop");
            context.previousNote=e.note;context.previousPassing=!tone;
        }
    }
    // Engine integration: pending chord edits and new phrase lengths feed the same forecast used by the planner.
    {
        Engine engine;engine.prepare(48000);Command command;command.action=Action::load;command.session=makePreset(8);command.session.detail.breathe=false;engine.command(command);
        command.action=Action::external;command.value=1;engine.command(command);engine.midi(0xfa,0);MelodyContext expectedContext;
        for(int tick=0;tick<800;++tick){
            if(tick==30){command.action=Action::music;command.session=engine.session();command.session.music.bars=1;command.session.music.slots=2;engine.command(command);}
            if(tick%12==0){const auto before=engine.status();expectedContext.phraseOrigin=tick>=768?64:0;
                expectedContext.upcoming=before.pending?&before.next:nullptr;expectedContext.changeStep=64;expectedContext.upcomingOrigin=64;
                const auto& active=tick==768?before.next:before.session;
                const auto event=melody(active,tick/12,expectedContext);engine.midi(0xf8,tick/48.);
                if(event.note>=0){check(engine.status().lastMelodyNote==event.note,"engine melody follows queued harmony and phrase origin");
                    expectedContext.previousNote=event.note;const auto now=engine.status();expectedContext.previousPassing=!chordContains(now.session.phrase.chords[now.slot],event.note);}
            }else engine.midi(0xf8,tick/48.);
        }
    }
    // Phrase origin must follow actual transport after a queued phrase-length change.
    auto s=makePreset(9);s.detail.breathe=false;MelodyContext shifted;shifted.phraseOrigin=32;
    for(int step=32;step<64;++step){auto e=melody(s,step,shifted);if(e.note>=0&&step%2==0)
        check(chordContains(s.phrase.chords[((step-32)/(s.music.bars*8))%s.music.slots],e.note),"phrase-relative landing after edit");}
    // A voice selection must never alter pitch, pattern seed, harmony or percussion.
    for(int step=0;step<64;++step){auto a=melody(s,step);for(int voice=0;voice<4;++voice){auto b=s;b.detail.voice=MelodyVoice(voice);auto e=melody(b,step);
        check(e.note==a.note&&e.gateBeats==a.gateBeats,"voice choice changes timbre only");}}
    double roughness[4]{};
    for(int rate:{44100,48000,96000})for(int voice=0;voice<4;++voice){DetailEngine synth;synth.prepare(rate);Detail d;d.motif=1;
        synth.trigger({72,1,.8f,.5f,MelodyVoice(voice),8},88);double energy=0,edge=0;float previous=0;
        for(int frame=0;frame<rate*2;++frame){float l=0,r=0;synth.process(l,r,d,88,true);
            check(std::isfinite(l)&&std::isfinite(r)&&std::abs(l)<1&&std::abs(r)<1,"voice finite at supported rates");
            energy+=l*l;edge+=(l-previous)*(l-previous);previous=l;}
        check(energy>.01,"voice audible");if(rate==48000)roughness[voice]=edge/energy;
        synth.clear();float l=0,r=0;synth.process(l,r,d,88,false);check(l==0&&r==0,"voice panic silence");
    }
    for(int voice=1;voice<4;++voice)check(roughness[voice]<roughness[0]*.9,"soft alternatives reduce normalized high-frequency slope energy");
    DetailEngine retained,cut;retained.prepare(48000);cut.prepare(48000);Detail d;d.motif=d.echo=1;
    const DetailEvent e{72,1,.8f,.5f,MelodyVoice::softSine,8};retained.trigger(e);cut.trigger(e);
    Chord common;common.count=2;common.notes={60,64};Chord changed;changed.count=2;changed.notes={62,65};
    double keepEnergy=0,cutEnergy=0;
    for(int frame=0;frame<48000;++frame){if(frame==9600){retained.harmonize(common);cut.harmonize(changed);}float a=0,b=0,x=0,y=0;
        retained.process(a,b,d,88,true);cut.process(x,y,d,88,true);if(frame>24000){keepEnergy+=a*a;cutEnergy+=x*x;}}
    check(keepEnergy>.001&&cutEnergy<keepEnergy*.001,"incompatible melody and its echoes release; common tone may ring");
    std::cout<<"PASS lookahead: "<<changedAhead<<" earlier notes changed; queued: "<<changedQueued<<"; passing: "<<passing<<"; seed changes: "<<seedChanges<<"\n";
    std::cout<<"Normalized slope energy original/soft/felt/flute: ";for(double value:roughness)std::cout<<value<<' ';std::cout<<'\n';
}
