#include "MainComponent.h"
#include "MidiExport.h"
#include "Design.h"
#include <cmath>

namespace undertone {
using namespace design;
namespace {
juce::File captureDirectory(){
    const auto isolated=juce::SystemStats::getEnvironmentVariable("UNDERTONE_DATA_DIR",{});
    return isolated.isEmpty()?juce::File::getSpecialLocation(juce::File::userDocumentsDirectory):juce::File(isolated);
}
}
MainComponent::Theme::Theme() {
    setColour(juce::ResizableWindow::backgroundColourId,background);
    setColour(juce::TextButton::buttonColourId,raised);
    setColour(juce::TextButton::buttonOnColourId,accent);
    setColour(juce::TextButton::textColourOffId,ink);setColour(juce::TextButton::textColourOnId,ink);
    setColour(juce::ComboBox::backgroundColourId,panel);setColour(juce::ComboBox::textColourId,ink);
    setColour(juce::ComboBox::outlineColourId,line);
    setColour(juce::PopupMenu::backgroundColourId,panel);setColour(juce::PopupMenu::textColourId,ink);
    setColour(juce::Slider::rotarySliderFillColourId,accent);setColour(juce::Slider::rotarySliderOutlineColourId,line);
    setColour(juce::Slider::thumbColourId,accent);setColour(juce::Slider::trackColourId,accent);
    setColour(juce::Slider::textBoxTextColourId,ink);setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId,muted);setColour(juce::ToggleButton::textColourId,ink);
    setColour(juce::ToggleButton::tickColourId,accent);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,raised);
    setColour(juce::PopupMenu::highlightedTextColourId,accent);
    setColour(juce::TooltipWindow::backgroundColourId,raised);
    setColour(juce::TooltipWindow::textColourId,ink);
    setColour(juce::TooltipWindow::outlineColourId,line);
    setColour(juce::TextEditor::highlightColourId,accent.withAlpha(.3f));
    setColour(juce::ScrollBar::thumbColourId,line.brighter(.2f));
}
void MainComponent::ChordButton::paintButton(juce::Graphics& g,bool over,bool down){
    auto bounds=getLocalBounds().toFloat().reduced(.5f);
    g.setColour(active?juce::Colour(0xff34302a):over||down?raised.brighter(.07f):raised);
    g.fillRoundedRectangle(bounds,5);
    g.setColour(hasKeyboardFocus(false)?ink:active?accent:locked?mint:line);
    g.drawRoundedRectangle(bounds,5,active||hasKeyboardFocus(false)?1.5f:1.f);
    g.setFont(font(10,true));g.setColour(active?accent:muted);
    g.drawText(juce::String(number).paddedLeft('0',2),10,6,28,14,juce::Justification::centredLeft);
    if(locked){g.setColour(mint);g.drawRoundedRectangle(float(getWidth()-22),9,8,7,1,1);g.drawRoundedRectangle(float(getWidth()-20),5,4,7,2,1);}
    if(resting){g.setFont(font(10));g.drawText("REST",38,6,getWidth()-48,14,juce::Justification::centredRight);}
    g.setFont(font(getWidth()<150?18.f:24.f,true));g.setColour(resting?muted:ink);
    g.drawFittedText(name,10,21,getWidth()-20,27,juce::Justification::centredLeft,1);
    g.setFont(font(10));g.setColour(preview.isNotEmpty()?accent:muted);
    g.drawFittedText(preview.isNotEmpty()?preview:voicing,10,getHeight()<72?45:51,getWidth()-20,13,juce::Justification::centredLeft,1);
    if(active){g.setColour(accent);g.fillRoundedRectangle(7.f,float(getHeight()-3),float(getWidth()-14)*progress,2,1);}
}
MainComponent::MainComponent() {
    setLookAndFeel(&theme);setWantsKeyboardFocus(true);setSize(1120,1502);
    addAndMakeVisible(transportLabel);addAndMakeVisible(messageLabel);addAndMakeVisible(clockHint);
    transportLabel.setFont(font(11,true));transportLabel.setJustificationType(juce::Justification::centredRight);
    messageLabel.setFont(font(11));messageLabel.setName("Status");
    clockHint.setFont(font(11));clockHint.setName("MIDI timing");
    clockHint.setTooltip("The MIDI buffer adds a fixed delay, separate from the audio device latency. Reduce the audio buffer for a shorter delay. Late counts events arriving more than one sample behind their render interval.");
    for(auto* b:{&play,&stop,&panic,&generateButton,&lockAll,&undo,&save,&load,&audio,&refreshMidi,&recover})addAndMakeVisible(b);
    for(auto* button:{&performanceTab,&soundTab,&varyMotif,&newFigure,&storeA,&storeB,&recallA,&recallB,&keepMoment,&recordAudio,&exportMidi})addAndMakeVisible(button);
    for(auto* control:{&phrasing,&sceneCycle,&sceneVariations})addAndMakeVisible(control);
    for(auto* box:{&motifMode,&swapStyle,&sceneInterval,&exportBars})addAndMakeVisible(box);
    motifMode.addItemList({"Hold motif","Vary motif","New motifs"},1);motifMode.setTitle("Automatic motif behavior");
    swapStyle.addItemList({"Nearby swap","More tension","Surprise swap"},1);swapStyle.setTitle("Chord swap approach");
    for(int bars=1;bars<=64;bars*=2)sceneInterval.addItem(juce::String(bars)+" bars",bars);
    sceneInterval.setTitle("Scene interval");exportBars.setTitle("MIDI export length");
    for(int bars:{4,8,16,32,64,128})exportBars.addItem(juce::String(bars)+" bars",bars);exportBars.setSelectedId(16);
    for(auto* label:{&sceneHeading,&motifModeLabel,&roomLabel,&performanceLevelLabel,&captureHint}){addAndMakeVisible(label);label->setFont(font(11,true));}
    motifModeLabel.setText("MELODY & MOTION",juce::dontSendNotification);roomLabel.setText("Give me space",juce::dontSendNotification);performanceLevelLabel.setText("Master level",juce::dontSendNotification);
    for(auto* slider:{&room,&performanceLevel}){addAndMakeVisible(slider);slider->setRange(0,1,.01);slider->setSliderStyle(juce::Slider::LinearHorizontal);slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,48,24);}
    room.setTitle("Give me space");performanceLevel.setTitle("Master level");
    room.setTooltip("Thin the melodic and broken-chord activity and lengthen breaths while retaining the base mix.");
    room.onValueChange=[this]{if(!refreshing){editing.detail.room=float(room.getValue());send(Action::detail);}};
    performanceLevel.onValueChange=[this]{if(!refreshing){editing.patch.level=float(performanceLevel.getValue());knobs[7].setValue(performanceLevel.getValue(),juce::dontSendNotification);send(Action::patch);}};
    performanceTab.onClick=[this]{setPerformanceView(true);};soundTab.onClick=[this]{setPerformanceView(false);};
    motifMode.onChange=[this]{if(!refreshing){editing.detail.motifMode=MotifMode(motifMode.getSelectedId()-1);send(Action::detail);}};
    phrasing.onClick=[this]{editing.detail.phrasing=phrasing.getToggleState();send(Action::detail);};
    varyMotif.onClick=[this]{send(Action::varyMotif);notification="Developed the motif ending; opening identity and figure retained.";};
    newFigure.onClick=[this]{send(Action::newFigure);notification="New broken-chord figure; melody retained.";};
    swapStyle.onChange=[this]{if(!refreshing){editing.swapStyle=SwapStyle(swapStyle.getSelectedId()-1);send(Action::swapStyle);}};
    storeA.onClick=[this]{send(Action::storeScene,0);};storeB.onClick=[this]{send(Action::storeScene,1);};
    recallA.onClick=[this]{send(Action::recallScene,0);};recallB.onClick=[this]{send(Action::recallScene,1);};
    sceneCycle.onClick=[this]{changeScenes();};sceneVariations.onClick=[this]{changeScenes();};sceneInterval.onChange=[this]{if(!refreshing)changeScenes();};
    storeA.setTooltip("Store the currently sounding state in A. Replaces the previous A snapshot.");storeB.setTooltip("Store the currently sounding state in B. Replaces the previous B snapshot.");
    recallA.setTooltip("Recall A at the next bar. Scene snapshots include their own locks and sound settings.");recallB.setTooltip("Recall B at the next bar. Scene snapshots include their own locks and sound settings.");
    keepMoment.onClick=[this]{keepCurrentMoment();};recordAudio.onClick=[this]{toggleRecording();};exportMidi.onClick=[this]{chooseMidiExport();};
    play.getProperties().set("primary",true);play.getProperties().set("icon","play");
    stop.getProperties().set("icon","stop");recordAudio.getProperties().set("icon","record");
    performanceTab.getProperties().set("tab",true);soundTab.getProperties().set("tab",true);
    play.setButtonText("Play / restart");panic.setColour(juce::TextButton::textColourOffId,danger);
    keepMoment.setColour(juce::TextButton::textColourOffId,mint);
    recordAudio.setTooltip("Record Undertone's stereo output. Stop recording finishes the WAV; transport Stop preserves its tails.");
    exportMidi.setTooltip("Export a new MIDI performance from the current state, using the selected number of bars.");
    recordingThread.startThread();
    for(auto* b:{&evolve,&rootless,&wide})addAndMakeVisible(b);
    for(auto* box:{&key,&mode,&character,&slots,&bars,&rhythm,&clockSource})addAndMakeVisible(box);
    for(auto* box:{&toneModel,&tonePreset,&evolveTarget,&evolvePace})addAndMakeVisible(box);
    for(int i=0;i<4;++i)toneModel.addItem(modelName(i),i+1);
    tonePreset.addItem("Custom / current sound",100);
    for(int i=0;i<9;++i)tonePreset.addItem(toneName(i),i+1);
    evolveTarget.addItemList({"Chords + tone","Chords only","Tone only"},1);
    for(int i=0;i<6;++i)evolvePace.addItem(juce::String(1<<i)+((i==0)?" bar":" bars"),i+1);
    const char* soundCaptions[]{"SYNTH ENGINE","TONE PATCH / KEEPS YOUR CHORDS","EVOLVE TARGET","EVOLVE INTERVAL / TONE CYCLE"};
    for(int i=0;i<4;++i){soundLabels[i].setText(soundCaptions[i],juce::dontSendNotification);soundLabels[i].setFont(font(11,true));addAndMakeVisible(soundLabels[i]);}
    toneModel.setTitle("Synth engine");tonePreset.setTitle("Tone patch");evolveTarget.setTitle("Evolve target");evolvePace.setTitle("Evolve interval");
    addAndMakeVisible(evolutionHint);evolutionHint.setFont(font(11));evolutionHint.setName("Evolution status");
    evolve.setTooltip("Evolve changes unlocked chords on a bar interval and/or slowly morphs tone. Configure target and interval below. Variation controls depth; zero disables change.");
    play.setColour(juce::TextButton::buttonColourId,accent);
    generateButton.setColour(juce::TextButton::textColourOffId,accent);
    const juce::StringArray keys{"C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B"};key.addItemList(keys,1);
    mode.addItemList({"Major","Natural minor","Dorian","Mixolydian"},1);
    for(int i=0;i<presetCount;++i)character.addItem(presetName(i),i+1);
    detailHeading.setText("ARRANGEMENT / MELODY, BROKEN CHORDS + WORN TEXTURE",juce::dontSendNotification);
    detailHeading.setFont(font(11,true));addAndMakeVisible(detailHeading);addAndMakeVisible(detailHint);
    detailHint.setFont(font(11));detailHint.setText("Melody phrases look ahead across your chords. Voice changes keep the notes, harmony and rhythm.",juce::dontSendNotification);
    addAndMakeVisible(melodyVoice);addAndMakeVisible(melodyVoiceLabel);
    melodyVoice.setName("Melody voice");melodyVoice.setTitle("Melody voice");
    melodyVoiceLabel.setText("MELODY VOICE",juce::dontSendNotification);melodyVoiceLabel.setFont(font(11,true));
    for(int i=0;i<6;++i)melodyVoice.addItem(melodyVoiceName(i),i+1);
    melodyVoice.setTooltip("Changes the melody timbre on new notes. Existing notes finish with their original voice. Chords and broken chords keep their sound.");
    melodyVoice.onChange=[this]{if(!refreshing)changeDetail();};
    addAndMakeVisible(composition);addAndMakeVisible(compositionLabel);addAndMakeVisible(formLabel);
    composition.setName("Composition");composition.setTitle("Composition");compositionLabel.setText("COMPOSITION",juce::dontSendNotification);compositionLabel.setFont(font(11,true));
    for(int i=0;i<3;++i)composition.addItem(compositionName(i),i+1);
    composition.setTooltip("Changes harmony generation and phrasing. Still and Uneasy use sparse gestures and, with Let layers come and go enabled, a 32-bar arc. Unlock harmony before changing modes.");
    composition.onChange=[this]{if(!refreshing)changeMusic();};formLabel.setFont(font(11,true));
    addAndMakeVisible(cycleHeading);addAndMakeVisible(cycleHint);
    cycleHeading.setText("AUTOMATION",juce::dontSendNotification);cycleHeading.setFont(font(11,true));cycleHint.setFont(font(11));
    for(auto* control:{&compositionCycle,&motifCycle}){addAndMakeVisible(control);control->onClick=[this]{changeCycles();};}
    for(auto* box:{&compositionInterval,&motifInterval}){addAndMakeVisible(box);for(int bars=1;bars<=64;bars*=2)box->addItem(juce::String(bars)+(bars==1?" bar":" bars"),bars);box->onChange=[this]{if(!refreshing)changeCycles();};}
    compositionInterval.setTitle("Composition cycle interval");motifInterval.setTitle("Motif cycle interval");
    for(auto* label:{&compositionIntervalLabel,&motifIntervalLabel}){addAndMakeVisible(label);label->setText("CHANGE EVERY",juce::dontSendNotification);label->setFont(font(11,true));}
    compositionCycle.setTooltip("Alternates Still and Uneasy on the bar grid. Keeps the chord roots, locks, rest slots and sounds. Pauses in Song mode or while a manual edit is queued.");
    motifCycle.setButtonText("Cycle motif");motifCycle.setTooltip("At each interval, follow Hold / Vary / New. Independent of composition and Evolve. Manual queued edits take priority.");
    const char* detailNames[]{"Pad bed","Broken chords","Melody","Drift / wear","Figure echo","Surface"};
    for(int i=0;i<6;++i){auto& control=detailControls[i];addAndMakeVisible(control);control.setRange(0,1,.01);
        control.setSliderStyle(juce::Slider::LinearHorizontal);control.setTextBoxStyle(juce::Slider::TextBoxRight,false,52,24);
        control.setName(detailNames[i]);control.setTitle(detailNames[i]);detailLabels[i].setText(detailNames[i],juce::dontSendNotification);addAndMakeVisible(detailLabels[i]);
        control.onValueChange=[this]{if(!refreshing)changeDetail();};}
    addAndMakeVisible(breathe);addAndMakeVisible(newMotif);
    breathe.setTooltip("Song mode alternates parts over 16 bars. Still / Uneasy develops over 32 bars: opening, statement, pressure and withdrawal. Off holds layer balance steady.");
    breathe.onClick=[this]{changeDetail();};newMotif.onClick=[this]{send(Action::newMotif);notification="New melody motif; broken-chord figure and harmony retained.";};
    addAndMakeVisible(percussionEnabled);addAndMakeVisible(newBeat);addAndMakeVisible(percussionHeading);addAndMakeVisible(percussionHint);
    percussionHeading.setText("PERCUSSION / A STEADY PULSE WITH GENERATED OFFBEATS",juce::dontSendNotification);percussionHeading.setFont(font(11,true));
    percussionHint.setFont(font(12));percussionHint.setText("Enable at the top. Density adds offbeats; Glitch adds short bursts. The main pulse stays put.",juce::dontSendNotification);
    const char* percussionNames[]{"Percussion level","Density","Glitch","Color"};
    for(int i=0;i<4;++i){auto& control=percussionControls[i];addAndMakeVisible(control);control.setRange(0,1,.01);
        control.setSliderStyle(juce::Slider::LinearHorizontal);control.setTextBoxStyle(juce::Slider::TextBoxRight,false,52,24);
        control.setName(percussionNames[i]);control.setTitle(percussionNames[i]);percussionLabels[i].setText(percussionNames[i],juce::dontSendNotification);addAndMakeVisible(percussionLabels[i]);
        control.onValueChange=[this]{if(!refreshing)changePercussion();};}
    percussionEnabled.setTooltip("Optional dry thumps, rim clicks and noise ticks. Off leaves space for your Elektron drums. Level and pattern controls are below Arrangement.");
    percussionControls[0].setRange(0,2,.01);percussionControls[0].setTooltip("Extended range: 1.0 is the previous maximum; 2.0 gives up to +6 dB before the master output limiter.");
    percussionEnabled.onClick=[this]{changePercussion();};newBeat.onClick=[this]{editing.percussion.seed=nextPercussionSeed(editing.percussion.seed);send(Action::percussion);notification="New offbeat pattern; main pulse and harmony retained.";};
    slots.addItemList({"2 chords","4 chords","8 chords"},1);bars.addItemList({"1 bar / chord","2 bars / chord","4 bars / chord"},1);
    rhythm.addItemList({"Sustain","Quarter pulse","Eighth pulse","Syncopated"},1);
    addAndMakeVisible(tempo);tempo.setRange(40,200,1);tempo.setSliderStyle(juce::Slider::LinearHorizontal);
    tempo.setTextBoxStyle(juce::Slider::TextBoxRight,false,56,28);tempo.setTextValueSuffix(" bpm");
    const char* names[]{"Complexity","Variation","Lowest note","Bass cut / Hz","Brightness","Motion","Space","Level"};
    for(size_t i=0;i<knobs.size();++i) {
        auto& k=knobs[i];addAndMakeVisible(k);k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::TextBoxBelow,false,94,24);k.setRange(0,1,.01);
        knobLabels[i].setText(names[i],juce::dontSendNotification);knobLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(knobLabels[i]);k.setName(names[i]);k.setTitle(names[i]);k.setTooltip(names[i]);
        k.onValueChange=[this,i]{if(!refreshing){if(i==1)changeEvolution();else if(i<3)changeMusic();else changePatch();}};
    }
    knobs[0].setRange(0,2,1);knobs[0].textFromValueFunction=[this](double v){
        if(editing.music.composition!=Composition::song)return juce::String(v<.5?"Sparse":v<1.5?"Open":"Layered");
        return juce::String(v<.5?"Triads":v<1.5?"Sevenths":"Ninths");};
    knobs[2].setRange(48,84,1);knobs[2].textFromValueFunction=[](double v){return juce::MidiMessage::getMidiNoteName(int(v),true,true,4)+" / "+juce::String(int(v));};
    knobs[3].setRange(20,500,1);
    const char* extra[]{"Attack / s","Release / s","Drive","Chorus","Detune / cents","Texture","Resonance","Decay / s","Sustain","Filter envelope"};
    for(size_t i=0;i<advanced.size();++i) {
        auto& s=advanced[i];addAndMakeVisible(s);s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight,false,52,24);s.setRange(0,1,.01);
        advancedLabels[i].setText(extra[i],juce::dontSendNotification);addAndMakeVisible(advancedLabels[i]);s.setName(extra[i]);
        s.onValueChange=[this]{if(!refreshing)changePatch();};
    }
    advanced[0].setRange(.005,5,.005);advanced[0].setSkewFactorFromMidPoint(.5);
    advanced[1].setRange(.05,8,.05);advanced[2].setRange(0,1,.01);advanced[4].setRange(0,25,.1);
    advanced[7].setRange(.05,8,.05);
    advanced[5].setTooltip("Ladder: oscillator weight/drive. DCO: pulse blend/width. FM: modulation index.");
    toneModel.setTooltip("Original architectures inspired by classic synth families; not circuit-accurate emulations. Switching keeps your phrase.");
    tonePreset.setTooltip("Changes sound only. Key, tempo, phrase, locks and rhythm stay as they are.");
    const char* captions[]{"KEY","MODE","STARTING CHARACTER","PHRASE LENGTH","CHORD LENGTH","ARTICULATION","CLOCK SOURCE","TEMPO"};
    for(size_t i=0;i<labels.size();++i){labels[i].setText(captions[i],juce::dontSendNotification);labels[i].setFont(font(11,true));addAndMakeVisible(labels[i]);}
    key.setTitle("Key");mode.setTitle("Mode");character.setTitle("Starting character");slots.setTitle("Phrase length");
    bars.setTitle("Bars per chord");rhythm.setTitle("Articulation");clockSource.setTitle("Clock source");tempo.setTitle("Tempo");
    for(size_t i=0;i<advanced.size();++i)advanced[i].setTitle(extra[i]);
    for(size_t i=0;i<chordButtons.size();++i) {
        addAndMakeVisible(chordButtons[i]);chordButtons[i].onClick=[this,i]{send(Action::lock,int(i));};
        chordButtons[i].setTooltip("Click to keep this chord and voicing unchanged.");
        addAndMakeVisible(swapButtons[i]);addAndMakeVisible(restButtons[i]);
        addAndMakeVisible(compareButtons[i]);compareButtons[i].setButtonText("Prev");compareButtons[i].setTitle("Compare chord "+juce::String(int(i)+1));
        compareButtons[i].setTooltip("Toggle this slot between its current and previous swap. Uses the same bar timing as Swap.");compareButtons[i].onClick=[this,i]{send(Action::compareChord,int(i));};
        swapButtons[i].setButtonText("Swap");restButtons[i].setButtonText("Rest");
        swapButtons[i].setTitle("Swap chord "+juce::String(int(i)+1));restButtons[i].setTitle("Rest chord "+juce::String(int(i)+1));
        swapButtons[i].setTooltip("Replace only this slot. Locked slots are protected. While playing: next bar, or the existing queued phrase boundary.");
        restButtons[i].setTooltip("Rest / Restore: pause this slot's pitched layers without shortening it. Tails decay naturally; percussion keeps playing.");
        swapButtons[i].onClick=[this,i]{send(Action::swapChord,int(i));notification=shown.playing?"Chord swap queued; next bar unless a phrase edit is already queued.":"Replaced one chord.";};
        restButtons[i].onClick=[this,i]{send(Action::restChord,int(i));notification=shown.playing?"Rest / Restore queued; next bar unless a phrase edit is already queued.":"Toggled this slot's rest. Restore recalls its saved voicing.";};
    }
    play.onClick=[this]{send(Action::play);};stop.onClick=[this]{send(Action::stop);};panic.onClick=[this]{send(Action::panic);};
    generateButton.onClick=[this]{send(Action::generate);notification=shown.playing?"New phrase queued for the loop boundary.":"Generated a new phrase.";};
    lockAll.onClick=[this]{bool locked=true;for(int i=0;i<shown.session.music.slots;++i)locked&=shown.session.phrase.chords[i].locked;send(Action::lockAll,locked?0:1);};
    undo.onClick=[this]{send(Action::undo);};save.onClick=[this]{chooseSave();};load.onClick=[this]{openSession();};
    audio.onClick=[this]{showAudioSettings();};refreshMidi.onClick=[this]{updateMidiList();};
    recover.onClick=[this]{Session s;auto result=loadSession(recoveryFile,s);if(result.wasOk()){editing=s;showSession(s);send(Action::load);notification="Recovered session; playback is stopped.";}else notification=result.getErrorMessage();};
    for(auto* box:{&key,&mode,&slots,&bars,&rhythm})box->onChange=[this]{if(!refreshing)changeMusic();};
    for(auto* b:{&rootless,&wide})b->onClick=[this]{changeMusic();};
    evolve.onClick=[this]{changeEvolution();};
    evolveTarget.onChange=[this]{if(!refreshing)changeEvolution();};
    evolvePace.onChange=[this]{if(!refreshing)changeEvolution();};
    toneModel.onChange=[this]{if(!refreshing){editing.patch.model=ToneModel(toneModel.getSelectedId()-1);editing.patch.tone=0;showSession(editing);tonePreset.setSelectedId(100,juce::dontSendNotification);send(Action::patch);}};
    tonePreset.onChange=[this]{if(!refreshing&&tonePreset.getSelectedId()!=100){editing.patch=makeTone(tonePreset.getSelectedId()-1);showSession(editing);send(Action::patch);notification="Tone changed; your chords, key and tempo are retained.";}};
    tempo.onValueChange=[this]{if(!refreshing){editing.bpm=tempo.getValue();send(Action::tempo);}};
    character.onChange=[this]{if(refreshing)return;auto s=makePreset(character.getSelectedId()-1,editing.seed);
        s.scenes=shown.session.scenes;s.sceneCycle=shown.session.sceneCycle;s.sceneBars=shown.session.sceneBars;s.sceneVariations=shown.session.sceneVariations;s.sceneVariationSeed=shown.session.sceneVariationSeed;
        editing=s;showSession(s);send(Action::load);notification="Starting character loaded; scenes retained. Press Play when ready.";};
    clockSource.onChange=[this]{if(!refreshing)chooseMidi();};
    editing=makePreset(10);shown.session=editing;shown.next=editing;showSession(editing);
    Command initial;initial.action=Action::load;initial.session=editing;engine.command(initial);
    const auto overrideDirectory=juce::SystemStats::getEnvironmentVariable("UNDERTONE_DATA_DIR",{});
    const auto dataDirectory=overrideDirectory.isNotEmpty()?juce::File(overrideDirectory):juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Undertone");
    recoveryFile=dataDirectory.getChildFile("recovery.undertone");audioSettingsFile=dataDirectory.getChildFile("audio.xml");
    recover.setEnabled(recoveryFile.existsAsFile());
    const auto savedAudio=juce::XmlDocument::parse(audioSettingsFile);
    const auto error=devices.initialise(0,2,savedAudio.get(),true);
    if(error.isNotEmpty())notification="Audio unavailable: "+error+". Open Audio settings.";
    else notification="Choose a key, generate a phrase, and press Play. Space = play/stop; Esc = panic.";
    devices.addAudioCallback(this);updateMidiList();startTimerHz(20);
    for(auto& label:detailLabels)label.setFont(font(12));
    for(auto& label:percussionLabels)label.setFont(font(12));
    roomLabel.setFont(font(12));performanceLevelLabel.setFont(font(12));
    setPerformanceView(true);
}
MainComponent::~MainComponent(){
    stopRecording();recordingThread.stopThread(5000);
    stopTimer();if(audioWindow)delete audioWindow.getComponent();
    if(selectedMidi.isNotEmpty())devices.removeMidiInputDeviceCallback(selectedMidi,this);
    devices.removeAudioCallback(this);
    if(recoveryArmed)saveSession(recoveryFile,engine.status().session);
    if(auto state=devices.createStateXml()){audioSettingsFile.getParentDirectory().createDirectory();state->writeTo(audioSettingsFile);}
    devices.closeAudioDevice();setLookAndFeel(nullptr);
}
void MainComponent::send(Action action,int value){
    Command c;c.action=action;c.session=editing;c.value=value;c.sequence=++sentSequence;c.midiEpoch=midiEpoch.load();
    if(!commands.push(c))notification="Control queue busy; please retry.";
    else { if(action!=Action::external)recoveryArmed=true;
        if(action==Action::undo||action==Action::lockAll)syncSequence=c.sequence;
    }
}
void MainComponent::dispatch(const Command& c){
    if(c.action==Action::external||c.action==Action::load||c.action==Action::panic||c.action==Action::stop)midiTimeline.changeSource(c.midiEpoch);
    engine.command(c);
}
void MainComponent::changeMusic(){
    auto& m=editing.music;m.key=key.getSelectedId()-1;m.mode=Mode(mode.getSelectedId()-1);
    m.composition=Composition(composition.getSelectedId()-1);
    m.slots=1<<slots.getSelectedId();m.bars=1<<(bars.getSelectedId()-1);m.rhythm=Rhythm(rhythm.getSelectedId()-1);
    m.complexity=int(knobs[0].getValue());m.variation=float(knobs[1].getValue());m.floor=int(knobs[2].getValue());
    m.rootless=rootless.getToggleState();m.evolve=evolve.getToggleState();m.spacing=wide.getToggleState()?1:0;send(Action::music);
}
void MainComponent::changePatch(){
    auto& p=editing.patch;p.highpass=float(knobs[3].getValue());p.brightness=float(knobs[4].getValue());
    p.motion=float(knobs[5].getValue());p.space=float(knobs[6].getValue());p.level=float(knobs[7].getValue());
    p.attack=float(advanced[0].getValue());p.release=float(advanced[1].getValue());p.drive=float(advanced[2].getValue());
    p.chorus=float(advanced[3].getValue());p.detune=float(advanced[4].getValue());
    p.texture=float(advanced[5].getValue());p.resonance=float(advanced[6].getValue());p.decay=float(advanced[7].getValue());
    p.sustain=float(advanced[8].getValue());p.filterEnv=float(advanced[9].getValue());p.tone=0;
    tonePreset.setSelectedId(100,juce::dontSendNotification);send(Action::patch);
}
void MainComponent::changeEvolution(){
    auto& m=editing.music;m.evolve=evolve.getToggleState();m.variation=float(knobs[1].getValue());
    m.evolveTarget=evolveTarget.getSelectedId()-1;m.evolveBars=1<<(evolvePace.getSelectedId()-1);
    send(Action::evolution);
}
void MainComponent::changeDetail(){
    auto& d=editing.detail;d.bed=float(detailControls[0].getValue());d.broken=float(detailControls[1].getValue());
    d.motif=float(detailControls[2].getValue());d.wear=float(detailControls[3].getValue());d.echo=float(detailControls[4].getValue());
    d.surface=float(detailControls[5].getValue());
    d.breathe=breathe.getToggleState();d.voice=MelodyVoice(melodyVoice.getSelectedId()-1);send(Action::detail);
}
void MainComponent::changePercussion(){
    auto& p=editing.percussion;p.enabled=percussionEnabled.getToggleState();p.level=float(percussionControls[0].getValue());
    p.density=float(percussionControls[1].getValue());p.glitch=float(percussionControls[2].getValue());p.color=float(percussionControls[3].getValue());send(Action::percussion);
}
void MainComponent::changeCycles(){
    editing.cycles.composition=compositionCycle.getToggleState();editing.cycles.motif=motifCycle.getToggleState();
    editing.cycles.compositionBars=compositionInterval.getSelectedId();editing.cycles.motifBars=motifInterval.getSelectedId();
    send(Action::cycles);
}
void MainComponent::changeScenes(){
    editing.sceneCycle=sceneCycle.getToggleState();editing.sceneBars=sceneInterval.getSelectedId();editing.sceneVariations=sceneVariations.getToggleState();send(Action::sceneControls);
}
void MainComponent::setPerformanceView(bool value){
    performanceView=value;
    if(auto* viewport=findParentComponentOfClass<juce::Viewport>()){viewport->setViewPosition(0,0);setSize(getWidth(),std::max(preferredHeight(),viewport->getHeight()-16));}
    else setSize(getWidth(),preferredHeight());
    resized();repaint();
}
void MainComponent::keepCurrentMoment(){
    const auto directory=recoveryFile.getParentDirectory().getChildFile("Moments");
    const auto file=directory.getNonexistentChildFile("Moment "+juce::Time::getCurrentTime().formatted("%Y-%m-%d %H-%M-%S"),".undertone");
    const auto result=saveSession(file,shown.session);
    notification=result.wasOk()?"Kept: "+file.getFullPathName():result.getErrorMessage();
}
void MainComponent::toggleRecording(){
    if(recordingWriter){stopRecording();return;}
    if(!audioActive.load()){notification="Select an audio output before recording.";return;}
    chooser=std::make_unique<juce::FileChooser>("Record Undertone output",captureDirectory().getChildFile("Undertone take.wav"),"*.wav");
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe](const juce::FileChooser& fc){
        if(!safe||fc.getResult()==juce::File())return;
        const juce::ScopedLock lock(safe->devices.getAudioCallbackLock());auto* device=safe->devices.getCurrentAudioDevice();
        if(!device||!safe->audioActive.load()){safe->notification="Audio output stopped; recording was not started.";return;}
        const auto file=fc.getResult().withFileExtension("wav");auto stream=file.createOutputStream();
        if(!stream||!stream->setPosition(0)||stream->truncate().failed()){safe->notification="Cannot create recording file.";return;}
        std::unique_ptr<juce::OutputStream> output=std::move(stream);juce::WavAudioFormat format;
        auto writer=format.createWriterFor(output,juce::AudioFormatWriterOptions{}.withSampleRate(device->getCurrentSampleRate()).withNumChannels(2).withBitsPerSample(24));
        if(!writer){safe->notification="Cannot create WAV writer.";return;}
        safe->recordingFile=file;safe->recordingRate=device->getCurrentSampleRate();safe->recordedFrames=0;safe->recordingOverflow=false;safe->recordingChanged=false;
        safe->recordingWriter=std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(writer.release(),safe->recordingThread,262144);
        safe->activeWriter.store(safe->recordingWriter.get());safe->recordAudio.setButtonText("Stop recording");safe->notification="Recording stereo output, including live changes and tails.";
    });
}
void MainComponent::stopRecording(){
    if(!recordingWriter)return;
    {const juce::ScopedLock lock(devices.getAudioCallbackLock());activeWriter.store(nullptr);}
    recordingWriter.reset();recordAudio.setButtonText("Record WAV");
    juce::WavAudioFormat format;std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(recordingFile.createInputStream().release(),true));
    if(recordingOverflow.load()||!reader||reader->lengthInSamples!=recordedFrames.load())notification="Recording incomplete; check disk space/performance. File: "+recordingFile.getFullPathName();
    else notification=(recordingChanged.load()?"Recording stopped after an audio-device change. Saved: ":"Recorded: ")+recordingFile.getFullPathName();
}
void MainComponent::chooseMidiExport(){
    chooser=std::make_unique<juce::FileChooser>("Export generated MIDI",captureDirectory().getChildFile("Undertone phrase.mid"),"*.mid");
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe](const juce::FileChooser& fc){
        if(!safe||fc.getResult()==juce::File())return;
        const auto state=safe->shown.session;const auto file=fc.getResult().withFileExtension("mid");const int bars=safe->exportBars.getSelectedId();
        safe->exportMidi.setEnabled(false);safe->notification="Rendering MIDI from the current state; playback can continue.";
        juce::Thread::launch([safe,state,file,bars]{const auto result=exportMidiFile(file,state,bars);
            juce::MessageManager::callAsync([safe,file,result]{if(safe){safe->exportMidi.setEnabled(true);safe->notification=result.wasOk()?"Exported MIDI: "+file.getFullPathName():result.getErrorMessage();}});});
    });
}
void MainComponent::showSession(const Session& s){
    refreshing=true;const auto& m=s.music;const auto& p=s.patch;
    phrasing.setToggleState(s.detail.phrasing,juce::dontSendNotification);motifMode.setSelectedId(int(s.detail.motifMode)+1,juce::dontSendNotification);
    swapStyle.setSelectedId(int(s.swapStyle)+1,juce::dontSendNotification);room.setValue(s.detail.room,juce::dontSendNotification);performanceLevel.setValue(s.patch.level,juce::dontSendNotification);
    sceneCycle.setToggleState(s.sceneCycle,juce::dontSendNotification);sceneVariations.setToggleState(s.sceneVariations,juce::dontSendNotification);sceneInterval.setSelectedId(s.sceneBars,juce::dontSendNotification);
    const double detailValues[]{s.detail.bed,s.detail.broken,s.detail.motif,s.detail.wear,s.detail.echo,s.detail.surface};
    for(int i=0;i<6;++i)detailControls[i].setValue(detailValues[i],juce::dontSendNotification);
    composition.setSelectedId(int(m.composition)+1,juce::dontSendNotification);
    compositionCycle.setToggleState(s.cycles.composition,juce::dontSendNotification);motifCycle.setToggleState(s.cycles.motif,juce::dontSendNotification);
    compositionInterval.setSelectedId(s.cycles.compositionBars,juce::dontSendNotification);motifInterval.setSelectedId(s.cycles.motifBars,juce::dontSendNotification);
    breathe.setToggleState(s.detail.breathe,juce::dontSendNotification);
    melodyVoice.setSelectedId(int(s.detail.voice)+1,juce::dontSendNotification);
    percussionEnabled.setToggleState(s.percussion.enabled,juce::dontSendNotification);
    const double percussionValues[]{s.percussion.level,s.percussion.density,s.percussion.glitch,s.percussion.color};
    for(int i=0;i<4;++i)percussionControls[i].setValue(percussionValues[i],juce::dontSendNotification);
    key.setSelectedId(m.key+1,juce::dontSendNotification);mode.setSelectedId(int(m.mode)+1,juce::dontSendNotification);
    character.setSelectedId(s.preset+1,juce::dontSendNotification);slots.setSelectedId(m.slots==2?1:m.slots==4?2:3,juce::dontSendNotification);
    bars.setSelectedId(m.bars==1?1:m.bars==2?2:3,juce::dontSendNotification);rhythm.setSelectedId(int(m.rhythm)+1,juce::dontSendNotification);
    tempo.setValue(s.bpm,juce::dontSendNotification);evolve.setToggleState(m.evolve,juce::dontSendNotification);
    rootless.setToggleState(m.rootless,juce::dontSendNotification);wide.setToggleState(m.spacing!=0,juce::dontSendNotification);
    const double values[]{double(m.complexity),m.variation,double(m.floor),p.highpass,p.brightness,p.motion,p.space,p.level};
    for(size_t i=0;i<knobs.size();++i)knobs[i].setValue(values[i],juce::dontSendNotification);
    knobs[0].updateText();
    toneModel.setSelectedId(int(p.model)+1,juce::dontSendNotification);
    tonePreset.setSelectedId(p.tone>0?p.tone+1:100,juce::dontSendNotification);
    evolveTarget.setSelectedId(m.evolveTarget+1,juce::dontSendNotification);
    int paceId=1;for(int n=m.evolveBars;n>1;n/=2)++paceId;evolvePace.setSelectedId(paceId,juce::dontSendNotification);
    const double extras[]{p.attack,p.release,p.drive,p.chorus,p.detune,p.texture,p.resonance,p.decay,p.sustain,p.filterEnv};
    for(size_t i=0;i<advanced.size();++i)advanced[i].setValue(extras[i],juce::dontSendNotification);
    for(int i=5;i<10;++i)advanced[i].setEnabled(p.model!=ToneModel::warm&&(p.model!=ToneModel::fm||(i!=6&&i!=9)));
    advanced[4].setEnabled(p.model==ToneModel::warm||p.model==ToneModel::ladder);
    refreshing=false;
}
void MainComponent::updateMidiList(){
    refreshing=true;clockSource.clear();clockSource.addItem("Internal clock",1);
    const auto inputs=juce::MidiInput::getAvailableDevices();int selected=1;
    for(int i=0;i<inputs.size();++i){clockSource.addItem(inputs[i].name,i+2);if(inputs[i].identifier==selectedMidi)selected=i+2;}
    clockSource.setSelectedId(selected,juce::dontSendNotification);refreshing=false;
    if(selected==1&&selectedMidi.isNotEmpty()){chooseMidi();notification="MIDI source disconnected; playback stopped.";}
}
void MainComponent::chooseMidi(){
    if(selectedMidi.isNotEmpty()){devices.removeMidiInputDeviceCallback(selectedMidi,this);devices.setMidiInputDeviceEnabled(selectedMidi,false);}
    ++midiEpoch;
    selectedMidi.clear();const auto inputs=juce::MidiInput::getAvailableDevices();const int index=clockSource.getSelectedId()-2;
    if(index>=0&&index<inputs.size()){
        selectedMidi=inputs[index].identifier;devices.setMidiInputDeviceEnabled(selectedMidi,true);
        if(devices.isMidiInputDeviceEnabled(selectedMidi))devices.addMidiInputDeviceCallback(selectedMidi,this);
        else {selectedMidi.clear();notification="Could not open MIDI source. It may be in use by another application.";}
    }
    send(Action::external,selectedMidi.isNotEmpty()?1:0);tempo.setEnabled(selectedMidi.isEmpty());play.setEnabled(selectedMidi.isEmpty());
    if(selectedMidi.isNotEmpty())notification="Waiting for MIDI Start from the selected instrument. Audio is routed separately.";
}
void MainComponent::showAudioSettings(){
    if(audioWindow){audioWindow->toFront(true);return;}
    auto* selector=new juce::AudioDeviceSelectorComponent(devices,0,0,0,2,false,false,true,false);selector->setSize(580,460);
    juce::DialogWindow::LaunchOptions options;options.content.setOwned(selector);options.dialogTitle="Undertone / Audio settings";
    options.dialogBackgroundColour=panel;options.escapeKeyTriggersCloseButton=true;options.useNativeTitleBar=true;
    options.resizable=false;options.componentToCentreAround=this;audioWindow=options.launchAsync();
}
void MainComponent::chooseSave(){
    chooser=std::make_unique<juce::FileChooser>("Save accompaniment session",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Untitled.undertone"),"*.undertone");
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe](const juce::FileChooser& fc){
        if(!safe||fc.getResult()==juce::File())return;
        auto result=saveSession(fc.getResult().withFileExtension("undertone"),safe->shown.session);
        safe->notification=result.wasOk()?"Saved current phrase and settings. Pending harmonic changes are not saved.":result.getErrorMessage();
    });
}
void MainComponent::openSession(){
    chooser=std::make_unique<juce::FileChooser>("Open accompaniment session",juce::File(),"*.undertone");
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe](const juce::FileChooser& fc){
        if(!safe||fc.getResult()==juce::File())return;Session s;auto result=loadSession(fc.getResult(),s);
        if(result.wasOk()){safe->editing=s;safe->showSession(s);safe->send(Action::load);safe->notification="Session loaded; playback stopped.";}
        else safe->notification=result.getErrorMessage();
    });
}
void MainComponent::timerCallback(){
    if(recordingWriter&&(recordingChanged.load()||recordingOverflow.load()))stopRecording();
    const int previousSlots=shown.session.music.slots;
    if(!audioActive.load()){
        const juce::ScopedLock lock(devices.getAudioCallbackLock());
        if(!audioActive.load()){Command command;while(commands.pop(command))dispatch(command);shown=midiTimeline.status(engine);statuses.push(shown);}
    }
    Status s;bool received=false;while(statuses.pop(s)){shown=s;received=true;}
    if(received){
        if(shown.sequence>=sentSequence&&shown.stateRevision!=seenRevision){seenRevision=shown.stateRevision;editing=shown.session;showSession(editing);}
        // Automation owns these two values after acknowledged controls; don't send an old seed/mode back on a slider edit.
        if(shown.sequence>=sentSequence){editing.detail.seed=shown.session.detail.seed;
            if(!shown.manualPending){editing.music.composition=shown.session.music.composition;composition.setSelectedId(int(editing.music.composition)+1,juce::dontSendNotification);}}
        if(syncSequence&&shown.sequence>=syncSequence){editing=shown.pending?shown.next:shown.session;showSession(editing);syncSequence=0;}
        if(shown.rejected!=lastRejected){lastRejected=shown.rejected;notification="Edit not applied. Unlock the chord before swapping/resting; unlock the phrase before changing key, register, spacing or composition.";editing=shown.session;showSession(editing);}
        for(int i=0;i<8;++i){auto& button=chordButtons[size_t(i)];const bool visible=i<shown.session.music.slots;button.setVisible(visible);swapButtons[i].setVisible(visible);restButtons[i].setVisible(visible);compareButtons[i].setVisible(visible);if(!visible)continue;
            const auto& chord=shown.session.phrase.chords[i];juce::String text=juce::String(i+1)+"  "+chordName(chord);
            const auto& target=shown.manualPending&&i<shown.next.music.slots?shown.next.phrase.chords[i]:chord;
            swapButtons[i].setEnabled(!chord.locked);restButtons[i].setEnabled(!chord.locked);
            compareButtons[i].setEnabled(!chord.locked&&(shown.compareMask&(1<<i)));
            restButtons[i].setButtonText(target.rest?"Restore":"Rest");restButtons[i].setTitle(juce::String(target.rest?"Restore chord ":"Rest chord ")+juce::String(i+1));
            if(chord.locked)text+="  [LOCK]";
            if(shown.pending&&!sameChord(chord,shown.next.phrase.chords[i]))text+=juce::String("  > ")+juce::String(chordName(shown.next.phrase.chords[i]));
            button.setButtonText(text);button.setTitle(text);button.setToggleState(chord.locked,juce::dontSendNotification);
            button.number=i+1;button.name=chordName(chord);button.locked=chord.locked;button.active=shown.playing&&i==shown.slot;
            button.resting=chord.rest;button.voicing.clear();
            for(int note=0;note<chord.count;++note){if(note)button.voicing+="  ";button.voicing+=juce::MidiMessage::getMidiNoteName(chord.notes[note],true,true,4);}
            button.progress=float(std::fmod(shown.beat,shown.session.music.bars*4.)/(shown.session.music.bars*4.));
            button.preview=shown.pending&&i<shown.next.music.slots&&!sameChord(chord,shown.next.phrase.chords[i])?juce::String("Next: ")+juce::String(chordName(shown.next.phrase.chords[i])):juce::String();
        }
        undo.setEnabled(shown.canUndo);bool all=true;for(int i=0;i<shown.session.music.slots;++i)all&=shown.session.phrase.chords[i].locked;
        lockAll.setButtonText(all?"Unlock phrase":"Lock phrase");
        if(shown.external){refreshing=true;tempo.setValue(shown.session.bpm,juce::dontSendNotification);refreshing=false;}
    }
    {const juce::ScopedLock lock(errorLock);if(deviceError.isNotEmpty()){notification="Audio: "+deviceError;deviceError.clear();}}
    if(shown.midiResets>seenMidiResets&&shown.external)notification="Timing stream reset; stopped. Check the connection and send a fresh MIDI Start.";
    seenMidiResets=shown.midiResets;
    if(++recoveryTicks>=200&&received&&recoveryArmed){recoveryTicks=0;auto result=saveSession(recoveryFile,shown.session);recover.setEnabled(result.wasOk());if(result.failed())notification="Recovery copy unavailable: "+result.getErrorMessage();}
    const char* clockText=shown.clock==ClockState::internal?"INTERNAL":shown.clock==ClockState::waiting?"WAITING FOR CLOCK/START":shown.clock==ClockState::lost?"CLOCK LOST":"MIDI LOCKED";
    transportLabel.setText(juce::String(shown.playing?"PLAYING":"STOPPED")+"  /  "+clockText+"  /  BAR "+juce::String(int(shown.beat/4)+1),juce::dontSendNotification);
    if(!audioActive.load())transportLabel.setText("AUDIO DEVICE NOT ACTIVE",juce::dontSendNotification);
    play.setEnabled(audioActive.load()&&!shown.external);
    transportLabel.setColour(juce::Label::textColourId,shown.playing?accent:muted);
    messageLabel.setText(notification,juce::dontSendNotification);
    clockHint.setText(shown.external?"MIDI buffer: "+juce::String(shown.midiDelayMs,1)+" ms  |  Late: "+juce::String(shown.lateMidi):"External MIDI timing is configured by the audio buffer.",juce::dontSendNotification);
    const auto& em=shown.session.music;
    recallA.setEnabled(shown.session.scenes[0].saved);recallB.setEnabled(shown.session.scenes[1].saved);
    recallA.setToggleState(shown.session.activeScene==0,juce::dontSendNotification);recallB.setToggleState(shown.session.activeScene==1,juce::dontSendNotification);
    recordAudio.setToggleState(recordingWriter!=nullptr,juce::dontSendNotification);
    stop.setToggleState(!shown.playing,juce::dontSendNotification);
    sceneHeading.setText(juce::String("SCENES  /  ")+(shown.session.activeScene==0?"A":shown.session.activeScene==1?"B":"LIVE")+
        (shown.session.sceneCycle?(shown.session.scenes[0].saved&&shown.session.scenes[1].saved?" / next in "+juce::String(shown.sceneBeats/4,1)+" bars":" / store A and B to alternate"):""),juce::dontSendNotification);
    captureHint.setText(recordingWriter?"RECORDING / "+juce::String(recordedFrames.load()/std::max(1.,recordingRate),1)+" s":"SNAPSHOT  /  AUDIO  /  MIDI",juce::dontSendNotification);
    const auto& cycles=shown.session.cycles;
    juce::String cycleText=shown.manualPending?"Manual edit queued; automatic changes wait.":!shown.playing?"Cycles follow Play / MIDI Start. Stop pauses; MIDI Continue resumes.":
        juce::String("Composition: ")+(cycles.composition?(em.composition==Composition::song?juce::String("paused in Song"):juce::String(shown.compositionBeats/4,1)+" bars to switch"):juce::String("off"))+
        "  |  Motif: "+(shown.session.detail.motifMode==MotifMode::hold?juce::String("held"):cycles.motif?juce::String(shown.motifBeats/4,1)+" bars to change":juce::String("off"));
    cycleHint.setText(cycleText,juce::dontSendNotification);
    formLabel.setText(em.composition==Composition::song?"":shown.session.detail.breathe?juce::String("32-BAR FORM / ")+sectionName(shown.section):"FORM OFF / STEADY LAYER BALANCE",juce::dontSendNotification);
    bool allLocked=true;for(int i=0;i<em.slots;++i)allLocked&=shown.session.phrase.chords[i].locked;
    juce::String evolutionText=!em.evolve?"Evolve is off.":em.variation==0?"Evolve paused: Variation is zero.":!shown.playing?"Evolve is ready; press Play or send MIDI Start.":
        (em.evolveTarget==1&&allLocked)?"No chord changes: every chord is locked.":
        juce::String(em.evolveTarget==2?"Tone morphing":allLocked?"Chords locked; tone morphing":em.evolveTarget==1?"Chords evolving":"Chords + tone evolving")+
        "  |  Next boundary in "+juce::String(shown.evolutionBeats/4,1)+" bars  |  Chord changes: "+juce::String(shown.evolutions);
    if(em.evolve&&em.evolveTarget!=1&&em.variation>0)evolutionText+="  |  Brightness "+juce::String(shown.toneBrightness,2)+" / Texture "+juce::String(shown.toneTexture,2);
    evolutionHint.setText(evolutionText,juce::dontSendNotification);
    if(previousSlots!=shown.session.music.slots)resized();repaint();
}
void MainComponent::audioDeviceIOCallbackWithContext(const float* const*,int,float* const* outputs,int channels,int samples,const juce::AudioIODeviceCallbackContext&){
    const double callbackTime=juce::Time::getMillisecondCounterHiRes()*.001;
    juce::ScopedNoDenormals noDenormals;Command c;while(commands.pop(c))dispatch(c);
    midiTimeline.process(engine,channels>0?outputs[0]:nullptr,channels>1?outputs[1]:nullptr,samples,callbackTime);
    for(int channel=2;channel<channels;++channel)if(outputs[channel])juce::FloatVectorOperations::clear(outputs[channel],samples);
    if(auto* writer=activeWriter.load()){if(channels>=2&&outputs[0]&&outputs[1]){const float* data[]{outputs[0],outputs[1]};if(writer->write(data,samples))recordedFrames+=samples;else recordingOverflow=true;}else recordingChanged=true;}
    reportSamples+=samples;if(reportSamples>=2048){statuses.push(midiTimeline.status(engine));reportSamples=0;}
}
void MainComponent::audioDeviceAboutToStart(juce::AudioIODevice* device){
    if(activeWriter.load()){activeWriter=nullptr;recordingChanged=true;}
    engine.prepare(device->getCurrentSampleRate());midiTimeline.prepare(device->getCurrentSampleRate(),device->getCurrentBufferSizeSamples());reportSamples=0;audioActive=true;
}
void MainComponent::audioDeviceStopped(){if(activeWriter.load()){activeWriter=nullptr;recordingChanged=true;}audioActive=false;Command c;c.action=Action::stop;c.sequence=engine.status().sequence;engine.command(c);statuses.push(midiTimeline.status(engine));}
void MainComponent::audioDeviceError(const juce::String& error){const juce::ScopedLock lock(errorLock);deviceError=error;}
void MainComponent::handleIncomingMidiMessage(juce::MidiInput*,const juce::MidiMessage& message){
    if(message.isMidiClock()||message.isMidiStart()||message.isMidiStop()||message.isMidiContinue())
        midiTimeline.push({message.getRawData()[0],message.getTimeStamp()>0?message.getTimeStamp():juce::Time::getMillisecondCounterHiRes()*.001,midiEpoch.load()});
}
bool MainComponent::keyPressed(const juce::KeyPress& keypress){
    if(keypress==juce::KeyPress::spaceKey){
        if(!audioActive.load())notification="Select an active output in Audio settings before playing.";
        else send(shown.playing?Action::stop:Action::play);
        return true;
    }
    if(keypress==juce::KeyPress::escapeKey){send(Action::panic);return true;}return false;
}
void MainComponent::paint(juce::Graphics& g){
    const float w=float(getWidth());g.fillAll(background);
    g.setColour(panel);g.fillRect(0.f,0.f,w,85.f);
    g.setColour(line);g.drawHorizontalLine(85,24,w-24);
    // A small, original interval mark. Every animated display below uses live engine state.
    g.setColour(accent);for(int i=0;i<5;++i){const float height=12.f+float((i*7)%5)*5;g.fillRoundedRectangle(29.f+i*6,43-height*.5f,2,height,1);}
    g.setColour(ink);g.setFont(font(29,true));g.drawText("undertone",70,19,210,37,juce::Justification::centredLeft);
    g.setColour(muted);g.setFont(font(10));g.drawText("GENERATIVE ACCOMPANIMENT",72,55,250,17,juce::Justification::centredLeft);
    auto surface=[&](float x,float y,float width,float height){g.setColour(panel);g.fillRoundedRectangle(x,y,width,height,7);g.setColour(line.withAlpha(.55f));g.drawRoundedRectangle(x+.5f,y+.5f,width-1,height-1,7,1);};
    surface(20,184,w-40,206);
    g.setColour(accent);g.fillRoundedRectangle(33,195,3,13,1);
    g.setColour(muted);g.setFont(font(11,true));g.drawText("HARMONY",44,191,120,22,juce::Justification::centredLeft);
    g.setFont(font(11));g.drawText("Click a chord to lock its voicing",164,191,300,22,juce::Justification::centredLeft);
    const auto current=chordName(shown.session.phrase.chords[shown.slot]);const auto next=chordName(shown.session.phrase.chords[(shown.slot+1)%shown.session.music.slots]);
    g.setColour(muted);g.setFont(font(10,true));g.drawText("NOW",35,341,38,20,juce::Justification::centredLeft);
    g.setColour(ink);g.setFont(font(16,true));g.drawText(current,76,337,210,27,juce::Justification::centredLeft);
    g.setColour(muted);g.setFont(font(10,true));g.drawText("NEXT",290,341,40,20,juce::Justification::centredLeft);
    g.setFont(font(14));g.drawText(next,337,337,215,27,juce::Justification::centredLeft);
    g.setColour(shown.pending?accent:muted);g.setFont(font(10));g.drawText(shown.pending?"CHANGE QUEUED":""+juce::String(shown.session.music.slots)+" CHORDS  /  "+juce::String(shown.session.music.slots*shown.session.music.bars)+" BARS",getWidth()-290,339,254,24,juce::Justification::centredRight);
    const float meterX=w-145;g.setColour(muted);g.setFont(font(9));g.drawText("OUT",int(meterX-33),57,28,17,juce::Justification::centredLeft);
    for(int i=0;i<18;++i){g.setColour(shown.peak>float(i)/18?(i>15?danger:mint):line);g.fillRect(meterX+i*6,62.f,4.f,6.f);}
    if(performanceView){
        surface(20,400,w-40,65);surface(20,473,w-40,67);surface(20,548,w-40,61);surface(20,617,w-40,84);
        const int mixW=(getWidth()-72)/6;
        for(int i=1;i<6;++i){g.setColour(line.withAlpha(.6f));g.drawVerticalLine(36+i*mixW-12,486,526);}
    }else{
        surface(20,404,w-40,190);surface(20,658,w-40,198);surface(20,865,w-40,84);
        surface(20,949,w-40,165);surface(20,1118,w-40,122);surface(20,1252,w-40,148);
    }
    g.setColour(line);g.drawHorizontalLine(getHeight()-53,24,w-24);
    g.setColour(muted);g.setFont(font(9));g.drawText("UNDERTONE  /  0.9",getWidth()-150,getHeight()-33,125,15,juce::Justification::centredRight);
}
void MainComponent::resized(){
    const int w=getWidth();int x=28;
    transportLabel.setBounds(w-450,26,420,25);messageLabel.setBounds(28,getHeight()-46,w-210,39);
    for(auto* b:{&play,&stop,&panic,&generateButton,&lockAll,&undo}){const int width=b==&play?132:b==&generateButton?112:b==&lockAll?120:76;b->setBounds(x,103,width,34);x+=width+8;}
    save.setBounds(w-193,103,75,34);load.setBounds(w-110,103,80,34);
    auto place=[&](juce::Component& c,int index,int px,int py,int width){labels[size_t(index)].setBounds(px,py-20,width,18);c.setBounds(px,py,width,30);};
    place(key,0,28,153,82);place(mode,1,120,153,155);place(character,2,285,153,282);
    evolve.setBounds(580,153,90,30);rootless.setBounds(678,153,110,30);
    wide.setBounds(790,153,140,30);percussionEnabled.setBounds(940,153,130,30);
    const int count=shown.session.music.slots;const int chordW=(w-80)/count;
    for(int i=0;i<8;++i){const int left=34+i*chordW,third=(chordW-16)/3;
        chordButtons[size_t(i)].setBounds(left,224,chordW-8,72);
        swapButtons[i].setBounds(left,299,third,26);restButtons[i].setBounds(left+third+4,299,third,26);compareButtons[i].setBounds(left+2*(third+4),299,third,26);
        if(count==8){const int half=(chordW-12)/2;chordButtons[size_t(i)].setBounds(left,224,chordW-8,62);
            swapButtons[i].setBounds(left,289,half,20);compareButtons[i].setBounds(left+half+4,289,half,20);restButtons[i].setBounds(left,312,chordW-8,20);}}
    const int knobW=(w-60)/8;
    for(int i=0;i<8;++i){knobLabels[size_t(i)].setBounds(30+i*knobW,385,knobW,24);knobs[size_t(i)].setBounds(32+i*knobW,412,knobW-4,133);}
    place(slots,3,28,594,155);place(bars,4,195,594,175);place(rhythm,5,382,594,175);
    place(clockSource,6,570,594,w-760);refreshMidi.setBounds(w-178,594,148,30);
    const int extraW=(w-58)/5;
    soundLabels[0].setBounds(28,640,310,20);toneModel.setBounds(28,663,310,30);
    soundLabels[1].setBounds(358,640,480,20);tonePreset.setBounds(358,663,380,30);
    for(int i=0;i<10;++i){const int px=28+(i%5)*extraW,py=704+(i/5)*64;advancedLabels[size_t(i)].setBounds(px,py,extraW-10,20);advanced[size_t(i)].setBounds(px,py+21,extraW-10,30);}
    soundLabels[2].setBounds(28,837,250,20);evolveTarget.setBounds(28,859,250,30);
    soundLabels[3].setBounds(298,837,300,20);evolvePace.setBounds(298,859,250,30);
    evolutionHint.setBounds(34,334,w-68,22);
    compositionLabel.setBounds(600,837,180,20);composition.setBounds(600,859,260,30);formLabel.setBounds(600,895,w-628,20);
    detailHeading.setBounds(28,917,w-56,20);const int detailW=(w-56)/6;
    for(int i=0;i<6;++i){detailLabels[i].setBounds(28+i*detailW,948,detailW-10,20);detailControls[i].setBounds(28+i*detailW,971,detailW-10,30);}
    breathe.setBounds(28,1015,260,30);newMotif.setBounds(310,1015,140,30);detailHint.setBounds(28,1050,w-56,28);
    melodyVoiceLabel.setBounds(480,1015,120,30);melodyVoice.setBounds(610,1015,260,30);
    percussionHeading.setBounds(28,1092,w-56,20);const int percussionW=(w-58)/4;
    for(int i=0;i<4;++i){percussionLabels[i].setBounds(28+i*percussionW,1120,percussionW-10,20);percussionControls[i].setBounds(28+i*percussionW,1143,percussionW-10,30);}
    newBeat.setBounds(28,1184,140,30);percussionHint.setBounds(190,1182,w-218,32);
    place(tempo,7,28,1244,300);audio.setBounds(358,1244,155,30);recover.setBounds(525,1244,110,30);
    clockHint.setBounds(650,1239,w-680,40);
    // Keep the existing control rows aligned while inserting chord actions and cycle controls.
    for(auto* child:getChildren())if(child!=&messageLabel&&child!=&cycleHeading&&child!=&cycleHint
        &&child!=&compositionCycle&&child!=&motifCycle&&child!=&compositionInterval&&child!=&motifInterval
        &&child!=&compositionIntervalLabel&&child!=&motifIntervalLabel){
        const int y=child->getY();if(y>=304)child->setTopLeftPosition(child->getX(),y+32+(y>=1092?140:0));}
    // The eight-slot second action row belongs to the fixed harmony strip.
    if(count==8)for(auto& button:restButtons)button.setTopLeftPosition(button.getX(),312);
    cycleHeading.setBounds(28,1124,w-56,20);
    compositionCycle.setBounds(28,1154,260,30);compositionIntervalLabel.setBounds(298,1154,110,30);compositionInterval.setBounds(410,1154,130,30);
    motifCycle.setBounds(570,1154,235,30);motifIntervalLabel.setBounds(805,1154,110,30);motifInterval.setBounds(917,1154,135,30);
    cycleHint.setBounds(28,1194,w-56,40);
    for(auto* child:getChildren())child->setVisible(!performanceView);
    juce::Component* extras[]{&storeA,&storeB,&recallA,&recallB,&sceneHeading,&sceneCycle,&sceneInterval,&sceneVariations,&motifMode,&motifModeLabel,&varyMotif,&newFigure,&phrasing,&room,&roomLabel,&performanceLevel,&performanceLevelLabel,&keepMoment,&recordAudio,&exportMidi,&exportBars,&captureHint};
    for(auto* child:extras)child->setVisible(performanceView);
    juce::Component* common[]{&performanceTab,&soundTab,&transportLabel,&messageLabel,&play,&stop,&panic,&generateButton,&lockAll,&undo,&save,&load,&key,&mode,&character,&evolve,&rootless,&wide,&percussionEnabled,&swapStyle};
    for(auto* child:common)child->setVisible(true);
    for(int i=0;i<3;++i)labels[i].setVisible(true);
    performanceTab.setBounds(340,30,112,32);soundTab.setBounds(457,30,82,32);
    performanceTab.setToggleState(performanceView,juce::dontSendNotification);soundTab.setToggleState(!performanceView,juce::dontSendNotification);
    swapStyle.setBounds(w-250,190,215,28);
    for(int i=0;i<8;++i){const bool visible=i<shown.session.music.slots;chordButtons[i].setVisible(visible);swapButtons[i].setVisible(visible);restButtons[i].setVisible(visible);compareButtons[i].setVisible(visible);}
    if(performanceView){
        evolutionHint.setVisible(true);evolutionHint.setBounds(34,364,w-68,22);
        sceneHeading.setBounds(34,402,w-68,22);recallA.setBounds(34,429,92,27);storeA.setBounds(132,429,66,27);
        recallB.setBounds(218,429,92,27);storeB.setBounds(316,429,66,27);sceneCycle.setBounds(420,429,160,27);sceneInterval.setBounds(585,429,125,27);sceneVariations.setBounds(730,429,190,27);
        const int mixW=(w-72)/6;
        for(int i=0;i<3;++i){detailLabels[i].setVisible(true);detailControls[i].setVisible(true);detailLabels[i].setBounds(36+i*mixW,476,mixW-10,20);detailControls[i].setBounds(36+i*mixW,501,mixW-10,30);}
        percussionLabels[0].setVisible(true);percussionControls[0].setVisible(true);percussionLabels[0].setBounds(36+3*mixW,476,mixW-10,20);percussionControls[0].setBounds(36+3*mixW,501,mixW-10,30);
        performanceLevelLabel.setBounds(36+4*mixW,476,mixW-10,20);performanceLevel.setBounds(36+4*mixW,501,mixW-10,30);
        roomLabel.setBounds(36+5*mixW,476,mixW-10,20);room.setBounds(36+5*mixW,501,mixW-10,30);
        motifModeLabel.setBounds(34,549,190,16);motifMode.setBounds(28,566,180,30);varyMotif.setBounds(224,566,120,30);newMotif.setVisible(true);newMotif.setBounds(354,566,120,30);
        newFigure.setBounds(484,566,120,30);phrasing.setBounds(626,566,260,30);
        compositionLabel.setVisible(true);composition.setVisible(true);compositionLabel.setBounds(906,549,140,16);composition.setBounds(900,566,w-928,30);
        cycleHeading.setVisible(true);cycleHeading.setBounds(34,618,w-68,20);
        for(auto* child:std::initializer_list<juce::Component*>{&compositionCycle,&motifCycle,&compositionInterval,&motifInterval,&compositionIntervalLabel,&motifIntervalLabel,&cycleHint})child->setVisible(true);
        compositionCycle.setBounds(28,638,250,30);compositionIntervalLabel.setBounds(280,638,110,30);compositionInterval.setBounds(390,638,130,30);
        motifCycle.setBounds(550,638,225,30);motifIntervalLabel.setBounds(780,638,110,30);motifInterval.setBounds(890,638,150,30);cycleHint.setBounds(28,674,w-56,24);
        keepMoment.setBounds(28,710,160,30);recordAudio.setBounds(200,710,150,30);exportMidi.setBounds(365,710,130,30);exportBars.setBounds(510,710,110,30);captureHint.setBounds(640,703,w-668,42);
        tempo.setVisible(true);labels[7].setVisible(true);labels[7].setBounds(28,747,80,18);tempo.setBounds(100,752,240,30);
        audio.setVisible(true);audio.setBounds(358,752,155,30);recover.setVisible(true);recover.setBounds(525,752,110,30);
        clockHint.setVisible(true);clockHint.setBounds(650,746,w-680,40);
    }
}
void MainComponent::smokeTest(const juce::File& directory){
    // Optional fixture for repeatable layout inspection; only used by --smoke.
    Session fixture;
    if(loadSession(directory.getChildFile("input.undertone"),fixture).wasOk()){
        const juce::ScopedLock lock(devices.getAudioCallbackLock());Command c;c.action=Action::load;c.session=fixture;dispatch(c);
        Status discarded;while(statuses.pop(discarded)){}statuses.push(engine.status());timerCallback();editing=shown.session;showSession(editing);resized();
    }
    directory.createDirectory();auto image=createComponentSnapshot(getLocalBounds());juce::PNGImageFormat png;
    if(auto stream=directory.getChildFile("window.png").createOutputStream()){
        if(stream->setPosition(0)&&stream->truncate().wasOk())png.writeImageToStream(image,*stream);
    }
    saveSession(directory.getChildFile("smoke.undertone"),shown.session);
    juce::String report="Native UI instantiated; "+juce::String(getWidth())+"x"+juce::String(getHeight())+"\n";
    report+="Chord controls: "+swapButtons[0].getBounds().toString()+" / "+restButtons[0].getBounds().toString()+" / "+compareButtons[0].getBounds().toString()+"\n";
    if(auto* device=devices.getCurrentAudioDevice())report+="Audio: "+device->getName()+" / "+juce::String(device->getCurrentSampleRate())+" Hz / "+juce::String(device->getCurrentBufferSizeSamples())+" samples\n";
    else report+="No active audio device\n";
    report+="MIDI render delay: "+juce::String(shown.midiDelayMs,3)+" ms; late="+juce::String(shown.lateMidi)+"; discarded="+juce::String(shown.discardedMidi)+"; resets="+juce::String(shown.midiResets)+"\n";
    for(const auto& device:juce::MidiInput::getAvailableDevices())report+="MIDI: "+device.name+"\n";
    directory.getChildFile("smoke.txt").replaceWithText(report);
    // Capture both views from the same state for visual regression inspection.
    const bool previousView=performanceView;setPerformanceView(false);
    if(auto stream=directory.getChildFile("sound.png").createOutputStream()){
        if(stream->setPosition(0)&&stream->truncate().wasOk())png.writeImageToStream(createComponentSnapshot(getLocalBounds()),*stream);
    }
    setPerformanceView(previousView);
}
}
