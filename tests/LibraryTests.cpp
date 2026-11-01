#include "app/SessionLibrary.h"
#include <iostream>
#include <cstdlib>
using namespace undertone;
void expect(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
    auto state=makePreset(10);SessionDocument document;document.establish(state,"Quiet room");
    expect(!document.edited(state),"fresh preset marked edited");
    auto phase=state;phase.tonePhase=.4;expect(!document.edited(phase),"live tone phase marks session edited");
    phase.bpm+=.01;expect(document.edited(phase)&&!document.edited(phase,true),"external clock jitter handling");
    auto sound=state;sound.patch.attack+=.1f;expect(document.edited(sound),"sound edit missed");
    auto phrase=state;phrase.phrase.chords[0].rest=true;expect(document.edited(phrase),"rest edit missed");
    auto scene=state;storeScene(scene,0);expect(document.edited(scene),"scene save missed");
    auto motif=state;advanceMotif(motif.detail,true);expect(document.edited(motif),"motif evolution missed");
    document.establish(sound,"Variation");expect(!document.edited(sound)&&document.edited(state),"saved baseline not updated");
    const auto root=juce::File::getCurrentWorkingDirectory().getChildFile("library-test-"+juce::Uuid().toString());
    const auto data=root.getChildFile("data"),moments=root.getChildFile("Moments"),other=root.getChildFile("other");
    other.createDirectory();SessionLibrary library(data,moments);
    expect(library.scan(LibraryFilter::factory,"").size()==presetCount,"factory catalogue incomplete");
    expect(library.scan(LibraryFilter::factory,"QUIET").size()==1,"case-insensitive preset search");
    const auto a=library.sessionDirectory().getChildFile("One.undertone"),b=other.getChildFile("One.undertone"),m=moments.getChildFile("Kept.undertone");
    expect(saveSession(a,state).wasOk()&&saveSession(b,sound).wasOk()&&saveSession(m,scene).wasOk(),"fixture saves");
    expect(library.addFolder(other).wasOk()&&library.addFolder(other).wasOk(),"add folder failed");
    expect(library.scan(LibraryFilter::sessions,"").size()==2,"same names in different folders lost or duplicate folder added");
    expect(library.scan(LibraryFilter::moments,"").size()==1,"moments missing");
    expect(library.remember(a).wasOk()&&library.remember(b).wasOk()&&library.remember(a).wasOk(),"recent persistence failed");
    SessionLibrary restored(data,moments);auto recent=restored.scan(LibraryFilter::recent,"");
    expect(recent.size()==2&&recent[0].file==a&&recent[1].file==b,"recent order/dedup not persisted");
    expect(restored.scan(LibraryFilter::all,"").size()==presetCount+3,"All duplicates a saved/recent file");
    b.deleteFile();expect(restored.scan(LibraryFilter::recent,"").size()==2,"missing recent should remain discoverable");
    Session destination=state;expect(loadSession(b,destination).failed()&&encodeSession(destination)==encodeSession(state),"missing load modifies destination");
    a.replaceWithText("broken JSON");expect(loadSession(a,destination).failed()&&encodeSession(destination)==encodeSession(state),"invalid load modifies destination");
    expect(restored.scan(LibraryFilter::sessions,"One").size()==1,"invalid file hidden from catalogue");
    for(int i=0;i<25;++i)expect(restored.remember(other.getChildFile(juce::String(i)+".undertone")).wasOk(),"recent write failed");
    expect(SessionLibrary(data,moments).scan(LibraryFilter::recent,"").size()==20,"recent list not bounded");
    data.getChildFile("library.json").replaceWithText("broken JSON");
    expect(SessionLibrary(data,moments).scan(LibraryFilter::factory,"").size()==presetCount,"corrupt preferences break factory browsing");
    root.deleteRecursively();std::cout<<"Library catalogue, persistence, invalid files and edit tracking passed\n";
}
