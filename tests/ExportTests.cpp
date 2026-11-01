#include "app/MidiExport.h"
#include "app/SessionFile.h"
#include <cstdlib>
#include <iostream>
using namespace undertone;
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
juce::MemoryBlock contents(const juce::File& file){juce::MemoryBlock data;check(file.loadFileAsData(data),"read exported file");return data;}
int main(){
    auto s=makePreset(8);s.music.bars=1;s.music.evolve=false;s.detail.breathe=false;s.percussion.enabled=true;s.bpm=120;
    storeScene(s,0);auto b=makePreset(11);static_cast<MusicalState&>(s)=b;storeScene(s,1);recallScene(s,0);s.bpm=120;s.sceneCycle=true;s.sceneBars=2;
    Session recalled;check(decodeSession(encodeSession(s),recalled).wasOk()&&encodeSession(s)==encodeSession(recalled),"full scene and performance session roundtrip");
    auto malformed=juce::JSON::parse(encodeSession(s));(*malformed["scenes"].getArray())[0].getDynamicObject()->setProperty("scenes",juce::Array<juce::var>{});
    check(decodeSession(juce::JSON::toString(malformed),recalled).failed(),"nested scene bank rejected");
    auto legacy=juce::JSON::parse(encodeSession(s));legacy.getDynamicObject()->setProperty("schema",7);
    check(decodeSession(juce::JSON::toString(legacy),recalled).wasOk()&&!recalled.detail.phrasing&&!recalled.scenes[0].saved&&recalled.detail.figureSeed==recalled.detail.seed,"legacy state migrates without new phrasing or scene bank");
    // Keep the structural MIDI check stable; scene rendering is checked separately below.
    s.sceneCycle=false;
    auto file=juce::File::getCurrentWorkingDirectory().getChildFile("test-export.mid");
    check(exportMidiFile(file,s,4).wasOk(),"MIDI export writes");auto bytes=contents(file);
    check(exportMidiFile(file,s,4).wasOk()&&bytes==contents(file),"MIDI export deterministic");
    juce::MidiFile midi;auto input=file.createInputStream();check(input&&midi.readFrom(*input)&&midi.getNumTracks()==5&&midi.getTimeFormat()==960,"type 1 MIDI with tempo plus four role tracks");
    int onsets[4]{};for(int track=1;track<5;++track){int active[128]{};auto* sequence=midi.getTrack(track);
        for(int i=0;i<sequence->getNumEvents();++i){const auto& m=sequence->getEventPointer(i)->message;check(m.getTimeStamp()>=0&&m.getTimeStamp()<=15360,"events within four bars");
            if(m.isNoteOn()){++active[m.getNoteNumber()];++onsets[track-1];check(m.getChannel()==(track==4?10:track),"separate role channels");}
            if(m.isNoteOff()){--active[m.getNoteNumber()];check(active[m.getNoteNumber()]>=0,"note off paired with onset");}}
        for(int count:active)check(count==0,"no stuck notes at file end");check(onsets[track-1]>0,"each active layer exported");}
    input.reset();s.sceneCycle=true;check(exportMidiFile(file,s,4).wasOk()&&bytes!=contents(file),"scene changes are rendered into MIDI");
    input.reset();auto muted=makePreset(10);muted.bpm=120;muted.music.evolve=false;muted.detail.breathe=false;muted.detail.bed=0;storeScene(muted,0);
    muted.detail.bed=1;storeScene(muted,1);recallScene(muted,0);muted.sceneCycle=true;muted.sceneBars=1;
    check(exportMidiFile(file,muted,4).wasOk(),"muted/unmuted legato scene export");input=file.createInputStream();juce::MidiFile gates;check(gates.readFrom(*input),"read legato scenes");
    int padOnsets=0;int held[128]{};for(int i=0;i<gates.getTrack(1)->getNumEvents();++i){const auto& m=gates.getTrack(1)->getEventPointer(i)->message;
        if(m.isNoteOn()){++padOnsets;++held[m.getNoteNumber()];check(m.getTimeStamp()>=3840&&!(m.getTimeStamp()>=7680&&m.getTimeStamp()<11520),"no pad onset in muted scene");}
        if(m.isNoteOff())check(--held[m.getNoteNumber()]>=0,"legato scene off has an onset");}
    check(padOnsets>0,"held pad notes begin when scene unmutes");for(int count:held)check(count==0,"legato scene notes end");
    input.reset();file.deleteFile();check(exportMidiFile(file,s,0).failed(),"invalid export rejected");
    std::cout<<"PASS scene serialization, legacy migration, reproducible MIDI tracks, timing, note cleanup and scene performance\n";
}
