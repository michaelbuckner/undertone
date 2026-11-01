#pragma once
#include "Music.h"
#include <vector>

namespace undertone {
struct DetailEvent { int note=0, role=0; float velocity=0, pan=.5f; MelodyVoice voice=MelodyVoice::original; float gateBeats=8; }; // role 0 broken chord, 1 motif
struct DetailStep { std::array<DetailEvent,2> events{}; int count=0; };
struct MelodyContext {
    struct Change {const Session* session=nullptr;int64_t step=0,origin=0;};
    std::array<Change,8> changes{};
    int changeCount=0;
    const Session* upcoming=nullptr;
    int64_t changeStep=-1,phraseOrigin=0,upcomingOrigin=0;
    int previousNote=-1;
    bool previousPassing=false;
};
bool chordContains(const Chord&,int note) noexcept;
DetailStep detailStep(const Session&,int64_t eighthStep,const MelodyContext& = {}) noexcept;
uint32_t nextMotifSeed(uint32_t) noexcept;
class DetailEngine {
public:
    void prepare(double sampleRate);
    void clear() noexcept;
    void trigger(const DetailEvent&,double bpm=88) noexcept;
    void harmonize(const Chord&) noexcept;
    void release() noexcept;
    void process(float& left,float& right,const Detail&,double bpm,bool playing) noexcept;
    uint64_t notes=0;
private:
    struct Voice { double phase=0,frequency=0,age=0,gate=8,echoTime=.5; float energy=0,pan=.5f,releaseGain=1; int role=0,note=0; MelodyVoice timbre=MelodyVoice::original; bool released=false; };
    std::array<Voice,12> voices{};
    std::vector<float> echoL,echoR;
    size_t position=0;
    double rate=48000,seconds=0,delaySamples=16000;
    float motif=0,broken=0,echo=0,wear=0,dampedL=0,dampedR=0;
};
}
