#include "MidiExport.h"
namespace undertone {
juce::Result exportMidiFile(const juce::File& file,const Session& state,int bars){
    if(!valid(state)||bars<1||bars>128)return juce::Result::fail("Invalid session or export length.");
    constexpr double rate=48000,ppq=960;
    struct Capture {std::array<juce::MidiMessageSequence,4> tracks;double ticksPerSample=0;};
    Capture capture;capture.ticksPerSample=state.bpm*ppq/(60*rate);
    const char* names[]{"Pad","Broken figure","Melody","Percussion"};
    for(int i=0;i<4;++i)capture.tracks[i].addEvent(juce::MidiMessage::textMetaEvent(3,names[i]));
    auto engine=std::make_unique<Engine>();engine->prepare(rate);
    Command c;c.action=Action::load;c.session=state;engine->command(c);
    engine->setNoteSink([](void* data,const Engine::NoteEvent& event){auto& out=*static_cast<Capture*>(data);
        auto message=event.on?juce::MidiMessage::noteOn(event.channel,event.note,juce::uint8(event.velocity)):juce::MidiMessage::noteOff(event.channel,event.note);
        message.setTimeStamp(std::round(event.sample*out.ticksPerSample));out.tracks[event.channel==10?3:event.channel-1].addEvent(message);
    },&capture);
    c.action=Action::play;engine->command(c);
    int64_t remaining=int64_t(std::llround(bars*4*60*rate/state.bpm));
    while(remaining>0){const int count=int(std::min(int64_t(256),remaining));engine->process(nullptr,nullptr,count);remaining-=count;}
    c.action=Action::stop;engine->command(c);
    juce::MidiFile midi;midi.setTicksPerQuarterNote(int(ppq));juce::MidiMessageSequence tempo;
    tempo.addEvent(juce::MidiMessage::tempoMetaEvent(int(std::llround(60000000/state.bpm))));tempo.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4,4));midi.addTrack(tempo);
    for(auto& track:capture.tracks){track.updateMatchedPairs();auto end=juce::MidiMessage::endOfTrack();end.setTimeStamp(bars*4*ppq);track.addEvent(end);midi.addTrack(track);}
    if(file.getParentDirectory().createDirectory().failed())return juce::Result::fail("Cannot create export directory.");
    juce::TemporaryFile temporary(file);auto stream=temporary.getFile().createOutputStream();
    if(!stream||!midi.writeTo(*stream))return juce::Result::fail("Cannot write MIDI file.");
    stream->flush();if(stream->getStatus().failed())return stream->getStatus();stream.reset();
    if(!temporary.overwriteTargetFileWithTemporary())return juce::Result::fail("Cannot replace MIDI file.");
    return juce::Result::ok();
}
}
