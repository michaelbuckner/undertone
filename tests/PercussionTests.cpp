#include "core/Engine.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace undertone;
void check(bool ok,const char* why){if(!ok){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
void render(Engine& e,int n){float l[256],r[256];while(n>0){int count=std::min(n,256);e.process(l,r,count);n-=count;}}
int main(){
    Percussion p;p.enabled=true;
    for(uint32_t seed=0;seed<1000;++seed)for(float density:{0.f,.5f,1.f})for(float glitch:{0.f,1.f}){
        p.seed=seed;p.density=density;p.glitch=glitch;int anchors=0,optional=0;
        for(int step=0;step<128;++step){const auto a=percussionStep(p,step),b=percussionStep(p,step+128);
            check(a.count<=3&&a.count==b.count,"bounded deterministic four-bar pattern");
            for(int i=0;i<a.count;++i){const auto& hit=a.hits[i];
                check(hit.sound==b.hits[i].sound&&hit.velocity==b.hits[i].velocity&&hit.noiseSeed==b.hits[i].noiseSeed,"noise and accents repeat with seed");
                if(hit.anchor){++anchors;check(step%8==0,"anchors lie exactly on quarter beats");}
                else ++optional;
                if(glitch==0)check(hit.sound!=3,"Glitch zero disables digital bursts");
            }
        }
        check(anchors==16,"every beat remains present at every density/glitch/seed");
        if(density==0&&glitch==0)check(optional==0,"minimal setting is only the stable pulse");
        if(density==1)check(optional>=16,"density adds audible offbeats");
        check(nextPercussionSeed(seed)%4!=seed%4,"New pattern chooses a different offbeat skeleton");
    }
    p.enabled=false;check(percussionStep(p,0).count==0,"disabled schedules no percussion");p.enabled=true;p.level=0;check(percussionStep(p,0).count==0,"zero level schedules no percussion");
    // Timing with irregular buffers and fractional samples per subdivision.
    for(int rate:{44100,48000,96000})for(int bpm:{40,88,200}){
        Engine e;e.prepare(rate);Command c;c.action=Action::load;c.session=makePreset(8);c.session.bpm=bpm;c.session.percussion.enabled=true;
        c.session.percussion.density=1;c.session.percussion.glitch=1;e.command(c);c.action=Action::play;e.command(c);
        uint64_t expected=0;int64_t previous=-1;int sample=0;const int sizes[]{64,127,256,511};
        for(int block=0;sample<rate*5;++block){const int n=std::min(sizes[block%4],rate*5-sample);render(e,n);sample+=n;
            const auto finalStep=int64_t(std::floor((sample-1)*double(bpm)/(60*rate)*8+1e-9));
            for(auto step=previous+1;step<=finalStep;++step)expected+=percussionStep(c.session.percussion,step).count;
            check(e.status().percussionHits==expected,"no missed/extra hits across variable audio buffers");previous=finalStep;
            if(expected){int64_t last=finalStep;while(percussionStep(c.session.percussion,last).count==0)--last;
                const double expectedSample=std::ceil(last*60.0*rate/(bpm*8)-1e-7);
                check(std::abs(double(e.status().lastPercussionSample)-expectedSample)<=1,"percussion subdivision sample accuracy");}
        }
        const auto before=e.session();c.action=Action::percussion;c.session=e.session();c.session.percussion.seed=nextPercussionSeed(c.session.percussion.seed);e.command(c);
        for(int i=0;i<4;++i)check(sameChord(before.phrase.chords[i],e.session().phrase.chords[i]),"new pattern retains harmony");
        check(before.detail.seed==e.session().detail.seed,"new pattern retains melody");
        c.session.percussion.enabled=false;e.command(c);auto count=e.status().percussionHits;render(e,rate);check(e.status().percussionHits==count,"disable stops new hits immediately");
        c.session.percussion.enabled=true;e.command(c);c.action=Action::stop;e.command(c);render(e,rate);check(e.status().percussionHits==count,"Stop stops percussion scheduling");
        c.action=Action::panic;e.command(c);float l[256],r[256];e.process(l,r,256);for(float v:l)check(v==0,"Panic clears percussion tails");
    }
    // MIDI-clock ticks, a tempo change, Stop/Continue and fresh Start.
    Engine midi;midi.prepare(48000);Command c;c.action=Action::load;c.session=makePreset(8);c.session.percussion.enabled=true;
    c.session.percussion.glitch=1;midi.command(c);c.action=Action::external;c.value=1;midi.command(c);midi.midi(0xfa,0);
    check(midi.status().percussionHits==0,"MIDI Start waits for first clock");uint64_t expected=0;double time=0;
    for(int tick=0;tick<192;++tick){midi.midi(0xf8,time);if(tick%3==0)expected+=percussionStep(c.session.percussion,tick/3).count;
        check(midi.status().percussionHits==expected,"percussion follows exact MIDI ticks across tempo change");render(midi,32);time+=tick<96?1./48:1./36;}
    midi.midi(0xfc,time);render(midi,1000);check(midi.status().percussionHits==expected,"MIDI Stop releases percussion");
    midi.midi(0xfb,time);midi.midi(0xf8,time+.02);expected+=percussionStep(c.session.percussion,64).count;
    check(midi.status().percussionHits==expected,"Continue resumes correct subdivision");
    midi.midi(0xfa,time+1);midi.midi(0xf8,time+1.01);expected+=percussionStep(c.session.percussion,0).count;
    check(midi.status().percussionHits==expected,"fresh Start returns to downbeat");
    render(midi,25000);check(!midi.status().playing,"clock loss stops percussion transport");auto stopped=midi.status().percussionHits;render(midi,5000);check(midi.status().percussionHits==stopped,"no free-running percussion after lost clock");
    // Isolated instrument stability and useful output for each sound at extreme color/levels.
    for(int rate:{44100,48000,96000})for(int sound=0;sound<4;++sound)for(float color:{0.f,1.f}){
        PercussionEngine drum;drum.prepare(rate);p={};p.enabled=true;p.level=1;drum.trigger({sound,1,.5f,123,true},color);
        double energy=0;for(int i=0;i<rate;++i){float l=0,r=0;drum.process(l,r,p);check(std::isfinite(l)&&std::abs(l)<1,"finite isolated percussion");energy+=l*l+r*r;}
        check(energy>.01,"each sound audible at both color extremes");
        drum.clear();float l=0,r=0;drum.process(l,r,p);check(l==0&&r==0,"clear leaves silence");
    }
    std::cout<<"PASS stable anchors, deterministic glitches, variable-buffer/sample timing, MIDI transport, independent pattern/mute, and sound stability\n";
}
