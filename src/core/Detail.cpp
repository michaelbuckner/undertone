#include "Detail.h"
#include "Composition.h"
#include <algorithm>
#include <cmath>
namespace undertone {
namespace {
constexpr double pi=3.14159265358979323846;
int fit(int note,int floor) noexcept {while(note<floor)note+=12;while(note>108)note-=12;return note;}
uint32_t hash(uint32_t value) noexcept {value^=value>>16;value*=0x7feb352du;value^=value>>15;value*=0x846ca68bu;return value^(value>>16);}
constexpr int positions[]{0,3,6,10,15,19,22,27};
constexpr int contours[4][8]={{4,2,1,0,2,4,5,2},{2,4,3,1,0,2,4,1},{0,2,4,3,2,1,4,0},{4,5,2,1,4,2,0,1}};
const Session& atSession(const Session& s,const MelodyContext& c,int64_t step) noexcept {
    for(int i=c.changeCount-1;i>=0;--i)if(step>=c.changes[i].step)return *c.changes[i].session;
    return c.upcoming&&c.changeStep>=0&&step>=c.changeStep?*c.upcoming:s;
}
const Chord& atChord(const Session& s,const MelodyContext& c,int64_t step) noexcept {
    const auto& state=atSession(s,c,step);
    auto origin=&state==c.upcoming?c.upcomingOrigin:c.phraseOrigin;
    for(int i=0;i<c.changeCount;++i)if(&state==c.changes[i].session)origin=c.changes[i].origin;
    const int slot=int(std::max(int64_t(0),step-origin)/(state.music.bars*8)%state.music.slots);
    return state.phrase.chords[slot];
}
int motifIndex(const Session& s,int64_t step) noexcept {
    if(s.detail.room>0){
        const int phase=int(step%64);
        if(phase>=int(60-20*s.detail.room))return -1;
        if(phase%32!=0&&float(hash(s.detail.seed+uint32_t(step%64))%1000)/1000.f<s.detail.room*.7f)return -1;
    }
    if(s.detail.phrasing&&s.music.composition==Composition::song){
        if(s.detail.breathe&&(step/8)%16>=12)return -1;
        if(s.detail.style!=PhraseStyle::classic){
            static constexpr int lyrical[]{0,5,12,18,24},pocket[]{3,6,12,19,22};
            const auto* rhythm=s.detail.style==PhraseStyle::pocket?pocket:lyrical;
            for(int i=0;i<5;++i)if(step%32==rhythm[i])return i;
            return -1;
        }
        static constexpr int phrase[]{0,3,8,14,18,22};
        for(int i=0;i<6;++i)if(step%32==phrase[i])return i;
        return -1;
    }
    if(s.music.composition!=Composition::song){
        if(s.detail.breathe&&(step/8)%32>=24)return -1;
        static constexpr int still[]{0,11,14,32,44},uneasy[]{0,3,20,32,35,54};
        const int* rhythm=s.music.composition==Composition::still?still:uneasy;
        const int count=s.music.composition==Composition::still?5:6;
        const int phase=int((step+(s.detail.seed%2?0:2))%64);
        for(int i=0;i<count;++i)if(phase==rhythm[i])return i;
        return -1;
    }
    if(s.detail.breathe&&(step/8)%16>=12)return -1;
    for(int i=0;i<8;++i)if(step%32==positions[i])return i;
    return -1;
}
int desiredNote(const Session& s,int64_t step,int index) noexcept {
    if(s.detail.phrasing){
        static constexpr int statements[4][6]={{0,2,4,2,1,2},{4,2,0,1,2,4},{2,4,5,4,2,1},{0,1,2,4,2,0}};
        int degree=statements[s.detail.seed%4][index%6];
        const bool answer=(step%64)>=32;
        if(answer&&index>=3)degree=index>=4?0:(degree+5)%7;
        // Variations leave the opening gesture intact and develop the answer/ending.
        if(s.detail.variationSeed&&index>=3&&!(answer&&index>=4))degree=(degree+1+hash(s.detail.variationSeed+uint32_t(index))%2)%7;
        return fit(s.music.key+scale(s.music.mode)[degree]+(s.music.composition==Composition::song&&s.detail.style==PhraseStyle::classic?72:60),s.music.floor+(s.music.composition==Composition::song&&s.detail.style==PhraseStyle::classic?7:3));
    }
    if(s.music.composition!=Composition::song){
        static constexpr int cells[4][6]={{0,4,1,0,4,1},{4,0,1,4,0,5},{0,1,4,0,1,5},{4,1,0,4,1,6}};
        int degree=cells[s.detail.seed%4][index];
        if(s.detail.variationSeed&&index>=3)degree=(degree+1+hash(s.detail.variationSeed+uint32_t(index))%2)%7;
        if((step/64)%4==2&&index>=4)degree=(degree+1)%7;
        return fit(s.music.key+scale(s.music.mode)[degree]+60,s.music.floor+3);
    }
    int degree=contours[s.detail.seed%4][index];
    if(s.detail.variationSeed&&index>=3)degree=(degree+1+hash(s.detail.variationSeed+uint32_t(index))%2)%7;
    if((step/32)%2&&index>=6)degree=(degree+2)%7;
    return fit(s.music.key+scale(s.music.mode)[degree]+72,s.music.floor+7);
}
struct Choices { std::array<int,16> pitch{};std::array<bool,16> chord{};std::array<float,16> cost{};int count=0,target=0; };
Choices choices(const Session& s,const MelodyContext& context,int64_t step,bool preserveOpening=false) noexcept {
    const auto& state=atSession(s,context,step);const auto& chord=atChord(s,context,step);Choices out;
    const int index=motifIndex(state,step);
    if(preserveOpening){auto stable=state;stable.detail.variationSeed=0;out.target=desiredNote(stable,step,index);}
    else out.target=desiredNote(state,step,index);
    // Accent and phrase-end notes land on harmony. Only weak offbeats may pass between tones.
    const bool anchor=step%2==0||index==7||index==0||(state.detail.phrasing&&index>=4);
    const int low=state.music.composition==Composition::song&&state.detail.style==PhraseStyle::classic?std::max(state.music.floor+7,67):std::max(state.music.floor+3,60),high=std::min(108,low+(state.music.composition!=Composition::song||state.detail.style==PhraseStyle::classic?23:19));
    for(int note=low;note<=high;++note){
        bool inScale=false;for(int pc:scale(state.music.mode))inScale|=(note-state.music.key+120)%12==pc;
        const bool tone=chordContains(chord,note);
        if(!inScale||(anchor&&!tone)||(state.music.rootless&&note%12==chord.root))continue;
        const int n=out.count++;out.pitch[n]=note;out.chord[n]=tone;
        out.cost[n]=.55f*std::abs(note-out.target)+(tone?0.f:.15f);
        if(state.detail.phrasing&&step%64>=32&&index>=4&&note%12!=chord.root)out.cost[n]+=2.f;
        // Approach a change using a common tone where available; otherwise let articulation create space.
        if(!chordContains(atChord(s,context,step+1),note))out.cost[n]+=.8f;
    }
    if(out.count==0){ // Defensive fallback for imported root-only voicings with root omission enabled.
        for(int note=low;note<=high;++note)for(int pc:scale(state.music.mode))
            if((note-state.music.key+120)%12==pc&&(!state.music.rootless||note%12!=chord.root)){
                out.pitch[0]=note;out.chord[0]=chordContains(chord,note);out.cost[0]=0;out.count=1;return out;}
    }
    return out;
}
float connection(int a,int b,bool aChord,bool bChord,int contour) noexcept {
    const int distance=std::abs(b-a);
    if(!aChord&&(!bChord||distance>2))return 100000.f; // Passing tension must resolve by step.
    return .32f*distance+.7f*std::max(0,distance-4)*std::max(0,distance-4)
        +.12f*std::abs((b-a)-std::clamp(contour,-4,4));
}
int plannedNote(const Session& s,const MelodyContext& context,int64_t step) noexcept {
    // Fixed-size backward dynamic program: 32 future melody events, at least sixteen bars.
    // Includes several upcoming chords, known queued edits and the progression's loop return.
    std::array<Choices,33> path{};int64_t future=step;int rows=0;
    const bool preserveOpening=motifIndex(s,step)<3;
    for(auto& choice:path){int scanned=0;
        while(scanned<1024&&(motifIndex(atSession(s,context,future),future)<0||atChord(s,context,future).rest)){++future;++scanned;}
        if(scanned==1024)break; // A silent future must never stall the audio callback.
        choice=choices(s,context,future++,preserveOpening);++rows;
    }
    if(!rows)return -1;
    std::array<float,16> next=path[rows-1].cost,current{};
    for(int row=rows-2;row>=0;--row){const auto& a=path[row];const auto& b=path[row+1];
        for(int i=0;i<a.count;++i){float best=1.e9f;
            for(int j=0;j<b.count;++j)best=std::min(best,next[j]+connection(a.pitch[i],b.pitch[j],a.chord[i],b.chord[j],b.target-a.target));
            current[i]=a.cost[i]+best;
        }next=current;
    }
    const auto& first=path[0];float best=1.e9f;int note=first.pitch[0];
    for(int i=0;i<first.count;++i){float value=next[i];
        if(context.previousNote>=0)value+=connection(context.previousNote,first.pitch[i],!context.previousPassing,first.chord[i],0);
        if(value<best){best=value;note=first.pitch[i];}}
    return note;
}
}
bool chordContains(const Chord& chord,int note) noexcept {if(chord.rest)return false;for(int n=0;n<chord.count;++n)if(chord.notes[n]%12==note%12)return true;return false;}
uint32_t nextMotifSeed(uint32_t seed) noexcept {
    const auto previous=seed%4;randomNext(seed);
    return (seed&~3u)|((previous+1+(seed>>2)%3)%4);
}
DetailStep detailStep(const Session& s,int64_t step,const MelodyContext& context) noexcept {
    DetailStep result;
    if(step<0)return result;
    const int bar=int((step/8)%16),pos=int(step%8);
    const auto& d=s.detail;
    const auto& chord=atChord(s,context,step);
    if(chord.rest)return result;
    const bool study=s.music.composition!=Composition::song;
    const auto frame=compositionFrame(s,step*.5);
    const auto identity=hash(d.figureSeed);
    // A repeating syncopation with a deliberate gap; note order answers on alternate bars.
    static constexpr int hits[2][4]={{0,3,5,6},{1,3,4,7}};
    const int pattern=int(identity%2);
    if(study&&d.broken>0&&frame.figure>0&&!(d.room>.5f&&(step/8)%2)){
        const int place=int(step%32);const bool hit=place==8||(s.music.composition==Composition::uneasy&&place==23&&frame.section==2);
        if(hit){const int n=int((step/32+d.figureSeed)%chord.count);
            result.events[result.count++]={fit(chord.notes[n],s.music.floor),0,.43f*frame.figure,.38f,MelodyVoice::feltObject,1.4f};}
    }
    if(!study&&d.style==PhraseStyle::classic&&d.broken>0 && (!d.breathe||bar<8||bar>=12))for(int i=0;i<4;++i)if(pos==hits[pattern][i]&&(d.room<.5f||i==0)){
        const int index=((bar%2)?chord.count-1-(i%chord.count):i%chord.count);
        const int note=fit(chord.notes[index],s.music.floor);
        result.events[result.count++]={note,0,(i==0?.82f:.58f)+float(hash(d.figureSeed+uint32_t(step))%13)*.01f,.28f+.12f*(i%3)};
    }
    const int index=motifIndex(s,step);
    if(!study&&d.style!=PhraseStyle::classic&&d.broken>0&&frame.figure>0&&(d.motif<=0||index<0)){
        const bool pocket=d.style==PhraseStyle::pocket;
        const int place=int(step%16);
        // A supporting reply, with no simultaneous lead attack. Space only removes notes.
        const bool hit=pocket?(place==1||place==10):(place==7||place==15);
        if(hit&&!(d.room>.5f&&place>=8)){
            const int n=int((step/16+identity)%chord.count);
            result.events[result.count++]={fit(chord.notes[n],s.music.floor),0,.58f*frame.figure,.4f,
                MelodyVoice::feltObject,pocket?.65f:1.4f};
        }
    }
    if(d.motif>0&&index>=0){
        const int note=plannedNote(s,context,step);
        if(note<0)return result;
        float gate=4;
        for(int offset=1;offset<=32;++offset)if(!chordContains(atChord(s,context,step+offset),note)){
            gate=std::max(.125f,offset*.5f-.125f);break;
        }
        if(!study&&d.style==PhraseStyle::pocket)gate=std::min(gate,.7f);
        const float velocity=study?(index==0?.9f:index==1?.58f:index==2?.42f:.68f)*frame.melody:(index==0?.84f:index==4?.76f:.66f)*(d.style==PhraseStyle::classic?1.f:frame.melody);
        result.events[result.count++]={note,1,velocity,study?.47f+.03f*float(hash(d.seed+uint32_t(step/64))%5):.65f,d.voice,gate};
    }
    return result;
}
void DetailEngine::prepare(double sampleRate){rate=sampleRate;echoL.assign(size_t(rate*3)+2,0);echoR=echoL;clear();}
void DetailEngine::clear() noexcept {
    voices={};std::fill(echoL.begin(),echoL.end(),0.f);std::fill(echoR.begin(),echoR.end(),0.f);
    position=0;seconds=0;notes=0;dampedL=dampedR=0;motif=broken=echo=wear=0;delaySamples=rate*.33;
}
void DetailEngine::trigger(const DetailEvent& event,double bpm) noexcept {
    auto* voice=&voices[0];
    for(auto& v:voices)if(v.energy<voice->energy)voice=&v;
    *voice={};voice->frequency=440*std::pow(2.0,(event.note-69)/12.0);voice->role=event.role;
    voice->energy=event.velocity;voice->pan=event.pan;++notes;
    voice->note=event.note;voice->timbre=event.voice;voice->gate=event.gateBeats*60/bpm;voice->echoTime=45/bpm;
}
void DetailEngine::harmonize(const Chord& chord) noexcept {
    if(chord.rest){release();return;}
    for(auto& voice:voices)if((voice.role==1||int(voice.timbre)>=4)&&!chordContains(chord,voice.note))voice.released=true;
}
void DetailEngine::release() noexcept {for(auto& v:voices)v.released=true;}
void DetailEngine::process(float& left,float& right,const Detail& detail,double bpm,bool playing) noexcept {
    const float smooth=float(1-std::exp(-1/(rate*.035)));
    motif+=smooth*(detail.motif-motif);broken+=smooth*(detail.broken-broken);
    echo+=smooth*(detail.echo-echo);wear+=smooth*(detail.wear-wear);
    seconds+=1/rate;
    float l=0,r=0;
    const double drift=1+wear*(.0023*std::sin(seconds*2*pi*.37)+.0007*std::sin(seconds*2*pi*5.1));
    for(auto& voice:voices)if(voice.energy>.00001f){
        const double t=voice.age;
        const float attack=float(std::min(1.0,t/(voice.role?.012:.006)));
        const float mod=float(std::exp(-t*(voice.role?2.5:9)));
        const double phase=voice.phase*2*pi;
        // Rounded, woody pluck under a softer, hollow upper motif: two timbres with distinct contours.
        const float signal=voice.role?
            float(.75*std::sin(phase+.65*mod*std::sin(phase*2))+.18*std::sin(phase*3)):
            float(.8*std::sin(phase+1.4*mod*std::sin(phase*2))+.12*std::sin(phase*2));
        float sample=signal*voice.energy*attack*(voice.role?motif*.22f:broken*.19f);
        const bool modal=int(voice.timbre)>=4;
        if(voice.role||modal){
            if(t>=voice.gate)voice.released=true;
            if(voice.released)voice.releaseGain*=float(std::exp(-1/(rate*.035)));
            auto tone=[&](double age,double phase){
                if(age<0)return 0.f;
                const double a=voice.timbre==MelodyVoice::bowedWire?.38:voice.timbre==MelodyVoice::feltObject?.008:voice.timbre==MelodyVoice::airFlute?.065:voice.timbre==MelodyVoice::feltKeys?.022:voice.timbre==MelodyVoice::softSine?.04:.012;
                const double envelope=std::min(1.0,age/a)*std::exp(-age/(voice.timbre==MelodyVoice::bowedWire?2.4:voice.timbre==MelodyVoice::feltObject?1.2:voice.timbre==MelodyVoice::feltKeys?.52:.75));
                double value=0;
                switch(voice.timbre){
                    case MelodyVoice::original:value=.75*std::sin(phase+.65*std::exp(-age*2.5)*std::sin(phase*2))+.18*std::sin(phase*3);break;
                    case MelodyVoice::softSine:value=.85*std::sin(phase)+.035*std::sin(phase*2);break;
                    case MelodyVoice::feltKeys:value=.78*std::sin(phase)+.16*std::exp(-age*5)*std::sin(phase*2)+.035*std::exp(-age*10)*std::sin(phase*3);break;
                    case MelodyVoice::airFlute:value=(.77*std::sin(phase)+.09*std::sin(phase*2)+.025*std::sin(phase*3))*(.98+.02*std::sin(age*2*pi*4.2));break;
                    case MelodyVoice::feltObject:{const double p=2*pi*voice.frequency*age;
                        value=.78*std::sin(p)+.18*voice.energy*std::exp(-age*2.7)*std::sin(p*2.002)
                            +.09*voice.energy*std::exp(-age*6)*std::sin(p*3.998)+.04*std::exp(-age*16)*std::sin(p*7.01);break;}
                    case MelodyVoice::bowedWire:{const double p=2*pi*voice.frequency*age;
                        value=.48*std::sin(p)+.26*std::sin(p*1.003)+.1*std::sin(p*2.01)*(.6+.4*std::sin(age*1.8))
                            +.045*std::sin(p*3.97)*std::exp(-age*.9);break;}
                }
                return float(value*envelope);
            };
            // Pitch-specific echo taps share this voice's harmonic release, so old notes cannot hide in a delay line.
            float sound=tone(t,phase);
            for(int tap=1;tap<=2;++tap){const double delay=voice.echoTime*tap;sound+=echo*(tap==1?.28f:.10f)*tone(t-delay,phase-delay*voice.frequency*2*pi);}
            sample=sound*voice.energy*voice.releaseGain*(voice.role?motif:broken)*.22f;
            // Melody is mixed outside the broken-chord feedback delay below.
            left+=sample*(1-voice.pan*.65f);right+=sample*(.35f+voice.pan*.65f);
        }else {l+=sample*(1-voice.pan*.65f);r+=sample*(.35f+voice.pan*.65f);}
        voice.phase+=voice.frequency*drift/rate;voice.phase-=std::floor(voice.phase);
        voice.age+=1/rate;
        if(!voice.role&&!modal)voice.energy*=float(std::exp(-1/(rate*(voice.released?.09:.22))));
        else if(voice.releaseGain<.00001f||t>6)voice.energy=0;
    }
    if(!echoL.empty()){
        const double target=rate*60/std::clamp(bpm,40.0,200.0)*.75;
        delaySamples+=float(1-std::exp(-1/(rate*.3)))*(target-delaySamples);
        double at=double(position)-delaySamples;if(at<0)at+=echoL.size();
        const auto index=size_t(at);const float fraction=float(at-index);
        const float wetL=echoL[index]*(1-fraction)+echoL[(index+1)%echoL.size()]*fraction;
        const float wetR=echoR[index]*(1-fraction)+echoR[(index+1)%echoR.size()]*fraction;
        dampedL+=.16f*(wetL-dampedL);dampedR+=.16f*(wetR-dampedR);
        echoL[position]=l+dampedR*.42f;echoR[position]=r+dampedL*.42f;
        position=(position+1)%echoL.size();
        l+=dampedL*echo*.8f;r+=dampedR*echo*.8f;
    }
    // No free-running noise or oscillators are added when playback is stopped.
    (void)playing;
    left+=l;right+=r;
}
}
