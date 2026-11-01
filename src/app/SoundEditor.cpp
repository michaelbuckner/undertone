#include "MainComponent.h"
#include "Design.h"

namespace undertone {
using namespace design;
void MainComponent::selectSoundPage(int index){
    soundPage=std::clamp(index,0,5);refreshSoundEditor();resized();repaint();
}
void MainComponent::refreshSoundEditor(){
    const auto model=editing.patch.model;
    const char* texture=model==ToneModel::ladder?"Oscillator weight":model==ToneModel::dco?"Pulse character":model==ToneModel::fm?"FM depth":"Texture";
    advancedLabels[5].setText(texture,juce::dontSendNotification);advanced[5].setTitle(texture);
    for(size_t i=0;i<advanced.size();++i)advancedLabels[i].setEnabled(advanced[i].isEnabled());
    soundPageTitle.setText(soundPages[soundPage].getButtonText(),juce::dontSendNotification);
    juce::String hint;
    switch(soundPage){
        case 0: hint=model==ToneModel::warm?"Warm blends detuned oscillators. Texture and resonance are not used.":
            model==ToneModel::ladder?"Shape oscillator weight and filter resonance; detune spreads the three oscillators.":
            model==ToneModel::dco?"Pulse character controls the pulse blend and width movement. Detune is not used.":
            "FM depth controls modulation intensity. Resonance and detune are not used.";break;
        case 1: hint=model==ToneModel::warm?"Warm uses Attack and Release. Decay, Sustain and Filter envelope are not used.":
            model==ToneModel::fm?"Shape the pad's attack, decay, sustain and release. FM does not use Filter envelope.":
            "Shape the pad's attack, decay, sustain and release; add movement with Filter envelope.";break;
        case 2: hint="Drive adds saturation; Chorus widens; Space adds reverb. Bass cut leaves room for your bass.";break;
        case 3: hint="Balance the pitched layers and texture. Melody voice changes timbre without changing notes.";break;
        case 4: hint="A steady pulse with generated offbeats. Enable Percussion above to hear these controls.";break;
        case 5: hint="Shape the progression and evolution, then choose Internal clock or an incoming MIDI source.";break;
    }
    soundPageHint.setText(hint,juce::dontSendNotification);
}
void MainComponent::layoutSoundEditor(){
    const int w=getWidth(),left=220,width=w-left-34;
    auto place=[](juce::Component& c,int x,int y,int width,int height){c.setVisible(true);c.setBounds(x,y,width,height);};
    auto field=[&](juce::Component& c,juce::Label& label,int x,int y,int width){place(label,x,y-20,width,18);place(c,x,y,width,30);};
    auto dial=[&](juce::Slider& control,juce::Label& label,int x,int y,int width,int height=136){
        control.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);control.setTextBoxStyle(juce::Slider::TextBoxBelow,false,std::min(width-8,110),24);
        place(label,x,y,width,20);label.setJustificationType(juce::Justification::centred);place(control,x,y+22,width,height-22);
    };
    auto fader=[&](juce::Slider& control,juce::Label& label,int x,int y,int width){
        control.setSliderStyle(juce::Slider::LinearHorizontal);control.setTextBoxStyle(juce::Slider::TextBoxRight,false,52,24);
        label.setJustificationType(juce::Justification::centredLeft);place(label,x,y,width,18);place(control,x,y+20,width,30);
    };
    for(int i=0;i<6;++i){place(soundPages[i],32,446+i*44,150,34);soundPages[i].setToggleState(i==soundPage,juce::dontSendNotification);}
    field(toneModel,soundLabels[0],left,428,260);
    field(tonePreset,soundLabels[1],left+276,428,width-276);
    place(soundPageTitle,left,471,width,27);place(soundPageHint,left,501,width,29);
    const int cell=width/5;
    if(soundPage==0){
        dial(knobs[4],knobLabels[4],left,540,cell);dial(knobs[5],knobLabels[5],left+cell,540,cell);
        dial(advanced[5],advancedLabels[5],left+cell*2,540,cell);dial(advanced[6],advancedLabels[6],left+cell*3,540,cell);
        dial(advanced[4],advancedLabels[4],left+cell*4,540,cell);
    }else if(soundPage==1){
        const int order[]{0,7,8,1,9};for(int i=0;i<5;++i)dial(advanced[order[i]],advancedLabels[order[i]],left+i*cell,540,cell);
    }else if(soundPage==2){
        const int size=width/4;dial(advanced[2],advancedLabels[2],left,540,size);dial(advanced[3],advancedLabels[3],left+size,540,size);
        dial(knobs[6],knobLabels[6],left+size*2,540,size);dial(knobs[3],knobLabels[3],left+size*3,540,size);
    }else if(soundPage==3){
        field(melodyVoice,melodyVoiceLabel,left,551,280);place(breathe,left+310,551,260,30);
        place(newMotif,left+width-244,551,114,30);place(newFigure,left+width-122,551,122,30);
        const int size=width/3;for(int i=0;i<6;++i)fader(detailControls[i],detailLabels[i],left+(i%3)*size,596+(i/3)*58,size-18);
        place(formLabel,left,713,width,18);
    }else if(soundPage==4){
        const int size=width/4;for(int i=0;i<4;++i)dial(percussionControls[i],percussionLabels[i],left+i*size,540,size);
        place(newBeat,left,696,136,30);place(percussionHint,left+154,692,width-154,38);
    }else{
        const int small=137,right=left+438,col=(width-454)/2;
        for(int i=0;i<3;++i)dial(knobs[i],knobLabels[i],left+i*small,544,small,132);
        field(slots,labels[3],right,548,col);field(bars,labels[4],right+col+16,548,col);
        field(rhythm,labels[5],right,607,col);field(composition,compositionLabel,right+col+16,607,col);
        field(evolveTarget,soundLabels[2],right,666,col);field(evolvePace,soundLabels[3],right+col+16,666,col);
        field(clockSource,labels[6],left,708,284);place(refreshMidi,left+296,708,122,30);
        place(clockHint,right,702,width-438,38);
    }
    field(tempo,labels[7],28,752,300);place(audio,358,752,155,30);place(recover,525,752,110,30);
    place(performanceLevelLabel,w-388,752,110,30);place(performanceLevel,w-274,752,240,30);
}
}
