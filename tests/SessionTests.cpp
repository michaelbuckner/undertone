#include "app/SessionFile.h"
#include <iostream>
#include <cstdlib>
using namespace undertone;
void expect(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
    for(int i=0;i<presetCount;++i){auto s=makePreset(i);s.music.evolve=true;s.phrase.chords[1].locked=true;Session restored;
        expect(decodeSession(encodeSession(s),restored).wasOk(),"roundtrip failed");
        expect(encodeSession(s)==encodeSession(restored),"roundtrip state mismatch");
        auto a=s.seed,b=restored.seed;auto next=generate(s.music,s.phrase,a,true),replay=generate(restored.music,restored.phrase,b,true);
        for(int j=0;j<s.music.slots;++j)expect(sameChord(next.chords[j],replay.chords[j]),"evolution replay mismatch");
    }
    auto original=makePreset(2);auto dest=original;
    {auto state=makePreset(10);state.cycles={true,true,64,2};state.phrase.chords[0].rest=true;state.phrase.chords[0].locked=true;
        expect(decodeSession(encodeSession(state),dest).wasOk()&&encodeSession(state)==encodeSession(dest),"cycle/rest roundtrip");
        for(int schema=1;schema<=6;++schema){auto old=juce::JSON::parse(encodeSession(state));old.getDynamicObject()->setProperty("schema",schema);
            if(schema<5)old.getDynamicObject()->removeProperty("melodyVoice");else if(schema==5)old.getDynamicObject()->setProperty("melodyVoice",1);
            expect(decodeSession(juce::JSON::toString(old),dest).wasOk()&&!dest.cycles.composition&&!dest.cycles.motif&&!dest.phrase.chords[0].rest,"legacy cycles/rests default off");}
        for(const auto* key:{"compositionCycle","motifCycle","compositionCycleBars","motifCycleBars"}){auto bad=juce::JSON::parse(encodeSession(state));bad.getDynamicObject()->removeProperty(key);
            expect(decodeSession(juce::JSON::toString(bad),dest).failed(),"missing cycle field accepted");}
        for(juce::var value:{juce::var(0),juce::var(3),juce::var(128),juce::var(1.5),juce::var(true),juce::var("8"),juce::var(juce::int64(4294967296LL))}){
            auto bad=juce::JSON::parse(encodeSession(state));bad.getDynamicObject()->setProperty("motifCycleBars",value);expect(decodeSession(juce::JSON::toString(bad),dest).failed(),"invalid cycle interval accepted");}
        auto bad=juce::JSON::parse(encodeSession(state));bad["chords"].getArray()->getReference(0).getDynamicObject()->removeProperty("rest");
        expect(decodeSession(juce::JSON::toString(bad),dest).failed(),"missing rest state accepted");
    }
    for(int version=1;version<=5;++version){auto legacy=juce::JSON::parse(encodeSession(makePreset(8)));legacy.getDynamicObject()->setProperty("schema",version);
        legacy.getDynamicObject()->removeProperty("composition");legacy.getDynamicObject()->removeProperty("surface");
        for(auto& chord:*legacy["chords"].getArray())chord.getDynamicObject()->removeProperty("color");
        expect(decodeSession(juce::JSON::toString(legacy),dest).wasOk()&&dest.music.composition==Composition::song&&dest.detail.surface==0,"legacy composition migration failed");}
    for(const auto* key:{"composition","surface"}){auto bad=juce::JSON::parse(encodeSession(makePreset(11)));bad.getDynamicObject()->removeProperty(key);
        expect(decodeSession(juce::JSON::toString(bad),dest).failed(),"missing composition field accepted");
        bad.getDynamicObject()->setProperty(key,-1);expect(decodeSession(juce::JSON::toString(bad),dest).failed(),"negative composition field accepted");
        bad.getDynamicObject()->setProperty(key,99);expect(decodeSession(juce::JSON::toString(bad),dest).failed(),"invalid composition range accepted");}
    {auto bad=juce::JSON::parse(encodeSession(makePreset(10)));bad["chords"].getArray()->getReference(0).getDynamicObject()->removeProperty("color");
        expect(decodeSession(juce::JSON::toString(bad),dest).failed(),"missing field voicing color accepted");}
    for(int voice=0;voice<6;++voice){auto s=makePreset(8);s.detail.voice=MelodyVoice(voice);
        expect(decodeSession(encodeSession(s),dest).wasOk()&&encodeSession(s)==encodeSession(dest),"melody voice roundtrip mismatch");}
    for(int version=1;version<=4;++version){auto old=juce::JSON::parse(encodeSession(makePreset(8)));old.getDynamicObject()->setProperty("schema",version);
        old.getDynamicObject()->removeProperty("melodyVoice");expect(decodeSession(juce::JSON::toString(old),dest).wasOk()&&dest.detail.voice==MelodyVoice::original,"legacy voice migration failed");}
    for(juce::var invalid:{juce::var(),juce::var(-1),juce::var(6),juce::var(1.5),juce::var("soft"),juce::var(true),juce::var(juce::int64(4294967296LL))}){
        auto bad=juce::JSON::parse(encodeSession(makePreset(8)));bad.getDynamicObject()->setProperty("melodyVoice",invalid);
        expect(decodeSession(juce::JSON::toString(bad),dest).failed(),"invalid melody voice accepted");}
    for(int version:{1,2,3}){auto old=juce::JSON::parse(encodeSession(makePreset(8)));old.getDynamicObject()->setProperty("schema",version);
        for(const auto* k:{"percussionEnabled","percussionLevel","percussionDensity","percussionGlitch","percussionColor","percussionSeed"})old.getDynamicObject()->removeProperty(k);
        expect(decodeSession(juce::JSON::toString(old),dest).wasOk()&&!dest.percussion.enabled,"legacy import adds unwanted percussion");}
    auto percussionState=makePreset(9);percussionState.percussion={true,.7f,.8f,.9f,.1f,4294967295u};
    expect(decodeSession(encodeSession(percussionState),dest).wasOk()&&encodeSession(dest)==encodeSession(percussionState),"percussion settings/seed roundtrip mismatch");
    for(const auto* k:{"percussionEnabled","percussionLevel","percussionDensity","percussionGlitch","percussionColor","percussionSeed"}){
        auto bad=juce::JSON::parse(encodeSession(percussionState));bad.getDynamicObject()->removeProperty(k);
        expect(decodeSession(juce::JSON::toString(bad),dest).failed(),"missing percussion field accepted");}
    for(const auto* k:{"percussionLevel","percussionDensity","percussionGlitch","percussionColor","percussionSeed"}){
        auto bad=juce::JSON::parse(encodeSession(percussionState));bad.getDynamicObject()->setProperty(k,-1);
        expect(decodeSession(juce::JSON::toString(bad),dest).failed(),"invalid percussion field accepted");}
    for(int oldVersion:{1,2}){
        auto old=juce::JSON::parse(encodeSession(original));old.getDynamicObject()->setProperty("schema",oldVersion);
        for(const auto* k:{"bed","broken","motif","wear","echo","breathe","motifSeed"})old.getDynamicObject()->removeProperty(k);
        expect(decodeSession(juce::JSON::toString(old),dest).wasOk(),"old arrangement-free session failed");
        expect(dest.detail.bed==1&&dest.detail.motif==0&&dest.detail.broken==0&&dest.detail.wear==0&&dest.detail.echo==0,"legacy session gained unwanted layers");
    }
    for(const auto* k:{"bed","broken","motif","wear","echo","breathe","motifSeed"}){
        auto invalid=juce::JSON::parse(encodeSession(makePreset(8)));invalid.getDynamicObject()->removeProperty(k);
        expect(decodeSession(juce::JSON::toString(invalid),dest).failed(),"missing arrangement state accepted");
    }
    for(const auto* k:{"bed","broken","motif","wear","echo","motifSeed"}){
        auto invalid=juce::JSON::parse(encodeSession(makePreset(8)));invalid.getDynamicObject()->setProperty(k,-1);
        expect(decodeSession(juce::JSON::toString(invalid),dest).failed(),"out of range arrangement state accepted");
    }
    // Schema 1 imports retain their realized harmony and original oscillator/filter path.
    auto legacy=juce::JSON::parse(encodeSession(original));legacy.getDynamicObject()->setProperty("schema",1);
    for(const auto* k:{"evolveTarget","evolveBars","model","tone","tonePhase","texture","resonance","decay","sustain","filterEnv"})legacy.getDynamicObject()->removeProperty(k);
    expect(decodeSession(juce::JSON::toString(legacy),dest).wasOk(),"legacy schema import failed");
    expect(dest.patch.model==ToneModel::warm&&dest.music.evolveTarget==1&&dest.music.evolveBars==original.music.slots*original.music.bars,"legacy semantics not migrated");
    for(int i=0;i<4;++i)expect(sameChord(original.phrase.chords[i],dest.phrase.chords[i]),"legacy chord changed");
    for(int tone=0;tone<9;++tone){auto state=makePreset(2);state.patch=makeTone(tone);state.tonePhase=.372;
        state.music.evolveBars=8;state.music.evolveTarget=2;
        expect(decodeSession(encodeSession(state),dest).wasOk()&&encodeSession(state)==encodeSession(dest),"tone/evolution phase roundtrip failed");}
    for(const auto* key:{"evolveBars","tonePhase","model","texture","decay"}){
        auto incomplete=juce::JSON::parse(encodeSession(original));incomplete.getDynamicObject()->removeProperty(key);
        expect(decodeSession(juce::JSON::toString(incomplete),dest).failed(),"incomplete schema 2 accepted");
    }
    for(const auto* key:{"evolveBars","evolveTarget","tonePhase","model","texture","resonance","decay","sustain","filterEnv"}){
        auto invalid=juce::JSON::parse(encodeSession(original));invalid.getDynamicObject()->setProperty(key,999);
        expect(decodeSession(juce::JSON::toString(invalid),dest).failed(),"invalid schema 2 field accepted");
    }
    dest=original;
    for(const auto* bad:{"{}","null","[]","{\"schema\":99}","invalid"}){
        expect(decodeSession(bad,dest).failed(),"invalid session accepted");expect(encodeSession(dest)==encodeSession(original),"failed load changed state");}
    auto object=juce::JSON::parse(encodeSession(original));object.getDynamicObject()->setProperty("floor",127);
    expect(decodeSession(juce::JSON::toString(object),dest).failed(),"out of range accepted");
    object=juce::JSON::parse(encodeSession(original));object.getDynamicObject()->setProperty("key","oops");
    expect(decodeSession(juce::JSON::toString(object),dest).failed(),"wrong type accepted");
    object=juce::JSON::parse(encodeSession(original));object.getDynamicObject()->setProperty("key",1.5);
    expect(decodeSession(juce::JSON::toString(object),dest).failed(),"fractional key accepted");
    object=juce::JSON::parse(encodeSession(original));object.getDynamicObject()->setProperty("key",juce::int64(4294967298LL));
    expect(decodeSession(juce::JSON::toString(object),dest).failed(),"overflowing integer accepted");
    auto file=juce::File::getCurrentWorkingDirectory().getChildFile("test-session.undertone");
    expect(saveSession(file,original).wasOk(),"file save failed");expect(saveSession(file,makePreset(3)).wasOk(),"second save failed");
    Session backup;expect(loadSession(file.getSiblingFile(file.getFileName()+".bak"),backup).wasOk(),"backup missing");
    expect(encodeSession(backup)==encodeSession(original),"backup not original");
    expect(loadSession(file,dest).wasOk()&&dest.preset==3,"saved file load failed");
    file.deleteFile();file.getSiblingFile(file.getFileName()+".bak").deleteFile();
    std::cout<<"PASS session roundtrip, evolution replay, validation, backup and file I/O\n";
}
