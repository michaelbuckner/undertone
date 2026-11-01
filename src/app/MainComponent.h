#pragma once
#include "core/Engine.h"
#include "core/MidiTimeline.h"
#include "SessionFile.h"
#include "SessionBrowser.h"
#include <juce_audio_utils/juce_audio_utils.h>

namespace undertone {
class MainComponent final : public juce::Component, private juce::AudioIODeviceCallback,
    private juce::MidiInputCallback, private juce::Timer {
public:
    MainComponent();
    ~MainComponent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    void smokeTest(const juce::File& directory);
    int preferredHeight() const noexcept {return 850;}
private:
    struct Theme : juce::LookAndFeel_V4 {
        Theme();
        juce::Font getTextButtonFont(juce::TextButton&,int) override;
        void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
        void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
        void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
        void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
        juce::Font getComboBoxFont(juce::ComboBox&) override;
        void positionComboBoxText(juce::ComboBox&,juce::Label&) override;
        void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;
        void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    } theme;
    juce::TooltipWindow tooltips{this,650};
    struct ChordButton : juce::TextButton {
        juce::String name, preview, voicing;
        int number=1;
        bool active=false, locked=false,resting=false;
        float progress=0;
        void paintButton(juce::Graphics&,bool,bool) override;
    };
    juce::AudioDeviceManager devices;
    Engine engine;
    Queue<Command,128> commands;
    Queue<Status,64> statuses;
    MidiTimeline midiTimeline;
    std::atomic<uint32_t> midiEpoch{0};
    std::atomic<bool> audioActive{false};
    Status shown;
    Session editing;
    int reportSamples=0, lastRejected=0, recoveryTicks=0;
    uint64_t sentSequence=0, syncSequence=0;
    uint64_t seenMidiResets=0;
    uint64_t seenRevision=0;
    bool performanceView=true;
    bool browserView=false,replacePromptOpen=false,fileDialogOpen=false;
    SessionDocument document;
    std::unique_ptr<SessionLibrary> library;
    std::unique_ptr<SessionBrowser> browser;
    juce::TextButton browserTab{"Library"};
    juce::Label documentLabel;
    int soundPage=0;
    std::array<juce::TextButton,6> soundPages;
    juce::Label soundPageTitle,soundPageHint;
    bool refreshing=false, recoveryArmed=false;
    juce::String notification, selectedMidi;
    juce::CriticalSection errorLock;
    juce::String deviceError;
    juce::File recoveryFile, audioSettingsFile;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Component::SafePointer<juce::DialogWindow> audioWindow;
    juce::TextButton play{"Play / restart"}, stop{"Stop"}, panic{"Panic"}, generateButton{"Generate"},
        lockAll{"Lock phrase"}, undo{"Undo"}, save{"Save"}, load{"Open"}, audio{"Audio settings"}, refreshMidi{"Refresh MIDI"}, recover{"Recover"};
    juce::ToggleButton evolve{"Evolve"}, rootless{"Omit root"}, wide{"Open voicing"};
    juce::ComboBox key, mode, character, slots, bars, rhythm, clockSource, toneModel, tonePreset, evolveTarget, evolvePace;
    std::array<juce::Label,4> soundLabels;
    juce::Label evolutionHint;
    std::array<juce::Slider,6> detailControls;
    std::array<juce::Label,6> detailLabels;
    juce::ComboBox composition;
    juce::Label compositionLabel,formLabel;
    juce::TextButton performanceTab{"Performance"},soundTab{"Sound"},varyMotif{"Vary motif"},newFigure{"New figure"};
    juce::ComboBox motifMode,swapStyle,sceneInterval,exportBars;
    juce::ToggleButton phrasing{"Statement / answer phrasing"},sceneCycle{"Alternate A / B"},sceneVariations{"Vary on return"};
    juce::TextButton storeA{"Store A"},storeB{"Store B"},recallA{"Recall A"},recallB{"Recall B"};
    juce::Label sceneHeading,motifModeLabel,roomLabel,performanceLevelLabel,captureHint;
    juce::Slider room,performanceLevel;
    juce::TextButton keepMoment{"Keep this moment"},recordAudio{"Record WAV"},exportMidi{"Export MIDI"};
    juce::TimeSliceThread recordingThread{"Undertone recording"};
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> recordingWriter;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter{nullptr};
    std::atomic<bool> recordingOverflow{false};
    std::atomic<bool> recordingChanged{false};
    std::atomic<int64_t> recordedFrames{0};
    juce::File recordingFile;
    double recordingRate=0;
    juce::ToggleButton compositionCycle{"Alternate Still / Uneasy"},motifCycle{"Generate new motifs"};
    juce::ComboBox compositionInterval,motifInterval;
    juce::Label cycleHeading,cycleHint,compositionIntervalLabel,motifIntervalLabel;
    juce::Label detailHeading, detailHint;
    juce::ToggleButton breathe{"Let layers come and go"};
    juce::TextButton newMotif{"New motif"};
    juce::ComboBox melodyVoice;
    juce::Label melodyVoiceLabel;
    juce::ToggleButton percussionEnabled{"Percussion"};
    juce::TextButton newBeat{"New pattern"};
    std::array<juce::Slider,4> percussionControls;
    std::array<juce::Label,4> percussionLabels;
    juce::Label percussionHeading,percussionHint;
    juce::Slider tempo;
    std::array<ChordButton,8> chordButtons;
    std::array<juce::TextButton,8> swapButtons,restButtons;
    std::array<juce::TextButton,8> compareButtons;
    std::array<juce::Slider,8> knobs;
    std::array<juce::Label,8> knobLabels;
    std::array<juce::Slider,10> advanced;
    std::array<juce::Label,10> advancedLabels;
    std::array<juce::Label,8> labels;
    juce::Label transportLabel, messageLabel, clockHint;
    bool send(Action,int value=0);
    void dispatch(const Command&);
    void changeMusic();
    void changePatch();
    void changeEvolution();
    void changeDetail();
    void changePercussion();
    void changeCycles();
    void changeScenes();
    void setPerformanceView(bool);
    void showBrowser();
    bool hasSessionEdits() const;
    void refreshDocumentLabel();
    void requestReplace(std::function<void()>);
    void loadFactoryPreset(int);
    void loadLibraryFile(const juce::File&,bool recovery=false);
    void applyLibrarySession(const Session&,const juce::String&,const juce::File&);
    void addLibraryFolder();
    void saveCurrentSession();
    bool saveDocument(const juce::File&,const Session&);
    void selectSoundPage(int);
    void layoutSoundEditor();
    void refreshSoundEditor();
    void keepCurrentMoment();
    void toggleRecording();
    void stopRecording();
    void chooseMidiExport();
    void showSession(const Session&);
    void updateMidiList();
    void chooseMidi();
    void showAudioSettings();
    void openSession();
    void chooseSave(std::function<void()> afterSave={});
    void timerCallback() override;
    void audioDeviceIOCallbackWithContext(const float* const*,int,float* const*,int,int,const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    void audioDeviceStopped() override;
    void audioDeviceError(const juce::String&) override;
    void handleIncomingMidiMessage(juce::MidiInput*,const juce::MidiMessage&) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
}
