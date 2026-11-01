#include "MidiTimeline.h"
#include <algorithm>
#include <cmath>

namespace undertone {
void MidiTimeline::drain() noexcept {TimedMidi event;for(int i=0;i<1024&&input.pop(event);++i){} }
void MidiTimeline::prepare(double sampleRate,int nominalBlockSize) noexcept {
    rate=sampleRate;delay=2.0*nominalBlockSize/rate;
    anchored=false;previousSamples=0;lastInput=-1;late=discarded=resets=0;drain();overflow=false;
}
void MidiTimeline::changeSource(uint32_t newEpoch) noexcept {
    epoch=newEpoch;lastInput=-1;drain();overflow=false;
}
bool MidiTimeline::push(const TimedMidi& event) noexcept {
    if(input.push(event))return true;
    overflow.store(true,std::memory_order_release);return false;
}
void MidiTimeline::stopAndRequireStart(Engine& engine) noexcept {
    // Match a source reset: never let old Continue messages revive a broken stream.
    const auto before=engine.status();
    if(before.external){Command c;c.action=Action::external;c.value=1;c.sequence=before.sequence;engine.command(c);}
    drain();lastInput=-1;++resets;
}
void MidiTimeline::process(Engine& engine,float* left,float* right,int samples,double now) noexcept {
    if(samples<=0)return;
    if(!std::isfinite(now)){stopAndRequireStart(engine);engine.process(left,right,samples);anchored=false;return;}
    const double observed=now-delay;
    if(!anchored){blockTime=observed;anchored=true;}
    else {
        const double predicted=blockTime+double(previousSamples)/rate;
        const double error=observed-predicted;
        if(now<lastCallback||std::abs(error)>.25){
            stopAndRequireStart(engine);blockTime=observed;
        } else {
            // Track slow audio/wall-clock drift without converting every callback's
            // scheduling jitter into a new MIDI offset. Correction is at most 1 sample.
            blockTime=predicted+std::clamp(error*.005,-1.0/rate,1.0/rate);
        }
    }
    previousSamples=samples;lastCallback=now;
    if(overflow.exchange(false,std::memory_order_acq_rel))stopAndRequireStart(engine);
    const double end=blockTime+double(samples)/rate;
    int rendered=0;float peak=0;
    const auto renderUntil=[&](int offset){
        if(offset>rendered){engine.process(left?left+rendered:nullptr,right?right+rendered:nullptr,offset-rendered);peak=std::max(peak,engine.status().peak);rendered=offset;}
    };
    TimedMidi event;
    for(int handled=0;handled<1024&&input.peek(event);++handled){
        if(event.epoch!=epoch||!std::isfinite(event.seconds)||event.seconds>now+1.0||event.seconds<blockTime-.5){
            input.pop(event);++discarded;continue;
        }
        if(event.seconds>=end)break; // Retain a future event for its own audio block.
        input.pop(event);
        if(event.seconds<lastInput){++discarded;continue;}
        lastInput=event.seconds;
        // The drift correction can move a boundary by at most one sample.
        // An event retained at that boundary is not a late delivery unless it
        // falls more than one sample behind the new interval.
        if(event.seconds<blockTime-1.0/rate)++late;
        const int offset=std::clamp(int(std::llround((event.seconds-blockTime)*rate)),rendered,samples);
        renderUntil(offset);
        engine.midi(event.byte,event.seconds);
    }
    renderUntil(samples);
    // Engine peak reflects the last segment; the caller's meter should see the
    // whole output block. The status snapshot calculates this outside the engine.
    blockPeak=peak;
}
Status MidiTimeline::status(const Engine& engine) const noexcept {
    auto s=engine.status();s.lateMidi=late;s.discardedMidi=discarded;s.midiResets=resets;s.midiDelayMs=delay*1000;s.peak=blockPeak;return s;
}
}
