#include "SessionBrowser.h"
#include "Design.h"

namespace undertone {
using namespace design;
namespace { constexpr int resultsTop=129,resultsInset=16; }
SessionBrowser::SessionBrowser(SessionLibrary& source):library(source){
    for(auto* label:{&heading,&description,&searchLabel,&filterLabel,&count,&title,&kind,&details,&path,&hint}){addAndMakeVisible(label);label->setFont(font(12));label->setColour(juce::Label::textColourId,muted);}
    heading.setText("Your library",juce::dontSendNotification);heading.setFont(font(25,true));heading.setColour(juce::Label::textColourId,ink);
    description.setText("Find a starting point, or return to a session you kept.",juce::dontSendNotification);
    searchLabel.setText("SEARCH",juce::dontSendNotification);filterLabel.setText("SHOW",juce::dontSendNotification);
    search.setTextToShowWhenEmpty("Search by name or style",muted);search.setTitle("Search library");search.onTextChange=[this]{updateResults();};addAndMakeVisible(search);
    filter.addItemList({"All items","Factory presets","Saved sessions","Kept moments","Recent files"},1);filter.setSelectedId(1,juce::dontSendNotification);filter.setTitle("Library filter");filter.onChange=[this]{refresh();};addAndMakeVisible(filter);
    addAndMakeVisible(list);list.setRowHeight(52);list.setColour(juce::ListBox::backgroundColourId,background);list.setOutlineThickness(0);
    title.setFont(font(23,true));title.setColour(juce::Label::textColourId,ink);details.setFont(font(14));details.setJustificationType(juce::Justification::topLeft);
    path.setJustificationType(juce::Justification::topLeft);path.setMinimumHorizontalScale(1);path.setFont(font(11));
    hint.setText("Selecting only shows details. Load stops playback; press Play when ready.",juce::dontSendNotification);
    for(auto* b:{&load,&saveAs,&open,&addFolder,&refreshButton})addAndMakeVisible(b);
    load.getProperties().set("primary",true);load.setColour(juce::TextButton::buttonColourId,accent);
    load.onClick=[this]{returnKeyPressed(list.getSelectedRow());};saveAs.onClick=[this]{if(onSaveAs)onSaveAs();};open.onClick=[this]{if(onOpen)onOpen();};
    addFolder.onClick=[this]{if(onAddFolder)onAddFolder();};refreshButton.onClick=[this]{refresh();};
    refresh();
}
void SessionBrowser::refresh(){
    catalogue=library.scan(LibraryFilter(std::max(0,filter.getSelectedId()-1)),{});updateResults();
}
void SessionBrowser::updateResults(){
    const int selected=list.getSelectedRow();const auto identity=selected>=0&&selected<int(entries.size())?entries[selected].identity():juce::String();
    entries.clear();for(const auto& entry:catalogue)if((entry.name+" "+entry.category+" "+entry.file.getFullPathName()).containsIgnoreCase(search.getText().trim()))entries.push_back(entry);
    list.updateContent();
    int row=entries.empty()?-1:0;for(int i=0;i<int(entries.size());++i)if(entries[i].identity()==identity)row=i;
    if(row<0)list.deselectAllRows();else list.selectRow(row);
    selectedRowsChanged(row);count.setText(juce::String(int(entries.size()))+(entries.size()==1?" item":" items"),juce::dontSendNotification);repaint();
}
juce::String SessionBrowser::getNameForRow(int row){return row>=0&&row<int(entries.size())?entries[row].name+", "+entries[row].category:juce::String();}
void SessionBrowser::paintListBoxItem(int row,juce::Graphics& g,int w,int h,bool selected){
    if(row<0||row>=int(entries.size()))return;
    if(selected){g.setColour(raised);g.fillRoundedRectangle(2,2,float(w-4),float(h-4),5);g.setColour(accent);g.fillRoundedRectangle(3,10,3,float(h-20),1);}
    g.setColour(ink);g.setFont(font(14,selected));g.drawText(entries[row].name,16,7,w-30,21,juce::Justification::centredLeft,true);
    g.setColour(muted);g.setFont(font(11));g.drawText(entries[row].category,16,29,w-30,16,juce::Justification::centredLeft);
}
void SessionBrowser::selectedRowsChanged(int row){
    load.setEnabled(false);
    if(row<0||row>=int(entries.size())){title.setText("No matching items",juce::dontSendNotification);kind.setText("",juce::dontSendNotification);details.setText("Try another search or filter. Open a session file or add its folder to include it here.",juce::dontSendNotification);path.setText("",juce::dontSendNotification);return;}
    const auto& entry=entries[row];title.setText(entry.name,juce::dontSendNotification);kind.setText(entry.category.toUpperCase(),juce::dontSendNotification);
    Session state;const auto result=entry.preset>=0?(state=makePreset(entry.preset),juce::Result::ok()):loadSession(entry.file,state);
    const auto location=entry.preset>=0?"Factory starting point. Your A/B scene bank is retained when loading.":entry.file.getFullPathName();
    path.setText(location,juce::dontSendNotification);path.setTooltip(location);
    if(result.failed()){details.setText(result.getErrorMessage()+"\n\nYour current session has not changed.",juce::dontSendNotification);return;}
    const char* keys[]{"C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B"};const char* modes[]{"Major","Natural minor","Dorian","Mixolydian"};
    const auto text=juce::String(keys[state.music.key])+" "+modes[int(state.music.mode)]+"  /  "+juce::String(state.bpm,0)+" BPM\n\n"+
        juce::String(state.music.slots)+" chords  /  "+juce::String(state.music.bars)+" bars per chord\n"+compositionName(int(state.music.composition))+"\n\n"+
        modelName(int(state.patch.model))+"\nMelody: "+melodyVoiceName(int(state.detail.voice))+"\nPercussion: "+(state.percussion.enabled?"on":"off");
    details.setText(text,juce::dontSendNotification);load.setEnabled(true);
}
void SessionBrowser::returnKeyPressed(int row){if(load.isEnabled()&&row>=0&&row<int(entries.size())&&onLoad)onLoad(entries[row]);}
void SessionBrowser::paint(juce::Graphics& g){
    g.fillAll(background);g.setColour(panel);g.fillRoundedRectangle(0,float(resultsTop),float(getWidth()),float(getHeight()-56-resultsTop),7);
    g.setColour(line);g.drawVerticalLine(getWidth()/2,float(resultsTop+resultsInset),float(getHeight()-72));
}
void SessionBrowser::resized(){
    const int w=getWidth(),h=getHeight(),right=w/2+22,rw=w-right-18;
    heading.setBounds(8,0,400,34);description.setBounds(10,36,w-20,20);
    searchLabel.setBounds(8,66,100,18);search.setBounds(8,87,w/2-28,30);
    filterLabel.setBounds(right,66,100,18);filter.setBounds(right,87,210,30);refreshButton.setBounds(w-114,87,106,30);
    list.setBounds(8,resultsTop+resultsInset,w/2-24,h-88-resultsTop-resultsInset);count.setBounds(10,h-83,w/2-24,20);
    title.setBounds(right,135,rw,55);kind.setBounds(right,191,rw,20);details.setBounds(right,225,rw,220);path.setBounds(right,450,rw,std::max(40,h-550));
    load.setBounds(right,h-100,150,34);open.setBounds(right+162,h-100,110,34);
    saveAs.setBounds(8,h-40,154,34);addFolder.setBounds(174,h-40,122,34);hint.setBounds(314,h-42,w-320,38);
}
}
