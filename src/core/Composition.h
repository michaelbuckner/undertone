#pragma once
#include "Music.h"
namespace undertone {
struct CompositionFrame {float bed=1,figure=1,melody=1,surface=0;int section=0;};
CompositionFrame compositionFrame(const Session&,double beat) noexcept;
const char* sectionName(int) noexcept;
class SurfaceEngine {
public:
    void prepare(double sampleRate) noexcept;
    void clear() noexcept;
    void process(float&,float&,const Chord&,float amount,bool playing) noexcept;
private:
    double rate=48000,phase=0,time=0;
    float low=0,slow=0,level=0;
    uint32_t noise=0x53555246u;
};
}
