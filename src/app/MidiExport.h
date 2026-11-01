#pragma once
#include "core/Engine.h"
#include <juce_audio_basics/juce_audio_basics.h>
namespace undertone {
juce::Result exportMidiFile(const juce::File&,const Session&,int bars);
}
