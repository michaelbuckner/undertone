#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace undertone::design {
inline const juce::Colour background{0xff111416}, panel{0xff1a1e21}, raised{0xff252b2e},
    ink{0xfff0eee8}, muted{0xffa6afb3}, line{0xff363e42}, accent{0xffe9b477},
    orange{accent}, mint{0xff91c7bb}, danger{0xffed8b82};
inline juce::Font font(float size,bool bold=false){
    return juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(),size,
        bold?juce::Font::bold:juce::Font::plain));
}
}
