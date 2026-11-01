#pragma once
#include "Engine.h"

namespace undertone {
struct TimedMidi { uint8_t byte=0;double seconds=0;uint32_t epoch=0; };

// Input timestamps and callbackTime must share one monotonic seconds clock.
// The MIDI producer only calls push(). All other methods belong to the audio
// consumer (or the device lifecycle, serialized against that consumer).
class MidiTimeline {
public:
    void prepare(double sampleRate,int nominalBlockSize) noexcept;
    void changeSource(uint32_t newEpoch) noexcept;
    bool push(const TimedMidi& event) noexcept;
    void process(Engine&,float* left,float* right,int samples,double callbackTime) noexcept;
    Status status(const Engine&) const noexcept;
private:
    Queue<TimedMidi,1024> input;
    std::atomic<bool> overflow{false};
    uint32_t epoch=0;
    double rate=48000,delay=0,blockTime=0,lastCallback=0,lastInput=-1;
    int previousSamples=0;
    bool anchored=false;
    uint64_t late=0,discarded=0,resets=0;
    float blockPeak=0;
    void drain() noexcept;
    void stopAndRequireStart(Engine&) noexcept;
};
}
