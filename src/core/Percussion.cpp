#include "Percussion.h"
#include <algorithm>
#include <cmath>
namespace undertone {
namespace {
constexpr double pi=3.14159265358979323846;
uint32_t hash(uint32_t v) noexcept {v^=v>>16;v*=0x7feb352du;v^=v>>15;v*=0x846ca68bu;return v^(v>>16);}
float chance(uint32_t v) noexcept {return float(double(hash(v))/4294967296.0);}
}
uint32_t nextPercussionSeed(uint32_t seed) noexcept {const auto old=seed%4;randomNext(seed);return(seed&~3u)|((old+1+(seed>>2)%3)%4);}
PercussionStep percussionStep(const Percussion& p,int64_t step) noexcept {
    PercussionStep out;if(!p.enabled||p.level<=0||step<0)return out;
    const int pos=int(step%32),bar=int((step/32)%4);
    const uint32_t identity=p.seed+uint32_t(step%128)*7919u;
    auto add=[&](int sound,float velocity,float pan,bool anchor){out.hits[out.count++]={sound,velocity,pan,hash(identity+uint32_t(sound)*131u),anchor};};
    // Four fixed anchors per bar survive all density, seed and glitch changes.
    if(pos==0||pos==16)add(0,pos==0?.84f:.65f,.5f,true);
    if(pos==8||pos==24)add(1,pos==8?.62f:.75f,.5f,true);
    if(pos%8==4&&chance(identity)<p.density)add(2,.25f+.14f*chance(identity+1),pos%16==4?.35f:.65f,false);
    // Each seed chooses a different offbeat skeleton; bar 4 supplies a restrained fill.
    static constexpr int ghost[4][3]={{6,18,28},{10,22,30},{2,14,26},{4,20,30}};
    for(int i=0;i<3;++i)if(pos==ghost[p.seed%4][i]&&chance(identity+7)<p.density*(bar==3?.95f:.6f))
        add(i==0?0:1,.21f+.13f*chance(identity+8),i==0?.5f:.42f,false);
    const float burst=p.glitch*(pos>=24?.6f:.16f)*(bar==3?1.4f:1.f);
    if(pos%2==1&&chance(identity+19)<burst)add(3,.17f+.13f*chance(identity+20),.25f+.5f*chance(identity+21),false);
    return out;
}
void PercussionEngine::prepare(double sampleRate) noexcept {
    rate=sampleRate;highpass=float(std::exp(-2*pi*75/rate));smoothing=float(1-std::exp(-1/(rate*.015)));clear();
}
void PercussionEngine::clear() noexcept {voices={};level=hpL=hpR=prevL=prevR=0;hits=0;}
void PercussionEngine::release() noexcept {for(auto& voice:voices)voice.decay=std::min(voice.decay,float(std::exp(-1/(rate*.012))));}
void PercussionEngine::trigger(const PercussionHit& hit,float color) noexcept {
    auto* choice=&voices[0];for(auto& voice:voices)if(voice.env<choice->env)choice=&voice;
    *choice={};choice->sound=hit.sound;choice->env=hit.velocity;choice->pan=hit.pan;choice->noise=hit.noiseSeed?hit.noiseSeed:1;
    constexpr double durations[]{.065,.032,.016,.011};
    choice->decay=float(std::exp(-1/(rate*durations[hit.sound])));
    const float pitches[]{88+color*32,340+color*640,3500+color*4200,1150+color*4200};
    choice->frequency=pitches[hit.sound];++hits;
}
void PercussionEngine::process(float& left,float& right,const Percussion& p) noexcept {
    level+=smoothing*((p.enabled?p.level:0)-level);
    float l=0,r=0;
    for(auto& voice:voices)if(voice.env>.00001f){
        const float noise=float(double(randomNext(voice.noise))/2147483648.0-1);
        voice.low+=.12f*(noise-voice.low);const float high=noise-voice.low;
        const double t=voice.age;
        const double frequency=voice.frequency*(voice.sound==0?1+.9*std::exp(-t*90):1);
        voice.phase+=frequency/rate;voice.phase-=std::floor(voice.phase);
        const float sine=float(std::sin(2*pi*voice.phase));
        float sound=0;
        switch(voice.sound){
            case 0:sound=.7f*sine+.18f*float(std::sin(6*pi*voice.phase)*std::exp(-t*30))+.09f*high*float(std::exp(-t*300));break;
            case 1:sound=.28f*sine+.42f*high;break;
            case 2:sound=.3f*high*(.65f+.35f*sine);break;
            case 3:sound=.35f*high+.18f*(sine>0?1.f:-1.f);break;
        }
        const float attack=float(std::min(1.0,t/.001));
        const float sample=sound*voice.env*attack*.4f;
        l+=sample*(1-voice.pan*.5f);r+=sample*(.5f+voice.pan*.5f);
        voice.env*=voice.decay;voice.age+=1/rate;
    }
    // Keep transients dry and remove deep sub energy, independent of the pad's reverb/Bass cut.
    hpL=highpass*(hpL+l-prevL);hpR=highpass*(hpR+r-prevR);prevL=l;prevR=r;
    left+=hpL*level;right+=hpR*level;
}
}
