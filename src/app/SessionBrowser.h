#pragma once
#include "SessionLibrary.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace undertone {
class SessionBrowser final : public juce::Component,private juce::ListBoxModel {
public:
    explicit SessionBrowser(SessionLibrary&);
    ~SessionBrowser() override {list.setModel(nullptr);}
    void refresh();
    void paint(juce::Graphics&) override;
    void resized() override;
    std::function<void(const LibraryEntry&)> onLoad;
    std::function<void()> onSaveAs,onOpen,onAddFolder;
private:
    SessionLibrary& library;
    std::vector<LibraryEntry> catalogue,entries;
    void updateResults();
    juce::Label heading,description,searchLabel,filterLabel,count,title,kind,details,path,hint;
    juce::TextEditor search;
    juce::ComboBox filter;
    juce::ListBox list{"Presets and sessions",this};
    juce::TextButton load{"Load selected"},saveAs{"Save variation..."},open{"Open file..."},addFolder{"Add folder..."},refreshButton{"Refresh"};
    int getNumRows() override {return int(entries.size());}
    juce::String getNameForRow(int) override;
    void paintListBoxItem(int,juce::Graphics&,int,int,bool) override;
    void selectedRowsChanged(int) override;
    void returnKeyPressed(int) override;
};
}
