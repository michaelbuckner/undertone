#pragma once
#include "Music.h"
namespace undertone {
struct PercussionHit { int sound=0;float velocity=0,pan=.5f;uint32_t noiseSeed=1;bool anchor=false; };
struct PercussionStep { std::array<PercussionHit,3> hits{};int count=0; };
PercussionStep percussionStep(const Percussion&,int64_t thirtySecond) noexcept;
uint32_t nextPercussionSeed(uint32_t) noexcept;
class PercussionEngine {
public:
    void prepare(double sampleRate) noexcept;
    void clear() noexcept;
    void release() noexcept;
    void trigger(const PercussionHit&,float color) noexcept;
    void process(float& left,float& right,const Percussion&) noexcept;
    uint64_t hits=0;
private:
    struct Voice {int sound=0;double phase=0,age=0;float env=0,decay=0,pan=.5f,low=0,frequency=0;uint32_t noise=1;};
    std::array<Voice,12> voices{};
    double rate=48000;
    float level=0,hpL=0,hpR=0,prevL=0,prevR=0,highpass=.99f,smoothing=.001f;
};
}
