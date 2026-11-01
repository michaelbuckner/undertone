#include "core/Engine.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace undertone;
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
void command(Engine& e,Action action,int value=0){Command c;c.action=action;c.value=value;e.command(c);}
void load(Engine& e,const Session& s){Command c;c.action=Action::load;c.session=s;e.command(c);}
int melody(const Session& s,int step){const auto events=detailStep(s,step);for(int i=0;i<events.count;++i)if(events.events[i].role==1)return events.events[i].note;return -1;}
int main(){
    int variations=0,spaceReduction=0;
    for(int seed=1;seed<=24;++seed){auto s=makePreset(8);s.detail.seed=seed;s.detail.breathe=false;auto changed=s;
        changed.detail.motifMode=MotifMode::vary;advanceMotif(changed.detail);
        check(changed.detail.seed==s.detail.seed&&changed.detail.figureSeed==s.detail.figureSeed,"Vary preserves motif and figure identity");
        for(int step:{0,3,8})check(melody(s,step)==melody(changed,step),"Vary preserves opening pitches under the same harmony");
        for(int step:{14,18,22,46,50,54})variations+=melody(s,step)!=melody(changed,step);
        for(int step=56;step<64;++step)check(melody(s,step)<0,"answer ends with a full-bar breath");
        auto held=s;held.detail.motifMode=MotifMode::hold;advanceMotif(held.detail);check(held.detail.seed==s.detail.seed&&held.detail.variationSeed==0,"Hold leaves motif unchanged");
        advanceMotif(held.detail,true);check(held.detail.seed!=s.detail.seed&&held.detail.figureSeed==s.detail.figureSeed,"New changes only melody identity");
        auto sparse=s;sparse.detail.room=1;
        for(int step=0;step<128;++step){const auto a=detailStep(s,step),b=detailStep(sparse,step);check(b.count<=a.count,"space only removes events");spaceReduction+=a.count-b.count;
            for(int n=0;n<a.count;++n)if(a.events[n].role==0){const auto other=detailStep(held,step);bool same=false;for(int j=0;j<other.count;++j)if(other.events[j].role==0)same=other.events[j].note==a.events[n].note&&other.events[j].velocity==a.events[n].velocity;check(same,"New motif leaves figure events exact");}}
    }
    check(variations>10&&spaceReduction>100,"variation and space have meaningful musical effect");
    auto s=makePreset(10);s.music.evolve=false;storeScene(s,0);auto b=makePreset(11);b.music.evolve=false;b.detail.bed=.8f;b.detail.motifMode=MotifMode::vary;b.phrase.chords[1].rest=true;
    static_cast<MusicalState&>(s)=b;storeScene(s,1);s.bpm=120;recallScene(s,0);s.sceneCycle=true;s.sceneBars=1;s.cycles={true,true,1,1};
    Engine scenes;scenes.prepare(48000);load(scenes,s);command(scenes,Action::external,1);scenes.midi(0xfa,0);
    for(int tick=0;tick<=192;++tick){scenes.midi(0xf8,tick/48.);
        if(tick==95)check(scenes.session().activeScene==0,"scene recall not early");
        if(tick==96){const auto& now=scenes.session();check(now.activeScene==1&&now.music.key==b.music.key&&now.detail.bed==b.detail.bed&&now.phrase.chords[1].rest,"B restores harmony, mix and rests");check(scenes.status().compositionChanges==0&&scenes.status().motifChanges==0,"scene wins simultaneous cycles");}
    }
    check(scenes.session().activeScene==0&&scenes.session().detail.seed==s.scenes[0].state.detail.seed,"exact A return");
    check(scenes.session().bpm==120&&valid(scenes.session()),"scene tempo remains global and state valid");
    auto varied=s;const auto original=varied.scenes[1].state.detail.variationSeed;recallScene(varied,1,true);check(varied.detail.variationSeed!=original&&varied.scenes[1].state.detail.variationSeed==original,"scene variation retains clean saved snapshot");
    recallScene(varied,1);check(varied.detail.variationSeed==original,"clean recall after varied return");
    varied.scenes[1].state.detail.motifMode=MotifMode::hold;recallScene(varied,1,true);
    check(varied.detail.variationSeed==original,"scene returns respect Hold mode");
    command(scenes,Action::stop);command(scenes,Action::recallScene,1);check(scenes.session().activeScene==1,"stopped scene immediate");
    command(scenes,Action::newMotif);const auto live=scenes.session().detail.seed;command(scenes,Action::storeScene,1);command(scenes,Action::recallScene,0);command(scenes,Action::recallScene,1);check(scenes.session().detail.seed==live,"store captures current realized motif");
    for(int preset=0;preset<presetCount;++preset)for(int style=0;style<3;++style){auto state=makePreset(preset);state.music.evolve=false;auto seed=state.seed;
        auto selected=chooseChord(state,seed,1,SwapStyle(style));check(!sameChord(selected.chords[1],state.phrase.chords[1]),"each swap approach supplies a different chord");
        for(int i=0;i<state.music.slots;++i)if(i!=1)check(sameChord(selected.chords[i],state.phrase.chords[i]),"intentional swap isolated");
        state.phrase=selected;check(valid(state),"intentional swap valid");}
    Engine compare;auto comparison=makePreset(8);comparison.music.evolve=false;load(compare,comparison);command(compare,Action::swapChord,0);auto swapped=compare.session().phrase.chords[0];
    command(compare,Action::compareChord,0);check(sameChord(compare.session().phrase.chords[0],comparison.phrase.chords[0]),"comparison recalls previous chord");
    command(compare,Action::compareChord,0);check(sameChord(compare.session().phrase.chords[0],swapped),"comparison returns to candidate");
    // Multi-boundary context changes a later scheduled harmony, not just the first transition.
    int futureEffects=0;
    for(int seed=1;seed<=64;++seed){auto a=makePreset(8);a.detail.seed=seed;a.detail.breathe=false;a.music.bars=1;auto first=a,second=a;
        auto chordSeed=uint32_t(seed);for(int i=0;i<second.music.slots;++i)second.phrase=chooseChord(second,chordSeed,i,SwapStyle::surprise);
        MelodyContext one,two;one.changes[0]={&first,8,0};one.changeCount=1;two=one;two.changes[1]={&second,16,0};two.changeCount=2;
        for(int step:{0,3,8,14}){const auto x=detailStep(a,step,one),y=detailStep(a,step,two);for(int i=0;i<std::min(x.count,y.count);++i)if(x.events[i].role==1&&y.events[i].role==1)futureEffects+=x.events[i].note!=y.events[i].note||x.events[i].gateBeats!=y.events[i].gateBeats;}}
    check(futureEffects>0,"second forecast boundary causally affects phrasing before it arrives");
    PercussionEngine low,high;low.prepare(48000);high.prepare(48000);Percussion p;p.enabled=true;p.level=1;auto louder=p;louder.level=2;double a=0,bEnergy=0;
    for(int frame=0;frame<48000;++frame){if(frame%12000==0){PercussionHit hit;hit.velocity=1;low.trigger(hit,.4f);high.trigger(hit,.4f);}float l=0,r=0,x=0,y=0;low.process(l,r,p);high.process(x,y,louder);a+=l*l;bEnergy+=x*x;}
    check(a>0&&std::abs(bEnergy/a-4)<.001,"upper percussion range doubles signal amplitude before master");
    for(int rate:{44100,48000,96000}){Engine rendered;auto state=s;state.bpm=200;state.sceneCycle=true;state.sceneBars=1;
        for(auto& scene:state.scenes){scene.state.percussion={true,2,1,1,1,42};scene.state.detail.bed=scene.state.detail.broken=scene.state.detail.motif=1;scene.state.patch.level=1;}
        recallScene(state,0);rendered.prepare(rate);load(rendered,state);command(rendered,Action::play);
        float l[256],r[256];for(int at=0;at<rate*5;at+=256){rendered.process(l,r,256);for(int i=0;i<256;++i)check(std::isfinite(l[i])&&std::isfinite(r[i])&&std::abs(l[i])<=1&&std::abs(r[i])<=1,"scene changes and boosted percussion remain finite");}
        check(rendered.status().sceneChanges>=4,"rendered scenes actually alternate");command(rendered,Action::panic);rendered.process(l,r,256);for(float v:l)check(v==0,"panic clears boosted scene output");}
    std::cout<<"PASS phrasing, motif independence, variation="<<variations<<", space reduction="<<spaceReduction<<", scene recall/priority, swaps, forecast effects="<<futureEffects<<", percussion +6 dB\n";
}
