#include "core/Engine.h"
#include "app/InterfaceScale.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace undertone;
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
int main(){
    check(interfaceScale(0,1920,1080)==1&&interfaceScale(0,2560,1440)==1.5f&&interfaceScale(0,3840,2160)==2,"automatic logical display sizes");
    check(interfaceScale(0,1280,720)==1&&interfaceScale(0,7680,2160)==2,"scale bounded on small and ultrawide displays");
    for(int choice=1;choice<=5;++choice)check(interfaceScale(choice,1280,720)==interfaceScale(choice,3840,2160),"manual choice independent of display");
    for(int preset=0;preset<presetCount;++preset){auto s=makePreset(preset);check(valid(s),"factory state valid");
        check(s.detail.motif>0&&s.detail.breathe&&s.detail.phrasing,"every factory arrangement has a lead and breaths");
        MelodyContext context;int lead=0,figure=0;int sections[4]{};int previous=-1;int largestLeap=0;
        for(int step=0;step<256;++step){const auto events=detailStep(s,step,context),repeat=detailStep(s,step,context);check(events.count==repeat.count,"repeatable event count");
            const auto& chord=s.phrase.chords[(step/(s.music.bars*8))%s.music.slots];
            int voices=0;
            for(int n=0;n<events.count;++n){const auto& e=events.events[n];check(e.note==repeat.events[n].note&&e.velocity==repeat.events[n].velocity,"repeatable event identity");
                check(e.note>=s.music.floor&&e.note<=108&&e.velocity>0&&e.velocity<=1,"bounded register and dynamics");
                if(e.role==1){++lead;++voices;++sections[(step/64)%4];
                    const bool tone=chordContains(chord,e.note);
                    if(step%2==0)check(tone,"accent lands on harmony");
                    if(context.previousPassing)check(tone&&std::abs(e.note-context.previousNote)<=2,"passing tension resolves");
                    if(previous>=0)largestLeap=std::max(largestLeap,std::abs(e.note-previous));previous=e.note;
                    context.previousNote=e.note;context.previousPassing=!tone;
                    if(s.detail.style!=PhraseStyle::classic){check(e.note<=s.music.floor+22,"new song lead stays in a compact register");
                        if(s.detail.style==PhraseStyle::pocket)check(e.gateBeats<=.7f,"pocket lead articulates short gestures");}
                }else ++figure;
            }
            if(s.detail.style!=PhraseStyle::classic&&voices)check(events.count==1,"supporting figure leaves lead attack clear");
        }
        check(lead>=10&&lead<=32&&figure>0,"complete form has bounded phrases and supporting replies");
        check(largestLeap<=7,"factory phrase includes smooth loop return");
        if(s.music.composition!=Composition::song)check(sections[3]==0,"sparse form withdraws melody in final eight bars");
        std::cout<<presetName(preset)<<": lead="<<lead<<" figure="<<figure<<" largest leap="<<largestLeap<<'\n';
    }
    for(int preset:{10,11})for(auto style:{PhraseStyle::lyrical,PhraseStyle::pocket}){
        const auto reference=makePreset(preset);auto changed=reference;changed.detail.style=style;
        for(int step=0;step<256;++step){const auto a=detailStep(reference,step),b=detailStep(changed,step);check(a.count==b.count,"Song style inactive in Studies");
            for(int i=0;i<a.count;++i)check(a.events[i].note==b.events[i].note&&a.events[i].velocity==b.events[i].velocity&&a.events[i].gateBeats==b.events[i].gateBeats,"Studies ignores Song style");}
    }
    // Stress both new styles across keys, high floors and root omission, with contextual planning.
    for(auto style:{PhraseStyle::lyrical,PhraseStyle::pocket})for(int key=0;key<12;++key)for(int floor:{48,60,84}){
        auto s=makePreset(2);s.detail.style=style;s.music.key=key;s.music.floor=floor;s.phrase=generate(s.music,{},s.seed);MelodyContext context;
        for(int step=0;step<128;++step){const auto events=detailStep(s,step,context);
            for(int i=0;i<events.count;++i){const auto& e=events.events[i];check(e.note>=floor&&e.note<=108,"transposed register bounded");
                const auto& chord=s.phrase.chords[(step/8)%4];check(e.note%12!=chord.root,"root omission leaves bassist room");
                if(e.role){const bool tone=chordContains(chord,e.note);if(context.previousPassing)check(tone&&std::abs(e.note-context.previousNote)<=2,"transposed passing resolution");context.previousNote=e.note;context.previousPassing=!tone;}}
        }
    }
    std::cout<<"PASS factory phrasing, register, articulation, rests and interface scaling\n";
}
