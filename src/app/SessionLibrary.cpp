#include "SessionLibrary.h"
#include <algorithm>

namespace undertone {
juce::String sessionFingerprint(Session s,bool ignoreTempo){
    // The live LFO phase and external clock jitter are not edits. Stored scene settings are.
    s.tonePhase=0;if(ignoreTempo)s.bpm=88;
    return encodeSession(s);
}
void SessionDocument::establish(const Session& state,const juce::String& title,const juce::File& source){baseline=state;name=title;file=source;}
bool SessionDocument::edited(const Session& current,bool externalClock) const {return sessionFingerprint(current,externalClock)!=sessionFingerprint(baseline,externalClock);}

SessionLibrary::SessionLibrary(juce::File data,juce::File momentDirectory)
    :settings(data.getChildFile("library.json")),sessions(data.getChildFile("Sessions")),moments(momentDirectory){
    if(settings.getSize()>128*1024)return;
    const auto json=juce::JSON::parse(settings);
    auto read=[&](const char* key,std::vector<juce::File>& output,int limit){
        if(auto* values=json[key].getArray())for(const auto& value:*values){
            if(!value.isString()||!juce::File::isAbsolutePath(value.toString()))continue;
            juce::File file(value.toString());
            if(std::find(output.begin(),output.end(),file)==output.end())output.push_back(file);
            if(int(output.size())>=limit)break;
        }
    };
    read("folders",folders,12);read("recent",recent,20);
}
juce::Result SessionLibrary::persist() const {
    auto* object=new juce::DynamicObject();juce::var json(object);
    for(const auto& pair:{std::make_pair("folders",&folders),std::make_pair("recent",&recent)}){
        juce::Array<juce::var> values;for(const auto& file:*pair.second)values.add(file.getFullPathName());object->setProperty(pair.first,values);
    }
    if(settings.getParentDirectory().createDirectory().failed())return juce::Result::fail("Cannot create library settings directory.");
    juce::TemporaryFile temporary(settings);
    if(!temporary.getFile().replaceWithText(juce::JSON::toString(json))||!temporary.overwriteTargetFileWithTemporary())return juce::Result::fail("Session is available, but library history could not be saved.");
    return juce::Result::ok();
}
juce::Result SessionLibrary::remember(const juce::File& file){
    recent.erase(std::remove(recent.begin(),recent.end(),file),recent.end());recent.insert(recent.begin(),file);
    if(recent.size()>20)recent.resize(20);return persist();
}
juce::Result SessionLibrary::addFolder(const juce::File& folder){
    if(!folder.isDirectory())return juce::Result::fail("Choose an existing folder.");
    if(folder==sessions||folder==moments||std::find(folders.begin(),folders.end(),folder)!=folders.end())return juce::Result::ok();
    if(folders.size()>=12)return juce::Result::fail("The library already has 12 added folders. Open individual files to add them to Recent.");
    folders.push_back(folder);return persist();
}
std::vector<LibraryEntry> SessionLibrary::scan(LibraryFilter filter,const juce::String& query) const {
    std::vector<LibraryEntry> result;
    auto add=[&](LibraryEntry entry){
        if(!(entry.name+" "+entry.category+" "+entry.file.getFullPathName()).containsIgnoreCase(query.trim()))return;
        if(std::none_of(result.begin(),result.end(),[&](const auto& existing){return existing.identity()==entry.identity();}))result.push_back(std::move(entry));
    };
    if(filter==LibraryFilter::all||filter==LibraryFilter::factory)for(int i=0;i<presetCount;++i)add({presetName(i),"Factory",{},i});
    auto scanFolder=[&](const juce::File& folder,const juce::String& category){
        auto files=folder.findChildFiles(juce::File::findFiles,false,"*.undertone");
        std::sort(files.begin(),files.end(),[](const auto& a,const auto& b){const auto ta=a.getLastModificationTime(),tb=b.getLastModificationTime();return ta==tb?a.getFullPathName()<b.getFullPathName():ta>tb;});
        for(const auto& file:files)add({file.getFileNameWithoutExtension(),category,file});
    };
    if(filter==LibraryFilter::all||filter==LibraryFilter::sessions){scanFolder(sessions,"Session");for(const auto& folder:folders)scanFolder(folder,"Session");}
    if(filter==LibraryFilter::all||filter==LibraryFilter::moments)scanFolder(moments,"Moment");
    if(filter==LibraryFilter::all||filter==LibraryFilter::recent)for(const auto& file:recent)add({file.getFileNameWithoutExtension(),file.existsAsFile()?"Recent":"Missing file",file});
    return result;
}
}
