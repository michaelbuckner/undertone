#include "Engine.h"
#include <algorithm>
#include <cmath>

namespace undertone {
namespace { constexpr double pi=3.14159265358979323846;
float blep(double phase,double increment) noexcept {
    if(phase<increment) { const auto t=phase/increment; return float(t+t-t*t-1); }
    if(phase>1-increment) { const auto t=(phase-1)/increment; return float(t*t+t+t+1); }
    return 0;
}
}
float Engine::Delay::tick(float input,float feedback) noexcept {
    if(data.empty()) return 0;
    const auto out=data[pos]; damp+=.28f*(out-damp);
    data[pos]=input+damp*feedback; pos=(pos+1)%data.size(); return out;
}
void Engine::Delay::clear() noexcept { std::fill(data.begin(),data.end(),0.0f); pos=0; damp=0; }
Engine::Engine() {
    // Stable bootstrap: factory-bank edits must not change the smoothing ramp of
    // an unrelated session loaded immediately after prepare (including exports).
    current.music.bars=2;current.patch.attack=1.2f;current.patch.space=.55f;
    current.phrase=generate(current.music,{},current.seed);current.detail.phrasing=true;
    next=current;previous=current;smoothed=current.patch;
}
void Engine::prepare(double sampleRate) {
    rate=sampleRate; stop(true); clockAge=0; lastMidi=-1; renderedSamples=lastOnsetSample=0;
    needsStart=true; clockRunning=false; clock=external?ClockState::waiting:ClockState::internal;
    constexpr double times[]{.0297,.0371,.0411,.0437,.0307,.0323,.0393,.0451};
    for(size_t i=0;i<reverbs.size();++i) { reverbs[i].data.assign(size_t(rate*times[i]),0); reverbs[i].clear(); }
    chorusBuffer.assign(size_t(rate*.08)+2,0); chorusPos=0;
    details.prepare(rate);
    drums.prepare(rate);
    surface.prepare(rate);
    smoothed=current.patch;
}
void Engine::emitNote(int channel,int note,float velocity,bool on,double gate) noexcept {
    if(!noteSink||note<0||note>127)return;
    MidiGate* empty=nullptr;
    for(auto& entry:midiGates){if(entry.channel==channel&&entry.note==note){noteSink(noteUser,{channel,note,0,false,renderedSamples});entry.channel=0;empty=&entry;break;}if(!entry.channel)empty=&entry;}
    if(!on||velocity<=0||!empty)return;
    noteSink(noteUser,{channel,note,std::clamp(int(velocity*127),1,127),true,renderedSamples});
    *empty={channel,note,gate<0?UINT64_MAX:renderedSamples+uint64_t(std::max(1.,gate*60*rate/(external?clockBpm:current.bpm)))};
}
void Engine::flushNotes() noexcept {if(noteSink)for(auto& entry:midiGates)if(entry.channel){noteSink(noteUser,{entry.channel,entry.note,0,false,renderedSamples});entry.channel=0;}}
void Engine::release() noexcept { for(auto& v:voices){if(v.held)emitNote(1,v.note,0,false);v.held=false;} }
void Engine::stop(bool clear) noexcept {
    playing=false; release(); details.release();drums.release(); gateSamples=-1;
    flushNotes();
    if(clear) {
        voices={}; for(auto& r:reverbs) r.clear();
        std::fill(chorusBuffer.begin(),chorusBuffer.end(),0.0f);
        hpL=hpR=prevL=prevR=peak=0;
        details.clear();lastDetailStep=-1;lastMelodyNote=-1;lastMelodyPassing=false;wearL=wearR=0;
        drums.clear();lastPercussionStep=-1;lastPercussionSample=0;
        surface.clear();form={};
    }
}
void Engine::planEvolution() noexcept {
    if(userPending) return;
    pending=false;
    if(!current.music.evolve||current.music.evolveTarget==2||current.music.variation<=0) return;
    bool unlocked=false; for(int i=0;i<current.music.slots;++i) unlocked|=!current.phrase.chords[i].locked&&!current.phrase.chords[i].rest;
    if(!unlocked) return;
    const double upcoming=std::fmod(std::max(0.0,nextEvolution-phraseStart),current.music.slots*current.music.bars*4.0);
    next=current; next.phrase=evolvePhrase(next.music,next.phrase,next.seed,int(upcoming/(current.music.bars*4))); pending=true;
}
void Engine::applyPending() noexcept {
    if(!pending) return;
    previous=current; hasUndo=true;
    // Sound and tempo controls remain live even while a harmonic edit is queued.
    if(!pendingScene){next.patch=current.patch; next.detail=current.detail;next.percussion=current.percussion;next.tonePhase=current.tonePhase;}
    next.bpm=current.bpm;
    next.cycles=current.cycles;
    next.scenes=current.scenes;next.sceneCycle=current.sceneCycle;next.sceneBars=current.sceneBars;next.sceneVariations=current.sceneVariations;
    next.sceneVariationSeed=current.sceneVariationSeed;next.swapStyle=current.swapStyle;
    if(pendingScene){++stateRevision;compareMask=0;}
    current=next; pending=false; userPending=false;
    pendingScene=false;
}
void Engine::resetCycles() noexcept {
    nextComposition=current.cycles.compositionBars*4;
    nextMotif=current.cycles.motifBars*4;
    compositionChanges=motifChanges=0;
    nextScene=current.sceneBars*4;sceneChanges=0;
}
void Engine::command(const Command& c) noexcept {
    sequence=c.sequence;
    switch(c.action) {
        case Action::play:
            lastMelodyNote=-1;lastMelodyPassing=false;
            if(external) { clock=ClockState::waiting; break; }
            if(userPending)applyPending(); playing=true; totalBeat=phraseStart=beat=0; nextEvolution=current.music.evolveBars*4; resetCycles(); slot=0; trigger=true; lastStep=-1;lastDetailStep=lastPercussionStep=-1; planEvolution(); break;
        case Action::stop: stop(false); if(external) clock=ClockState::waiting; break;
        case Action::panic: stop(true); if(external) clock=ClockState::waiting; break;
        case Action::swapStyle: current.swapStyle=c.session.swapStyle; break;
        case Action::patch: current.patch=c.session.patch; if(pending&&!pendingScene) next.patch=current.patch; break;
        case Action::detail: {
            // Mix/voice gestures may carry a UI snapshot from before an automatic motif change.
            const auto seed=current.detail.seed,variation=current.detail.variationSeed,figure=current.detail.figureSeed;
            current.detail=c.session.detail;current.detail.seed=seed;current.detail.variationSeed=variation;current.detail.figureSeed=figure;
            if(pending&&!pendingScene)next.detail=current.detail;break;
        }
        case Action::newMotif:
            advanceMotif(current.detail,true);if(pending&&!pendingScene)next.detail=current.detail;break;
        case Action::varyMotif:
            randomNext(current.detail.variationSeed);if(pending&&!pendingScene)next.detail=current.detail;break;
        case Action::newFigure:
            current.detail.figureSeed=nextMotifSeed(current.detail.figureSeed);if(pending&&!pendingScene)next.detail.figureSeed=current.detail.figureSeed;break;
        case Action::storeScene:
            if(storeScene(current,c.value)){if(pending)next.scenes=current.scenes;++stateRevision;}else ++rejected;break;
        case Action::recallScene:
            if(c.value<0||c.value>1||!current.scenes[c.value].saved){++rejected;break;}
            next=current;
            if(!recallScene(next,c.value)){++rejected;break;}
            pending=userPending=pendingScene=true;manualResetsPhrase=true;manualBoundary=(std::floor(totalBeat/4)+1)*4;
            if(!playing){applyPending();planEvolution();}break;
        case Action::sceneControls: {
            const int bars=c.session.sceneBars;
            if(bars<1||bars>64||(bars&(bars-1))){++rejected;break;}
            if(current.sceneBars!=bars||current.sceneCycle!=c.session.sceneCycle)nextScene=(std::floor(totalBeat/(bars*4))+1)*bars*4;
            current.sceneCycle=c.session.sceneCycle;current.sceneBars=bars;current.sceneVariations=c.session.sceneVariations;break;
        }
        case Action::percussion: current.percussion=c.session.percussion;if(pending&&!pendingScene)next.percussion=current.percussion;break;
        case Action::cycles: {
            const auto& controls=c.session.cycles;
            if(!validCycles(controls)){++rejected;break;}
            if(controls.composition!=current.cycles.composition||controls.compositionBars!=current.cycles.compositionBars)
                nextComposition=(std::floor(totalBeat/(controls.compositionBars*4))+1)*controls.compositionBars*4;
            if(controls.motif!=current.cycles.motif||controls.motifBars!=current.cycles.motifBars)
                nextMotif=(std::floor(totalBeat/(controls.motifBars*4))+1)*controls.motifBars*4;
            current.cycles=controls;if(pending)next.cycles=controls;break;
        }
        case Action::evolution: {
            const auto& m=c.session.music;
            current.music.evolve=m.evolve;current.music.variation=m.variation;
            current.music.evolveTarget=m.evolveTarget;current.music.evolveBars=m.evolveBars;
            nextEvolution=(std::floor(totalBeat/(m.evolveBars*4))+1)*m.evolveBars*4;
            if(userPending){next.music.evolve=m.evolve;next.music.variation=m.variation;next.music.evolveTarget=m.evolveTarget;next.music.evolveBars=m.evolveBars;}
            planEvolution();break;
        }
        case Action::tempo: current.bpm=std::clamp(c.session.bpm,40.0,200.0); break;
        case Action::external:
            stop(false); external=c.value!=0; clock=external?ClockState::waiting:ClockState::internal;
            lastMidi=-1; clockAge=0; needsStart=true; clockRunning=false; break;
        case Action::load:
            if(!valid(c.session)) { ++rejected; break; }
            stop(true); previous=current; hasUndo=true; current=c.session;
            pendingScene=false;compareMask=0;++stateRevision;
            pending=userPending=false; totalBeat=phraseStart=beat=0; slot=0; nextEvolution=current.music.evolveBars*4; evolutions=0;
            resetCycles();
            if(external) { clock=ClockState::waiting; needsStart=true; clockRunning=false; }
            planEvolution(); break;
        case Action::music: {
            const auto& m=c.session.music;
            bool locked=false; for(int i=0;i<current.music.slots;++i) locked|=current.phrase.chords[i].locked;
            if(locked&&(m.key!=current.music.key||m.mode!=current.music.mode||m.floor!=current.music.floor
                ||m.spacing!=current.music.spacing||m.slots!=current.music.slots||m.composition!=current.music.composition)) { ++rejected; break; }
            const auto& old=userPending?next.music:current.music;
            const bool harmonyChanged=m.key!=old.key||m.mode!=old.mode||m.floor!=old.floor
                ||m.spacing!=old.spacing||m.rootless!=old.rootless||m.complexity!=old.complexity||m.slots!=old.slots||m.composition!=old.composition;
            next=userPending?next:current; next.music=m;
            compareMask=0;
            if(harmonyChanged) next.phrase=generate(m,next.phrase,next.seed);
            pending=userPending=true;
            manualBoundary=phraseStart+current.music.slots*current.music.bars*4;manualResetsPhrase=true;
            if(!playing) { applyPending(); planEvolution(); }
            break;
        }
        case Action::generate:
            next=userPending?next:current; next.phrase=generate(next.music,next.phrase,next.seed);
            pending=userPending=true;
            manualBoundary=phraseStart+current.music.slots*current.music.bars*4;manualResetsPhrase=true;
            if(!playing) { applyPending(); planEvolution(); }
            break;
        case Action::swapChord:
        case Action::restChord:
        case Action::compareChord: {
            const int at=c.value;
            if(at<0||at>=current.music.slots||current.phrase.chords[at].locked
                ||(userPending&&at>=next.music.slots)){++rejected;break;}
            if(!userPending){next=current;manualBoundary=(std::floor(totalBeat/4)+1)*4;manualResetsPhrase=false;}
            if(c.action==Action::swapChord){comparison[at]=next.phrase.chords[at];compareMask|=1<<at;
                current.swapStyle=c.session.swapStyle;next.phrase=chooseChord(next,next.seed,at,current.swapStyle);}
            else if(c.action==Action::compareChord){if(!(compareMask&(1<<at))){++rejected;break;}std::swap(next.phrase.chords[at],comparison[at]);}
            else next.phrase.chords[at].rest=!next.phrase.chords[at].rest;
            pending=userPending=true;
            if(!playing){applyPending();planEvolution();}
            break;
        }
        case Action::lock:
            if(c.value>=0&&c.value<current.music.slots) {
                auto& chord=current.phrase.chords[c.value]; chord.locked=!chord.locked;
                if(chord.locked&&pending&&(next.music.key!=current.music.key||next.music.mode!=current.music.mode
                    ||next.music.floor!=current.music.floor||next.music.spacing!=current.music.spacing||next.music.slots!=current.music.slots||next.music.composition!=current.music.composition)) {
                    pending=userPending=pendingScene=false; ++rejected;
                }
                if(pending) {
                    if(chord.locked) next.phrase.chords[c.value]=chord;
                    else next.phrase.chords[c.value].locked=false;
                }
                if(!userPending) planEvolution();
            }
            break;
        case Action::lockAll:
            for(int i=0;i<current.music.slots;++i) current.phrase.chords[i].locked=c.value!=0;
            pending=userPending=pendingScene=false; if(!c.value) planEvolution(); break;
        case Action::undo:
            if(hasUndo) { next=previous; next.patch=current.patch; next.bpm=current.bpm; pending=userPending=true;pendingScene=false; hasUndo=false;
                manualBoundary=phraseStart+current.music.slots*current.music.bars*4;manualResetsPhrase=true;
                if(!playing) { applyPending(); planEvolution(); }
            } break;
    }
}
void Engine::midi(uint8_t byte,double time) noexcept {
    if(!external) return;
    if(byte==0xfa) {
        lastMelodyNote=-1;lastMelodyPassing=false;
        if(userPending)applyPending(); totalBeat=phraseStart=beat=0; nextEvolution=current.music.evolveBars*4; resetCycles(); midiTicks=0; slot=0; lastStep=-1;lastDetailStep=lastPercussionStep=-1;
        trigger=true; playing=true; clockAge=0;
        if(lastMidi<0)clockBpm=current.bpm;
        needsStart=false; clockRunning=false;
        clock=ClockState::waiting; planEvolution();
    } else if(byte==0xfc) { stop(false); clock=ClockState::waiting; }
    else if(byte==0xfb && !needsStart) { playing=true; trigger=true; lastStep=-1; clockAge=0; clock=ClockState::waiting; clockRunning=false; }
    else if(byte==0xf8) {
        if(lastMidi>=0) {
            const double dt=time-lastMidi;
            if(dt>0.007&&dt<0.1) clockBpm += .15*(std::clamp(60.0/(dt*24),40.0,200.0)-clockBpm);
        }
        lastMidi=time; clockAge=0;
        if(playing) {
            clockRunning=true;clock=ClockState::locked;
            // All MVP onsets lie on the MIDI tick grid. Count ticks for phase;
            // a tempo estimate controls gate duration/display, never note-on position.
            totalBeat=double(midiTicks)/24.0;
            advanceTransport();
            ++midiTicks;
        }
    }
}
void Engine::startChord() noexcept {
    const bool legato=current.music.composition!=Composition::song&&current.music.rhythm==Rhythm::sustain;
    if(!legato)release();
    std::array<bool,16> retained{};
    const auto& chord=current.phrase.chords[slot];
    if(chord.rest){release();details.release();return;}
    for(int n=0;n<chord.count;++n) {
        const double frequency=440.0*std::pow(2.0,(chord.notes[n]-69)/12.0);
        bool found=false;
        if(legato)for(size_t i=0;i<voices.size();++i)if(voices[i].active&&voices[i].held&&std::abs(voices[i].frequency-frequency)<.001){retained[i]=true;found=true;break;}
        if(found){
            if(noteSink){bool exported=false;for(const auto& entry:midiGates)exported|=entry.channel==1&&entry.note==chord.notes[n];
                const float amount=current.detail.bed*form.bed;if(amount<=0)emitNote(1,chord.notes[n],0,false);else if(!exported)emitNote(1,chord.notes[n],.72f*amount,true);}
            continue;
        }
        Voice* choice=nullptr;
        for(auto& v:voices) if(!v.active) { choice=&v; break; }
        if(!choice) {
            choice=&voices[0];
            for(auto& v:voices) if(!v.held&&v.env<choice->env) choice=&v;
        }
        // Preserve phase/envelope when stealing a tail to avoid hard waveform resets.
        const bool wasActive=choice->active;
        if(choice->held)emitNote(1,choice->note,0,false);
        choice->active=choice->held=true;
        choice->frequency=440.0*std::pow(2.0,(chord.notes[n]-69)/12.0);
        if(!wasActive) { *choice=Voice{}; choice->active=choice->held=true; choice->frequency=440.0*std::pow(2.0,(chord.notes[n]-69)/12.0); choice->a=double(n)*.13; choice->b=double(n)*.17; choice->c=double(n)*.19; }
        choice->attacking=true;
        choice->note=chord.notes[n];emitNote(1,choice->note,.72f*current.detail.bed*form.bed,true);
        retained[size_t(choice-voices.data())]=true;
        choice->pan=chord.count>1?float(n)/float(chord.count-1):.5f;
    }
    if(legato)for(size_t i=0;i<voices.size();++i)if(!retained[i]){if(voices[i].held)emitNote(1,voices[i].note,0,false);voices[i].held=false;}
    ++notesTriggered;
    lastOnsetSample=renderedSamples;
}
void Engine::advanceTransport() noexcept {
    bool appliedManual=false;
    beat=std::max(0.0,totalBeat-phraseStart);
    const double length=current.music.slots*current.music.bars*4.0;
    if(beat+1e-9>=length) {
        phraseStart+=length;beat=std::max(0.0,totalBeat-phraseStart);
        lastStep=-1;trigger=true;
    }
    if(userPending&&totalBeat+1e-9>=manualBoundary){
        const bool reset=manualResetsPhrase,scene=pendingScene;
        const int sounding=std::clamp(int(beat/(current.music.bars*4)),0,current.music.slots-1);
        const auto before=current.phrase.chords[sounding];applyPending();appliedManual=true;
        if(reset){phraseStart=manualBoundary;beat=std::max(0.0,totalBeat-phraseStart);lastStep=-1;trigger=true;}
        else if(!sameChord(before,current.phrase.chords[sounding]))trigger=true;
        if(scene)nextEvolution=(std::floor((totalBeat+1e-9)/(current.music.evolveBars*4))+1)*current.music.evolveBars*4;
    }
    if(totalBeat+1e-9>=nextEvolution) {
        const int sounding=std::clamp(int(beat/(current.music.bars*4)),0,current.music.slots-1);
        const auto before=current.phrase.chords[sounding];
        if(pending&&!userPending){applyPending();++evolutions;}
        if(!sameChord(before,current.phrase.chords[sounding]))trigger=true;
        nextEvolution=(std::floor((totalBeat+1e-9)/(current.music.evolveBars*4))+1)*current.music.evolveBars*4;
        planEvolution();
    } else if(appliedManual)planEvolution();
    bool cycled=false;
    if(totalBeat+1e-9>=nextScene){
        if(current.sceneCycle&&current.scenes[0].saved&&current.scenes[1].saved&&!userPending&&!appliedManual){
            previous=current;hasUndo=true;recallScene(current,current.activeScene==0?1:0,current.sceneVariations);
            phraseStart=totalBeat;beat=0;trigger=true;lastStep=-1;compareMask=0;++sceneChanges;++stateRevision;cycled=true;appliedManual=true;
            nextEvolution=(std::floor((totalBeat+1e-9)/(current.music.evolveBars*4))+1)*current.music.evolveBars*4;
        }
        nextScene=(std::floor((totalBeat+1e-9)/(current.sceneBars*4))+1)*current.sceneBars*4;
    }
    if(totalBeat+1e-9>=nextComposition){
        if(current.cycles.composition&&!userPending&&!appliedManual&&current.music.composition!=Composition::song){
            alternateComposition(current);++compositionChanges;cycled=true;trigger=true;
        }
        nextComposition=(std::floor((totalBeat+1e-9)/(current.cycles.compositionBars*4))+1)*current.cycles.compositionBars*4;
    }
    if(totalBeat+1e-9>=nextMotif){
        if(current.cycles.motif&&current.detail.motifMode!=MotifMode::hold&&!userPending&&!appliedManual){advanceMotif(current.detail);++motifChanges;cycled=true;}
        nextMotif=(std::floor((totalBeat+1e-9)/(current.cycles.motifBars*4))+1)*current.cycles.motifBars*4;
    }
    if(cycled)planEvolution();
    slot=std::clamp(int((beat+1e-9)/(current.music.bars*4)),0,current.music.slots-1);
    const auto detailPosition=int64_t(std::floor(totalBeat*2+1e-9));
    if(detailPosition!=lastDetailStep){
        form=compositionFrame(current,totalBeat);
        MelodyContext context;context.phraseOrigin=int64_t(std::llround(phraseStart*2));
        context.previousNote=lastMelodyNote;context.previousPassing=lastMelodyPassing;
        // Fixed-size timeline of committed edits and eight future deterministic boundaries.
        // The same priority applies here and in transport: manual/scene recalls win a shared boundary.
        std::array<Session,8> forecasts{};Session simulated=current;
        double origin=phraseStart,evolutionAt=nextEvolution,compositionAt=nextComposition,motifAt=nextMotif,sceneAt=nextScene;
        auto after=[](double at,int bars){return (std::floor((at+1e-9)/(bars*4))+1)*bars*4;};
        for(int n=0;n<8;++n){double boundary=1.e30;const bool manual=userPending&&n==0;
            if(manual)boundary=manualBoundary;
            else {
                bool eligible=false;for(int i=0;i<simulated.music.slots;++i)eligible|=!simulated.phrase.chords[i].locked&&!simulated.phrase.chords[i].rest;
                if(simulated.music.evolve&&simulated.music.evolveTarget!=2&&simulated.music.variation>0&&eligible)boundary=evolutionAt;
                if(simulated.cycles.composition&&simulated.music.composition!=Composition::song)boundary=std::min(boundary,compositionAt);
                if(simulated.cycles.motif&&simulated.detail.motifMode!=MotifMode::hold)boundary=std::min(boundary,motifAt);
                if(simulated.sceneCycle&&simulated.scenes[0].saved&&simulated.scenes[1].saved)boundary=std::min(boundary,sceneAt);
            }
            if(boundary>1.e29)break;
            if(manual){simulated=next;
                if(!pendingScene){simulated.patch=current.patch;simulated.detail=current.detail;simulated.percussion=current.percussion;}
                simulated.scenes=current.scenes;simulated.cycles=current.cycles;simulated.sceneCycle=current.sceneCycle;simulated.sceneBars=current.sceneBars;simulated.sceneVariations=current.sceneVariations;
                if(manualResetsPhrase)origin=boundary;
                if(pendingScene)evolutionAt=after(boundary,simulated.music.evolveBars);
            }else if(simulated.sceneCycle&&simulated.scenes[0].saved&&simulated.scenes[1].saved&&sceneAt==boundary){
                recallScene(simulated,simulated.activeScene==0?1:0,simulated.sceneVariations);origin=boundary;
                evolutionAt=after(boundary,simulated.music.evolveBars);
            }else {
                if(evolutionAt==boundary&&simulated.music.evolve&&simulated.music.evolveTarget!=2){
                    const int at=int(std::fmod(std::max(0.,boundary-origin),simulated.music.slots*simulated.music.bars*4.)/(simulated.music.bars*4));
                    simulated.phrase=evolvePhrase(simulated.music,simulated.phrase,simulated.seed,at);
                }
                if(simulated.cycles.composition&&compositionAt==boundary)alternateComposition(simulated);
                if(simulated.cycles.motif&&motifAt==boundary)advanceMotif(simulated.detail);
            }
            forecasts[n]=simulated;context.changes[n]={&forecasts[n],int64_t(std::llround(boundary*2)),int64_t(std::llround(origin*2))};++context.changeCount;
            if(evolutionAt<=boundary)evolutionAt=after(boundary,simulated.music.evolveBars);
            if(compositionAt<=boundary)compositionAt=after(boundary,simulated.cycles.compositionBars);
            if(motifAt<=boundary)motifAt=after(boundary,simulated.cycles.motifBars);
            if(sceneAt<=boundary)sceneAt=after(boundary,simulated.sceneBars);
        }
        if(!sameChord(detailChord,current.phrase.chords[slot])){details.harmonize(current.phrase.chords[slot]);detailChord=current.phrase.chords[slot];
            if(noteSink)for(auto& entry:midiGates)if((entry.channel==2||entry.channel==3)&&!chordContains(detailChord,entry.note))emitNote(entry.channel,entry.note,0,false);}
        const auto events=detailStep(current,detailPosition,context);
        for(int i=0;i<events.count;++i){const auto& event=events.events[i];details.trigger(event,external?clockBpm:current.bpm);
            emitNote(event.role==1?3:2,event.note,event.velocity*(event.role==1?current.detail.motif:current.detail.broken),true,event.role==1?event.gateBeats:std::min(1.f,event.gateBeats));
            if(event.role==1){lastMelodyNote=event.note;lastMelodyPassing=!chordContains(current.phrase.chords[slot],event.note);}}
        lastDetailStep=detailPosition;
    }
    const auto rhythm=current.music.rhythm;
    const auto percussionPosition=int64_t(std::floor(totalBeat*8+1e-9));
    if(percussionPosition!=lastPercussionStep){
        const auto events=percussionStep(current.percussion,percussionPosition);
        for(int i=0;i<events.count;++i){drums.trigger(events.hits[i],current.percussion.color);
            const int notes[]{36,37,42,39};emitNote(10,notes[events.hits[i].sound],events.hits[i].velocity*current.percussion.level,true,.125);}
        if(events.count)lastPercussionSample=renderedSamples;
        lastPercussionStep=percussionPosition;
    }
    const int step=int(std::floor(beat*4+1e-9));
    bool hit=trigger;
    if(step!=lastStep) {
        if(rhythm==Rhythm::sustain)hit|=(step%(current.music.bars*16))==0;
        if(rhythm==Rhythm::quarter)hit|=(step%4)==0;
        if(rhythm==Rhythm::eighth)hit|=(step%2)==0;
        if(rhythm==Rhythm::syncopated){const int pos=step%16;hit|=pos==0||pos==3||pos==6||pos==10||pos==14;}
        lastStep=step;
    }
    if(hit){
        startChord();trigger=false;
        gateSamples=rhythm==Rhythm::sustain?-1:std::round((rhythm==Rhythm::quarter?.36:.18)*60.0*rate/(external?clockBpm:current.bpm));
    }
}
void Engine::process(float* left,float* right,int count) noexcept {
    const float smoothing=float(1-std::exp(-1.0/(.025*rate)));
    const float attack=float(1-std::exp(-1.0/(std::max(.005f,current.patch.attack)*rate)));
    const float releaseRate=float(std::exp(-1.0/(std::max(.05f,current.patch.release)*rate*.18)));
    const float shapedAttack=float(1/(std::max(.005f,current.patch.attack)*rate));
    const float shapedDecay=float(1-std::exp(-4/(current.patch.decay*rate)));
    const double detune=std::pow(2.0,current.patch.detune/1200.0);
    peak=0;
    for(int sample=0;sample<count;++sample) {
        if(noteSink)for(auto& entry:midiGates)if(entry.channel&&renderedSamples>=entry.end)emitNote(entry.channel,entry.note,0,false);
        if(external&&playing) {
            clockAge+=1.0/rate;
            if(clockAge>.5) { stop(false); clock=ClockState::lost; needsStart=true; clockRunning=false; }
        }
        if(playing&&!external){advanceTransport();totalBeat+=current.bpm/(60.0*rate);}
        if(gateSamples==0){release();gateSamples=-1;}
        detailBed+=smoothing*(current.detail.bed*form.bed-detailBed);
        detailWear+=smoothing*(current.detail.wear-detailWear);
        const auto& music=current.music;
        const bool toneEvolve=music.evolve&&music.evolveTarget!=1&&music.variation>0;
        if(playing&&toneEvolve&&(!external||clockRunning)) {
            current.tonePhase+=(external?clockBpm:current.bpm)/(60.0*rate*music.evolveBars*4);
            if(current.tonePhase>=1)current.tonePhase-=1;
        }
        const float depth=toneEvolve?music.variation:0;
        const float morph=float(std::sin(2*pi*current.tonePhase));
        const float shade=float(std::sin(2*pi*current.tonePhase+1.1));
        const float brightness=std::clamp(current.patch.brightness+depth*.38f*morph,0.02f,1.f);
        const float texture=std::clamp(current.patch.texture+depth*.45f*shade,0.f,1.f);
        smoothed.brightness+=smoothing*((depth>0?brightness:current.patch.brightness)-smoothed.brightness);
        smoothed.texture+=smoothing*(texture-smoothed.texture);
        smoothed.resonance+=smoothing*(current.patch.resonance-smoothed.resonance);
        smoothed.motion+=smoothing*(current.patch.motion-smoothed.motion);
        smoothed.space+=smoothing*(std::clamp(current.patch.space+depth*.15f*shade,0.f,1.f)-smoothed.space);
        smoothed.level+=smoothing*(current.patch.level-smoothed.level);
        smoothed.drive+=smoothing*(current.patch.drive-smoothed.drive);
        smoothed.chorus+=smoothing*(current.patch.chorus-smoothed.chorus);
        smoothed.highpass+=smoothing*(current.patch.highpass-smoothed.highpass);
        lfo+=(.07+smoothed.motion*.6)/rate; if(lfo>=1) lfo-=1;
        const float motion=float(std::sin(2*pi*lfo));
        const double cutoff=std::clamp(250.0+smoothed.brightness*6500.0+motion*smoothed.motion*900.0,100.0,10000.0);
        const float filter=float(1-std::exp(-2*pi*cutoff/rate));
        float l=0,r=0;
        for(int model=0;model<4;++model) modelWeights[model]+=smoothing*((int(current.patch.model)==model?1.f:0.f)-modelWeights[model]);
        for(auto& v:voices) if(v.active) {
            v.env=v.held?v.env+attack*(1-v.env):v.env*releaseRate;
            if(v.held) {
                if(v.attacking){v.shaped+=shapedAttack;if(v.shaped>=1){v.shaped=1;v.attacking=false;}}
                else v.shaped+=shapedDecay*(current.patch.sustain-v.shaped);
            } else v.shaped*=releaseRate;
            if(!v.held&&v.env<.00002f&&v.shaped<.00002f) { v.active=false; continue; }
            const double drift=detailWear>0?1+detailWear*(.0018*std::sin(renderedSamples/rate*2*pi*.29+v.pan)+.00055*std::sin(renderedSamples/rate*2*pi*4.7)):1;
            const double da=v.frequency/rate*drift,db=da*detune;
            float waveA=float(2*v.a-1)-blep(v.a,da),waveB=float(2*v.b-1)-blep(v.b,db);
            const float signal=.28f*(waveA+waveB)+.44f*float(std::sin(2*pi*v.a));
            v.low+=filter*(.35f+.65f*v.env)*(signal-v.low);
            float voiceOut=modelWeights[0]*v.low*v.env;
            const double dc=da/detune;
            const float waveC=float(2*v.c-1)-blep(v.c,dc);
            const double shapedCutoff=std::clamp(90.0+std::pow(double(smoothed.brightness),2)*9500.0+current.patch.filterEnv*v.shaped*6000.0,70.0,rate*.18);
            const float pole=float(1-std::exp(-2*pi*shapedCutoff/(rate*2)));
            if(modelWeights[1]>.00001f) {
                const float source=(waveA+waveB+waveC)*(.22f+.1f*smoothed.texture)+float(std::sin(2*pi*v.a))*(.3f-.2f*smoothed.texture);
                for(int substep=0;substep<2;++substep) {
                    float x=std::tanh(source*(1+smoothed.texture)-3.5f*smoothed.resonance*v.ladder[3]);
                    for(auto& z:v.ladder){z+=pole*(std::tanh(x)-std::tanh(z));x=z;}
                }
                voiceOut+=modelWeights[1]*v.ladder[3]*v.shaped*2.f;
            }
            if(modelWeights[2]>.00001f) {
                const double width=.5+.28*smoothed.texture*motion;
                double edge=v.a-width;if(edge<0)edge+=1;
                const float pulse=(v.a<width?1.f:-1.f)+blep(v.a,da)-blep(edge,da);
                double subEdge=v.sub-.5;if(subEdge<0)subEdge+=1;
                const float sub=(v.sub<.5?1.f:-1.f)+blep(v.sub,da*.5)-blep(subEdge,da*.5);
                const float source=.38f*waveA+(.15f+.3f*smoothed.texture)*pulse+.14f*sub;
                for(int substep=0;substep<2;++substep) {
                    float x=source-3.f*smoothed.resonance*v.dco[3];
                    for(auto& z:v.dco){z+=pole*(x-z);x=z;}
                }
                voiceOut+=modelWeights[2]*v.dco[3]*v.shaped*1.5f;
            }
            if(modelWeights[3]>.00001f) {
                // Two modulator/carrier pairs. The second pair introduces a quiet, slowly beating colour.
                const double ratios[]{1,2,1.003,3.01};
                for(int op=0;op<4;++op){v.fmPhase[op]+=da*ratios[op];v.fmPhase[op]-=std::floor(v.fmPhase[op]);}
                const float index=(.15f+smoothed.texture*4.5f)*(.35f+.65f*v.shaped);
                const float fm=.65f*float(std::sin(2*pi*v.fmPhase[0]+index*std::sin(2*pi*v.fmPhase[1])))
                    +.35f*float(std::sin(2*pi*v.fmPhase[2]+index*.7*std::sin(2*pi*v.fmPhase[3])));
                v.fmLow+=filter*(fm-v.fmLow);
                voiceOut+=modelWeights[3]*v.fmLow*v.shaped*.85f;
            }
            const float out=voiceOut*.095f;
            l+=out*(.85f-.35f*v.pan); r+=out*(.5f+.35f*v.pan);
            v.a+=da; if(v.a>=1) v.a-=1;
            v.b+=db; if(v.b>=1) v.b-=1;
            v.c+=dc; if(v.c>=1)v.c-=1;
            v.sub+=da*.5;if(v.sub>=1)v.sub-=1;
        }
        l*=detailBed;r*=detailBed;
        float dryL=0,dryR=0;
        if(current.music.composition==Composition::song)details.process(l,r,current.detail,external?clockBpm:current.bpm,playing);
        else {details.process(dryL,dryR,current.detail,external?clockBpm:current.bpm,playing);l+=dryL*.15f;r+=dryR*.15f;}
        surface.process(l,r,current.phrase.chords[slot],current.detail.surface*form.surface,playing&&!current.phrase.chords[slot].rest);
        const float gain=1+smoothed.drive*8;
        l=std::tanh(l*gain)/(1+smoothed.drive*2); r=std::tanh(r*gain)/(1+smoothed.drive*2);
        if(!chorusBuffer.empty()) {
            chorusBuffer[chorusPos]=(l+r)*.5f;
            const double delay=rate*(.014+.004*std::sin(2*pi*lfo*.77));
            double position=double(chorusPos)-delay; if(position<0) position+=double(chorusBuffer.size());
            const auto i=size_t(position); const float f=float(position-double(i));
            const float wet=chorusBuffer[i]*(1-f)+chorusBuffer[(i+1)%chorusBuffer.size()]*f;
            l+=wet*smoothed.chorus*.3f; r-=wet*smoothed.chorus*.3f;
            if(modelWeights[2]>.00001f) {
                double other=double(chorusPos)-rate*(.019+.005*std::sin(2*pi*lfo*.77+1.8));
                if(other<0)other+=double(chorusBuffer.size());
                const auto j=size_t(other);const float mix=float(other-double(j));
                const float wetR=chorusBuffer[j]*(1-mix)+chorusBuffer[(j+1)%chorusBuffer.size()]*mix;
                l+=wet*smoothed.chorus*.5f*modelWeights[2];r+=wetR*smoothed.chorus*.8f*modelWeights[2];
            }
            chorusPos=(chorusPos+1)%chorusBuffer.size();
        }
        float revL=0,revR=0;
        const float extended=(1-modelWeights[0])*smoothed.space;
        for(size_t i=0;i<4;++i) { revL+=reverbs[i].tick(l*.22f,.72f+extended*.18f); revR+=reverbs[i+4].tick(r*.22f,.73f+extended*.17f); }
        l+=revL*smoothed.space*.55f; r+=revR*smoothed.space*.55f;
        l+=dryL*.85f;r+=dryR*.85f;
        if(detailWear>0){
            const float wornFilter=float(1-std::exp(-2*pi*(6500-4000*detailWear)/rate));
            wearL+=wornFilter*(l-wearL);wearR+=wornFilter*(r-wearR);
            l+=(std::tanh(wearL*1.4f)/1.4f-l)*detailWear;
            r+=(std::tanh(wearR*1.4f)/1.4f-r)*detailWear;
        }else{wearL=l;wearR=r;}
        const float hp=float(std::exp(-2*pi*smoothed.highpass/rate));
        hpL=hp*(hpL+l-prevL); hpR=hp*(hpR+r-prevR); prevL=l; prevR=r;
        float finalL=hpL,finalR=hpR;drums.process(finalL,finalR,current.percussion);
        const float outL=std::tanh(finalL*smoothed.level),outR=std::tanh(finalR*smoothed.level);
        if(left) left[sample]=outL; if(right) right[sample]=outR;
        peak=std::max(peak,std::max(std::abs(outL),std::abs(outR)));
        if(gateSamples>0)--gateSamples;
        ++renderedSamples;
    }
}
Status Engine::status() const noexcept {
    Status s; s.session=current; s.next=next; s.playing=playing; s.pending=pending;
    s.canUndo=hasUndo; s.external=external; s.slot=slot; s.beat=beat;
    s.clock=clock; s.notesTriggered=notesTriggered; s.peak=peak; s.rejected=rejected; s.sequence=sequence;
    s.lastOnsetSample=lastOnsetSample;
    s.evolutionBeats=std::max(0.0,nextEvolution-totalBeat);s.evolutions=evolutions;
    s.compositionBeats=std::max(0.0,nextComposition-totalBeat);s.motifBeats=std::max(0.0,nextMotif-totalBeat);
    s.compositionChanges=compositionChanges;s.motifChanges=motifChanges;s.manualPending=userPending;
    s.sceneBeats=std::max(0.0,nextScene-totalBeat);s.sceneChanges=sceneChanges;s.stateRevision=stateRevision;s.compareMask=compareMask;
    s.detailNotes=details.notes;s.lastMelodyNote=lastMelodyNote;s.section=form.section;
    s.percussionHits=drums.hits;s.lastPercussionSample=lastPercussionSample;
    s.toneBrightness=smoothed.brightness;s.toneTexture=smoothed.texture;
    if(external) s.session.bpm=clockBpm;
    return s;
}
}
