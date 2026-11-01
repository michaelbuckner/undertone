#include "MainComponent.h"
#include "Design.h"
namespace undertone {
using namespace design;
juce::Font MainComponent::Theme::getTextButtonFont(juce::TextButton& b,int height){return font(height<25?11.f:13.f,b.getProperties()["primary"]);}
void MainComponent::Theme::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& colour,bool over,bool down){
    const bool selected=b.getToggleState(),primary=b.getProperties()["primary"],tab=b.getProperties()["tab"];
    auto bounds=b.getLocalBounds().toFloat().reduced(.5f);
    const bool recording=selected&&b.getProperties()["icon"].toString()=="record";
    auto fill=recording?danger.withAlpha(.14f):selected?(tab?raised:mint.withAlpha(.16f)):colour;
    if(over)fill=fill.brighter(.10f);if(down)fill=fill.darker(.12f);
    g.setColour(b.isEnabled()?fill:panel);g.fillRoundedRectangle(bounds,5);
    g.setColour(!b.isEnabled()?line.withAlpha(.5f):recording?danger:selected?(tab?accent:mint):primary?accent:over?muted:line);
    g.drawRoundedRectangle(bounds,5,b.hasKeyboardFocus(true)?2.f:1.f);
    if(tab&&selected){g.setColour(accent);g.fillRoundedRectangle(12.f,float(b.getHeight()-3),float(b.getWidth()-24),2.f,1.f);}
}
void MainComponent::Theme::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool){
    const bool primary=b.getProperties()["primary"];
    g.setColour(!b.isEnabled()?muted.withAlpha(.45f):primary?background:b.getToggleState()?ink:b.findColour(juce::TextButton::textColourOffId));
    g.setFont(getTextButtonFont(b,b.getHeight()));auto bounds=b.getLocalBounds().reduced(6,0);
    const auto icon=b.getProperties()["icon"].toString();
    if(icon.isNotEmpty()){
        const float x=12,y=b.getHeight()*.5f;juce::Path p;
        if(icon=="play"){p.addTriangle(x,y-4,x,y+4,x+7,y);g.fillPath(p);}
        else if(icon=="record"){g.setColour(b.getToggleState()?danger:muted);g.fillEllipse(x,y-3,6,6);}
        else if(icon=="stop")g.fillRect(x,y-3,6.f,6.f);
        bounds.removeFromLeft(13);
    }
    g.drawFittedText(b.getButtonText(),bounds,juce::Justification::centred,1);
}
void MainComponent::Theme::drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool over,bool){
    const bool on=b.getToggleState();const float y=(b.getHeight()-14)*.5f;
    g.setColour(on?mint:raised);g.fillRoundedRectangle(1,y,25,14,7);
    g.setColour(on?background:muted);g.fillEllipse(on?14.f:4.f,y+3,8,8);
    if(b.hasKeyboardFocus(true)){g.setColour(accent);g.drawRoundedRectangle(.5f,y-2,29,18,8,1.5f);}
    g.setFont(font(12));g.setColour(!b.isEnabled()?muted.withAlpha(.45f):over?ink:muted);
    g.drawFittedText(b.getButtonText(),34,0,b.getWidth()-35,b.getHeight(),juce::Justification::centredLeft,1);
}
void MainComponent::Theme::drawComboBox(juce::Graphics& g,int w,int h,bool over,int,int,int,int,juce::ComboBox& box){
    g.setColour(background);g.fillRoundedRectangle(.5f,.5f,float(w-1),float(h-1),4);
    g.setColour(box.hasKeyboardFocus(true)?accent:over?muted:line);g.drawRoundedRectangle(.5f,.5f,float(w-1),float(h-1),4,1);
    juce::Path p;const float x=float(w-17),y=h*.5f;p.startNewSubPath(x-3,y-1);p.lineTo(x,y+2);p.lineTo(x+3,y-1);
    g.setColour(box.isEnabled()?muted:muted.withAlpha(.4f));g.strokePath(p,juce::PathStrokeType(1.5f));
}
juce::Font MainComponent::Theme::getComboBoxFont(juce::ComboBox&){return font(13);}
void MainComponent::Theme::positionComboBoxText(juce::ComboBox& box,juce::Label& label){label.setBounds(9,1,box.getWidth()-32,box.getHeight()-2);label.setFont(font(13));}
void MainComponent::Theme::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float position,float,float,juce::Slider::SliderStyle style,juce::Slider& slider){
    if(style!=juce::Slider::LinearHorizontal){juce::LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,position,0,0,style,slider);return;}
    const float cy=y+h*.5f;g.setColour(line);g.fillRoundedRectangle(float(x),cy-2,float(w),4,2);
    const auto colour=slider.findColour(juce::Slider::trackColourId);g.setColour(slider.isEnabled()?colour:muted.withAlpha(.3f));g.fillRoundedRectangle(float(x),cy-2,std::max(0.f,position-x),4,2);
    for(int i=0;i<5;++i){g.setColour(line);g.fillRect(x+w*i/4, int(cy+8),1,2);}
    g.setColour(background);g.fillRoundedRectangle(position-4,cy-8,8,16,2);
    g.setColour(slider.hasKeyboardFocus(true)?ink:colour);g.drawRoundedRectangle(position-4,cy-8,8,16,2,1.5f);
}
void MainComponent::Theme::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float value,float start,float end,juce::Slider& slider){
    const float radius=std::min(w,h)*.37f,cx=x+w*.5f,cy=y+h*.5f,angle=start+value*(end-start);
    for(int i=0;i<=10;++i){const auto a=start+i*(end-start)/10;g.setColour(i<=int(value*10)?accent.withAlpha(.7f):line);
        g.drawLine(cx+std::sin(a)*(radius+6),cy-std::cos(a)*(radius+6),cx+std::sin(a)*(radius+9),cy-std::cos(a)*(radius+9),1);}
    juce::Path track,amount;track.addCentredArc(cx,cy,radius,radius,0,start,end,true);amount.addCentredArc(cx,cy,radius,radius,0,start,angle,true);
    g.setColour(line);g.strokePath(track,juce::PathStrokeType(2));g.setColour(accent);g.strokePath(amount,juce::PathStrokeType(2));
    g.setGradientFill(juce::ColourGradient(raised.brighter(.08f),cx,cy-radius,background,cx,cy+radius,false));g.fillEllipse(cx-radius+5,cy-radius+5,2*radius-10,2*radius-10);
    g.setColour(slider.hasKeyboardFocus(true)?accent:line);g.drawEllipse(cx-radius+5,cy-radius+5,2*radius-10,2*radius-10,1);
    g.setColour(ink);g.drawLine(cx+std::sin(angle)*(radius*.50f),cy-std::cos(angle)*(radius*.50f),cx+std::sin(angle)*(radius-10),cy-std::cos(angle)*(radius-10),2);
}
}
