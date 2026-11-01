#pragma once
#include "SessionFile.h"
#include <vector>

namespace undertone {
enum class LibraryFilter { all, factory, sessions, moments, recent };
struct LibraryEntry {
    juce::String name, category;
    juce::File file;
    int preset=-1;
    juce::String identity() const {return preset>=0?"preset:"+juce::String(preset):file.getFullPathName();}
};
// UI-owned file catalogue. Never accessed from the audio callback.
class SessionLibrary {
public:
    SessionLibrary(juce::File dataDirectory,juce::File momentsDirectory);
    std::vector<LibraryEntry> scan(LibraryFilter,const juce::String& query) const;
    juce::Result remember(const juce::File&);
    juce::Result addFolder(const juce::File&);
    const juce::File& sessionDirectory() const {return sessions;}
private:
    juce::File settings,sessions,moments;
    std::vector<juce::File> folders,recent;
    juce::Result persist() const;
};

class SessionDocument {
public:
    void establish(const Session& state,const juce::String& title,const juce::File& source={});
    bool edited(const Session& current,bool externalClock=false) const;
    juce::String name;
    juce::File file;
private:
    Session baseline;
};
juce::String sessionFingerprint(Session,bool ignoreTempo=false);
}
