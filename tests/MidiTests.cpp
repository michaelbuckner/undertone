#include "core/MidiTimeline.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

using namespace undertone;
namespace {
int checks=0;
void check(bool ok,const char* message){++checks;if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
struct Event {TimedMidi message;bool onset=false;};
struct Result {std::vector<double> errors;Status status;};

Result run(const std::vector<Event>& events,int rate,int blockSize,bool noisy=false,bool variable=false){
    Engine engine;engine.prepare(rate);Command c;c.action=Action::load;c.session=makePreset(2);
    c.session.music.rhythm=Rhythm::quarter;engine.command(c); // Explicit timing fixture, independent of factory sound design.
    c.action=Action::external;c.value=1;engine.command(c);
    MidiTimeline timeline;timeline.prepare(rate,blockSize);
    std::vector<double> expected;for(const auto& e:events)if(e.onset)expected.push_back(e.message.seconds);
    float left[2048],right[2048];size_t delivered=0,observed=0;int64_t sample=0;int block=0;
    const double drift=noisy?80e-6:0,delay=2.0*blockSize/rate;
    const double end=events.back().message.seconds+delay+.05;
    Result result;
    while(10.0+double(sample)/rate*(1+drift)<end){
        const int sizes[]{64,127,256,511};const int count=variable?sizes[block%4]:blockSize;
        const double callback=10.0+double(sample)/rate*(1+drift)+(noisy?.0008*std::sin(block*.73):0);
        // Timestamp source and callback delivery are independent: late delivery
        // must retain the original event timestamp rather than retiming to receipt.
        while(delivered<events.size()&&events[delivered].message.seconds+(noisy?.0005:0)<=callback){
            check(timeline.push(events[delivered].message),"normal stream queue capacity");++delivered;
        }
        timeline.process(engine,left,right,count,callback);
        const auto state=timeline.status(engine);
        if(state.notesTriggered!=observed){
            check(state.notesTriggered==observed+1,"no duplicate/missing onsets within one block");
            check(observed<expected.size(),"no unexpected onset");
            const double outputTime=10.0+double(state.lastOnsetSample)/rate*(1+drift);
            result.errors.push_back(outputTime-expected[observed]-delay);
            ++observed;
        }
        sample+=count;++block;
    }
    check(observed==expected.size(),"all expected onsets rendered");
    result.status=timeline.status(engine);return result;
}
std::vector<Event> ticks(int rateBpm,int count,bool tempoChanges=false){
    std::vector<Event> events{{{0xfa,10.013,0},false}};
    double time=10.017;
    for(int tick=0;tick<count;++tick){
        const double bpm=tempoChanges?(tick<count/3?120:tick<2*count/3?90:140):rateBpm;
        events.push_back({{0xf8,time,0},tick%24==0});time+=60.0/(bpm*24.0);
    }
    return events;
}
}
int main(){
    double maximum=0;
    for(int rate:{44100,48000,96000})for(int block:{64,256,1024})for(int bpm:{60,120,180}){
        auto result=run(ticks(bpm,192),rate,block);
        for(double error:result.errors){maximum=std::max(maximum,std::abs(error));check(std::abs(error)*rate<=1.01,"exact timestamp mapping across buffer sizes and sample rates");}
        check(result.status.lateMidi==0&&result.status.discardedMidi==0,"clean clock stream needs no corrections");
    }
    auto changed=run(ticks(120,576,true),48000,256);
    for(auto error:changed.errors)check(std::abs(error)*48000<=1.01,"120 to 90 to 140 BPM follows tick positions immediately");
    auto variable=run(ticks(120,192),48000,1024,false,true);
    for(auto error:variable.errors)check(std::abs(error)*48000<=1.01,"variable callback block lengths retain event placement");
    std::vector<Event> transport{{{0xfa,10.013,0},false}};
    for(int tick=0;tick<144;++tick){
        double time=10.017+tick/48.0;
        if(tick==48)transport.push_back({{0xfc,time-.001,0},false});
        if(tick==72)transport.push_back({{0xfb,time-.001,0},false});
        transport.push_back({{0xf8,time,0},tick<48?tick%24==0:tick>=72&&(tick-72)%24==0});
    }
    auto resumed=run(transport,48000,256);for(auto error:resumed.errors)check(std::abs(error)*48000<=1.01,"stop and continue retain phase without extra onset");

    Engine engine;engine.prepare(48000);Command c;c.action=Action::external;c.value=1;engine.command(c);
    MidiTimeline timeline;timeline.prepare(48000,256);float l[256],r[256];
    timeline.changeSource(2);
    timeline.push({0xfa,10.001,1});timeline.push({0xf8,10.002,1}); // Previous source must not start playback.
    timeline.process(engine,l,r,256,10.01);
    timeline.process(engine,l,r,256,10.01+256.0/48000);
    check(!engine.status().playing&&timeline.status(engine).discardedMidi==2,"source-generation isolation");
    timeline.push({0xfa,std::numeric_limits<double>::quiet_NaN(),2});
    timeline.push({0xfa,100.0,2});timeline.push({0xfa,1.0,2});
    timeline.process(engine,l,r,256,10.01+512.0/48000);
    check(!engine.status().playing&&timeline.status(engine).discardedMidi==5,"invalid, stale and implausible timestamps discarded");
    for(int i=0;i<1100;++i)timeline.push({0xf8,10.03,2});
    timeline.process(engine,l,r,256,10.01+768.0/48000);
    check(timeline.status(engine).midiResets==1&&!engine.status().playing,"overflow stops and clears queued clock");
    engine.midi(0xfb,10.03);check(!engine.status().playing,"overflow requires fresh Start");
    engine.midi(0xfa,10.03);engine.midi(0xf8,10.03);check(engine.status().playing,"fresh Start can recover");
    timeline.process(engine,l,r,256,12.0);
    check(timeline.status(engine).midiResets==2&&!engine.status().playing,"audio timeline discontinuity stops playback");
    engine.midi(0xfb,12.01);check(!engine.status().playing,"gap recovery requires fresh Start");

    // Twenty simulated minutes with a faster/slower device clock and callback
    // scheduling jitter. This is not a hardware or OS real-time measurement.
    auto longRun=run(ticks(120,57600),48000,256,true);
    std::vector<double> absolute;for(size_t i=4;i<longRun.errors.size();++i)absolute.push_back(std::abs(longRun.errors[i]));
    std::sort(absolute.begin(),absolute.end());const double p95=absolute[size_t(absolute.size()*.95)];
    std::cout<<"Drift diagnostics: late="<<longRun.status.lateMidi<<" discarded="<<longRun.status.discardedMidi<<" resets="<<longRun.status.midiResets<<" p95="<<p95*1000<<" ms\n";
    check(p95<.005,"simulated p95 onset error below 5 ms after fixed delay");
    check(longRun.status.lateMidi==0&&longRun.status.midiResets==0,"jitter/drift model has no late events or discontinuities");
    std::cout<<"PASS "<<checks<<" MIDI checks; exact-stream maximum error="<<maximum*1000<<" ms\n"
        <<"20 simulated minutes: p95="<<p95*1000<<" ms; maximum="<<absolute.back()*1000<<" ms; late="<<longRun.status.lateMidi<<"\n";
}
