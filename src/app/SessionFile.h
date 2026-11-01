#pragma once
#include "core/Music.h"
#include <juce_core/juce_core.h>
namespace undertone {
juce::String encodeSession(const Session&);
juce::Result decodeSession(const juce::String&, Session&);
juce::Result saveSession(const juce::File&, const Session&);
juce::Result loadSession(const juce::File&, Session&);
}
