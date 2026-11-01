#include "Composition.h"
#include <algorithm>
#include <cmath>
namespace undertone {
namespace {constexpr double pi=3.14159265358979323846;}
const char* sectionName(int section) noexcept {static constexpr const char* names[]{"Opening","Statement","Pressure","Withdrawal"};return names[std::clamp(section,0,3)];}
CompositionFrame compositionFrame(const Session& s,double beat) noexcept {
    if(s.music.composition==Composition::song)return {1,1,1,1,0};
    const double bars=std::fmod(std::max(0.,beat)/4,32.);const int section=int(bars/8);
    if(!s.detail.breathe)return {1,1,1,1,section};
    const float t=float((1-std::cos(pi*std::fmod(bars,8.)/8))*.5);
    // Shared 32-bar arc with continuous boundaries; the last section leaves room for a return.
    static constexpr float bed[]{.18f,.48f,.7f,.38f,.18f};
    static constexpr float texture[]{.15f,.3f,.85f,.65f,.15f};
    auto mix=[&](const float* values){return values[section]+t*(values[section+1]-values[section]);};
    const bool uneasy=s.music.composition==Composition::uneasy;
    return {mix(bed),section==0?0.f:section==1?.45f:section==2?1.f:.1f,
        section==0?.76f:section==1?1.f:section==2?.85f:.4f,mix(texture)*(uneasy?1.f:.5f),section};
}
void SurfaceEngine::prepare(double sampleRate) noexcept {rate=sampleRate;clear();}
void SurfaceEngine::clear() noexcept {phase=time=0;low=slow=level=0;noise=0x53555246u;}
void SurfaceEngine::process(float& left,float& right,const Chord& chord,float amount,bool playing) noexcept {
    level+=float(1-std::exp(-1/(rate*(playing?.4:.12))))*((playing?amount:0)-level);
    if(level<.000001f)return;
    time+=1/rate;const float n=float(double(randomNext(noise))/2147483648.-1);
    low+=float(1-std::exp(-2*pi*900/rate))*(n-low);slow+=float(1-std::exp(-2*pi*120/rate))*(n-slow);
    const double frequency=chord.count?440*std::pow(2.,(chord.notes[0]-69)/12.):220;
    phase+=frequency/rate;if(phase>=1)phase-=std::floor(phase);
    const float breath=float(.45+.3*std::sin(time*.73)+.2*std::sin(time*1.31));
    const float grit=(low-slow)*breath*.035f;
    const float resonance=float(std::sin(phase*2*pi))*(.003f+.002f*float(std::sin(time*.41)));
    left+=level*(grit+resonance);right+=level*(grit*.73f+resonance*.91f);
}
}
