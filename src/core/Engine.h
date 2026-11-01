#pragma once
#include "Music.h"
#include "Detail.h"
#include "Percussion.h"
#include "Composition.h"
#include <atomic>
#include <vector>

namespace undertone {
// Single producer / single consumer. No allocation or mutex on the audio thread.
template<typename T, size_t Capacity> class Queue {
public:
    bool push(const T& value) noexcept {
        const auto w=write.load(std::memory_order_relaxed), next=(w+1)%Capacity;
        if(next==read.load(std::memory_order_acquire)) return false;
        data[w]=value; write.store(next,std::memory_order_release); return true;
    }
    bool pop(T& value) noexcept {
        const auto r=read.load(std::memory_order_relaxed);
        if(r==write.load(std::memory_order_acquire)) return false;
        value=data[r]; read.store((r+1)%Capacity,std::memory_order_release); return true;
    }
    bool peek(T& value) const noexcept {
        const auto r=read.load(std::memory_order_relaxed);
        if(r==write.load(std::memory_order_acquire))return false;
        value=data[r];return true;
    }
private:
    std::array<T,Capacity> data{};
    std::atomic<size_t> write{0},read{0};
};
enum class Action { play, stop, panic, generate, music, patch, tempo, lock, lockAll, undo, load, external, evolution, detail, percussion, swapChord, restChord, cycles, newMotif, varyMotif, newFigure, storeScene, recallScene, sceneControls, compareChord, swapStyle };
struct Command { Action action{}; Session session{}; int value=0; uint64_t sequence=0; uint32_t midiEpoch=0; };
enum class ClockState { internal, waiting, locked, lost };
struct Status {
    Session session{}, next{};
    bool playing=false, pending=false, canUndo=false, external=false;
    int slot=0, rejected=0;
    double beat=0;
    ClockState clock=ClockState::internal;
    uint64_t notesTriggered=0;
    uint64_t lastOnsetSample=0;
    uint64_t sequence=0;
    uint64_t lateMidi=0, discardedMidi=0, midiResets=0;
    double midiDelayMs=0;
    float peak=0;
    double evolutionBeats=0;
    float toneBrightness=0, toneTexture=0;
    uint64_t evolutions=0;
    double compositionBeats=0,motifBeats=0;
    uint64_t compositionChanges=0,motifChanges=0;
    bool manualPending=false;
    double sceneBeats=0;
    uint64_t sceneChanges=0,stateRevision=0;
    int compareMask=0;
    uint64_t detailNotes=0;
    int lastMelodyNote=-1;
    int section=0;
    uint64_t percussionHits=0,lastPercussionSample=0;
};
class Engine {
public:
    struct NoteEvent {int channel=1,note=60,velocity=0;bool on=false;uint64_t sample=0;};
    using NoteSink=void(*)(void*,const NoteEvent&);
    Engine();
    void prepare(double sampleRate);
    void command(const Command&) noexcept;
    void midi(uint8_t byte, double timeSeconds) noexcept;
    void process(float* left, float* right, int count) noexcept;
    Status status() const noexcept;
    const Session& session() const noexcept { return current; }
    void setNoteSink(NoteSink sink,void* user) noexcept {noteSink=sink;noteUser=user;}
private:
    struct Voice {
        bool active=false, held=false, attacking=true;
        double a=0,b=0,c=0,sub=0,frequency=0;
        std::array<double,4> fmPhase{};
        std::array<float,4> ladder{},dco{};
        float env=0, shaped=0, low=0, fmLow=0, pan=0;
        int note=-1;
    };
    struct Delay { std::vector<float> data; size_t pos=0; float damp=0; float tick(float,float) noexcept; void clear() noexcept; };
    Session current{}, next{}, previous{};
    bool playing=false, pending=false, userPending=false, hasUndo=false, external=false, trigger=true;
    bool needsStart=true, clockRunning=false;
    int slot=0, lastStep=-1, rejected=0;
    double rate=48000, beat=0, totalBeat=0, phraseStart=0, gateSamples=-1, lastMidi=-1, clockAge=0, clockBpm=88;
    uint64_t midiTicks=0, notesTriggered=0;
    uint64_t renderedSamples=0, lastOnsetSample=0;
    uint64_t sequence=0;
    double nextEvolution=8;
    double manualBoundary=0,nextComposition=64,nextMotif=32;
    double nextScene=64;
    bool pendingScene=false;
    uint64_t sceneChanges=0,stateRevision=0;
    std::array<Chord,maxSlots> comparison{};
    int compareMask=0;
    bool manualResetsPhrase=true;
    uint64_t compositionChanges=0,motifChanges=0;
    uint64_t evolutions=0;
    std::array<float,4> modelWeights{{1,0,0,0}};
    DetailEngine details;
    PercussionEngine drums;
    SurfaceEngine surface;
    CompositionFrame form;
    int64_t lastPercussionStep=-1;
    uint64_t lastPercussionSample=0;
    int64_t lastDetailStep=-1;
    int lastMelodyNote=-1;
    bool lastMelodyPassing=false;
    Chord detailChord{};
    float detailBed=1,detailWear=0,wearL=0,wearR=0;
    ClockState clock=ClockState::internal;
    std::array<Voice,16> voices{};
    std::array<Delay,8> reverbs{};
    std::vector<float> chorusBuffer;
    size_t chorusPos=0;
    double lfo=0;
    Patch smoothed{};
    float hpL=0,hpR=0,prevL=0,prevR=0,peak=0;
    NoteSink noteSink=nullptr;void* noteUser=nullptr;
    struct MidiGate {int channel=0,note=0;uint64_t end=0;};
    std::array<MidiGate,64> midiGates{};
    void emitNote(int channel,int note,float velocity,bool on,double gateBeats=-1) noexcept;
    void flushNotes() noexcept;
    void release() noexcept;
    void startChord() noexcept;
    void advanceTransport() noexcept;
    void planEvolution() noexcept;
    void applyPending() noexcept;
    void resetCycles() noexcept;
    void stop(bool clear) noexcept;
};
}
