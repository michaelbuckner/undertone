#include "core/Engine.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
using namespace undertone;
void u16(std::ofstream& f,uint16_t v){const char b[]{char(v),char(v>>8)};f.write(b,2);}
void u32(std::ofstream& f,uint32_t v){u16(f,uint16_t(v));u16(f,uint16_t(v>>16));}
int main(int argc,char** argv){
    if(argc<2)return 1;const int preset=argc>2?std::clamp(std::atoi(argv[2]),0,presetCount-1):8;
    const bool plain=argc>3&&std::atoi(argv[3])!=0;
    const int rate=48000,tail=preset>=10?8:4;
    const double seconds=argc>7?std::clamp(std::atof(argv[7]),4.,300.):plain?24:48;
    const int playFrames=int(std::llround(seconds*rate)),frames=playFrames+tail*rate;
    Engine engine;engine.prepare(rate);Command c;c.action=Action::load;c.session=makePreset(preset);
    if(argc>4){const int percussion=std::atoi(argv[4]);c.session.percussion.enabled=percussion!=0;if(percussion==2)c.session.detail={0,0,0,0,0,false,1};}
    if(argc>5)c.session.detail.voice=MelodyVoice(std::clamp(std::atoi(argv[5]),0,5));
    if(argc>8)c.session.cycles={true,true,8,4};
    // Optional reproducible v0.8 comparisons: 0=statement, 1=variation, 2=new, 3=A/B.
    if(argc>9){const int study=std::atoi(argv[9]);c.session.cycles={};c.session.music.evolve=false;c.session.detail.breathe=false;
        c.session.percussion.level=1.3f;
        if(study==1)randomNext(c.session.detail.variationSeed);
        if(study==2)advanceMotif(c.session.detail,true);
        if(study==3){storeScene(c.session,0);auto other=makePreset(11);static_cast<MusicalState&>(c.session)=other;storeScene(c.session,1);recallScene(c.session,0);c.session.sceneCycle=true;c.session.sceneBars=8;}}
    if(plain)c.session.detail={};engine.command(c);c.action=Action::play;engine.command(c);
    std::vector<float> l(size_t(frames),0),r=l;float peak=0;double energy=0;
    for(int at=0;at<frames;){if(at==playFrames){c.action=Action::stop;engine.command(c);}
        int n=std::min(256,frames-at);if(at<playFrames)n=std::min(n,playFrames-at);
        engine.process(l.data()+at,r.data()+at,n);at+=n;}
    for(int i=0;i<frames;++i){const float fade=float(std::min({1.0,i/(rate*.02),(frames-i)/(rate*.8)}));l[i]*=fade;r[i]*=fade;
        if(!std::isfinite(l[i])||!std::isfinite(r[i]))return 2;
        peak=std::max({peak,std::abs(l[i]),std::abs(r[i])});energy+=l[i]*l[i]+r[i]*r[i];}
    // Preview-only gain: both auditions use the same RMS target, with ample peak headroom.
    const double rms=std::sqrt(energy/(frames*2));const double target=argc>6?std::clamp(std::atof(argv[6]),.1,20.):.10/std::max(rms,1e-9);
    const double gain=std::min(target,.85/std::max(double(peak),1e-9));
    std::ofstream f(argv[1],std::ios::binary);if(!f)return 3;
    f.write("RIFF",4);u32(f,36+frames*4);f.write("WAVEfmt ",8);u32(f,16);u16(f,1);u16(f,2);u32(f,rate);u32(f,rate*4);u16(f,4);u16(f,16);f.write("data",4);u32(f,frames*4);
    for(int i=0;i<frames;++i){u16(f,uint16_t(int16_t(l[i]*gain*32767)));u16(f,uint16_t(int16_t(r[i]*gain*32767)));}
    std::cout<<preset<<(plain?" plain":" layered")<<": "<<seconds+tail<<"s; raw RMS "<<rms<<"; preview gain "<<gain<<"; peak "<<peak*gain<<"; layer notes "<<engine.status().detailNotes<<"; percussion hits "<<engine.status().percussionHits<<'\n';
}
