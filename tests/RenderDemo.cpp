#include "core/Engine.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <chrono>
using namespace undertone;
void write16(std::ofstream& f,uint16_t v){char b[]{char(v),char(v>>8)};f.write(b,2);}
void write32(std::ofstream& f,uint32_t v){write16(f,uint16_t(v));write16(f,uint16_t(v>>16));}
int main(int argc,char** argv) {
    const char* path=argc>1?argv[1]:"undertone-demo.wav";
    const int preset=argc>2?std::clamp(std::atoi(argv[2]),0,presetCount-1):2;
    const int seconds=argc>3?std::clamp(std::atoi(argv[3]),1,3600):16;
    constexpr int rate=48000;const int frames=rate*seconds;const uint32_t bytes=uint32_t(frames)*4;
    std::ofstream out(path,std::ios::binary);if(!out)return 2;
    out.write("RIFF",4);write32(out,36+bytes);out.write("WAVEfmt ",8);write32(out,16);write16(out,1);write16(out,2);
    write32(out,rate);write32(out,rate*4);write16(out,4);write16(out,16);out.write("data",4);write32(out,bytes);
    Engine e;e.prepare(rate);Command c;c.action=Action::load;c.session=makePreset(preset);
    if(argc>4)c.session.patch=makeTone(std::clamp(std::atoi(argv[4]),0,8));
    if(argc>5){c.session.music.evolve=std::atoi(argv[5])!=0;c.session.music.variation=.65f;c.session.music.evolveBars=2;}
    e.command(c);c.action=Action::play;e.command(c);
    float l[256],r[256],peak=0;double energy=0; const auto start=std::chrono::steady_clock::now();
    for(int at=0;at<frames;at+=256) {int n=std::min(256,frames-at);e.process(l,r,n);for(int i=0;i<n;++i){
        if(!std::isfinite(l[i])||!std::isfinite(r[i]))return 3;
        peak=std::max(peak,std::max(std::abs(l[i]),std::abs(r[i])));energy+=l[i]*l[i]+r[i]*r[i];
        write16(out,uint16_t(int16_t(std::clamp(l[i],-1.f,1.f)*32767)));write16(out,uint16_t(int16_t(std::clamp(r[i],-1.f,1.f)*32767)));
    }}
    std::cout<<"Rendered "<<seconds<<"s; peak="<<peak<<" RMS="<<std::sqrt(energy/(frames*2))
        <<" elapsed="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"s\n";
}
