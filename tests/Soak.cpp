#include "core/Engine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace undertone;
int main(int argc,char** argv){
    const int seconds=argc>1?std::clamp(std::atoi(argv[1]),1,7200):60;
    Engine e;e.prepare(48000);Command c;c.action=Action::load;c.session=makePreset(7);
    c.session.patch.model=ToneModel(argc>2?std::clamp(std::atoi(argv[2]),0,3):1);
    c.session.music.rhythm=Rhythm::eighth;c.session.music.bars=1;c.session.music.evolveTarget=0;
    c.session.bpm=200;c.session.music.complexity=2;c.session.music.evolve=true;c.session.music.variation=1;
    c.session.patch.attack=.005f;c.session.patch.release=8;c.session.patch.space=1;c.session.patch.chorus=1;c.session.patch.drive=1;
    c.session.patch.resonance=1;c.session.patch.texture=1;c.session.patch.brightness=1;
    if(argc>3){c.session.detail=makePreset(8).detail;c.session.detail.bed=c.session.detail.broken=c.session.detail.motif=c.session.detail.wear=c.session.detail.echo=1;c.session.detail.breathe=false;}
    if(argc>4)c.session.percussion={true,1,1,1,1,42};
    if(argc>5){c.session.music.composition=Composition::uneasy;c.session.detail.surface=1;c.session.detail.voice=MelodyVoice::bowedWire;}
    if(argc>6)c.session.cycles={true,true,1,2};
    c.session.phrase=generate(c.session.music,c.session.phrase,c.session.seed);e.command(c);c.action=Action::play;e.command(c);
    const int blocks=seconds*48000/256;std::vector<double> durations;durations.reserve(size_t(blocks));
    float l[256],r[256],peak=0;double energy=0;const auto start=std::chrono::steady_clock::now();
    for(int block=0;block<blocks;++block){auto before=std::chrono::steady_clock::now();e.process(l,r,256);auto after=std::chrono::steady_clock::now();
        durations.push_back(std::chrono::duration<double>(after-before).count());
        for(int n=0;n<256;++n){if(!std::isfinite(l[n])||!std::isfinite(r[n]))return 2;peak=std::max(peak,std::max(std::abs(l[n]),std::abs(r[n])));energy+=l[n]*l[n]+r[n]*r[n];}
        if(block%1000==0&&!valid(e.session()))return 3;
    }
    const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::sort(durations.begin(),durations.end());
    std::cout<<"Offline stress: "<<seconds<<" audio seconds, "<<blocks<<" blocks, 48 kHz / 256 samples\n"
        <<"Wall time: "<<elapsed<<"s; callback processing p99: "<<durations[size_t(durations.size()*.99)]*1000<<"ms ("
        <<durations[size_t(durations.size()*.99)]/(256.0/48000)*100<<"% of deadline)\n"
        <<"Maximum block: "<<durations.back()*1000<<"ms\n"
        <<"Peak: "<<peak<<"; RMS: "<<std::sqrt(energy/(double(blocks)*512))<<"; chord triggers: "<<e.status().notesTriggered<<"\n"
        <<"Composition switches: "<<e.status().compositionChanges<<"; motif changes: "<<e.status().motifChanges<<"\n"
        <<"This is accelerated engine processing, not a real-time device stability or underrun test.\n";
}
