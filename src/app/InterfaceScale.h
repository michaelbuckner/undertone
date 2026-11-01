#pragma once
#include <algorithm>
#include <cmath>

namespace undertone {
// Display sizes are JUCE logical pixels: the OS has already applied DPI scaling.
inline float interfaceScale(int choice,int displayWidth,int displayHeight) noexcept {
    if(choice>=1&&choice<=5)return 1.f+.25f*(choice-1);
    const float fit=std::min(displayWidth*.88f/1140.f,displayHeight*.92f/850.f);
    return std::clamp(std::floor(fit*4)/4,1.f,2.f);
}
}
