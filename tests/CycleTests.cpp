#include "core/Engine.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace undertone;
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
void command(Engine& e,Action action,int value=0){Command c;c.action=action;c.value=value;e.command(c);}
void load(Engine& e,const Session& s){Command c;c.action=Action::load;c.session=s;e.command(c);}
void render(Engine& e,int frames){float l[256],r[256];while(frames>0){const int n=std::min(frames,256);e.process(l,r,n);for(int i=0;i<n;++i)check(std::isfinite(l[i])&&std::isfinite(r[i])&&std::abs(l[i])<=1&&std::abs(r[i])<=1,"finite bounded audio");frames-=n;}}
struct Clock {
    Engine e;int tick=0;
    explicit Clock(Session s){e.prepare(48000);load(e,s);command(e,Action::external,1);e.midi(0xfa,0);}
    void through(int target){while(tick<=target){e.midi(0xf8,tick/48.0);++tick;}}
};
int main(){
    // Local edits must be local across all characters, registers and slots.
    for(int preset=0;preset<presetCount;++preset){auto s=makePreset(preset);s.music.evolve=false;Engine e;load(e,s);
        for(int at=0;at<s.music.slots;++at){const auto before=e.session();command(e,Action::swapChord,at);const auto after=e.session();
            check(valid(after)&&!sameChord(before.phrase.chords[at],after.phrase.chords[at]),"swap always changes selected slot");
            for(int i=0;i<s.music.slots;++i)if(i!=at)check(sameChord(before.phrase.chords[i],after.phrase.chords[i]),"swap leaves other slots exact");
            command(e,Action::restChord,at);check(e.session().phrase.chords[at].rest,"rest set");
            command(e,Action::restChord,at);check(sameChord(after.phrase.chords[at],e.session().phrase.chords[at]),"restore exact stored chord");
            command(e,Action::restChord,at);command(e,Action::undo);check(sameChord(after.phrase.chords[at],e.session().phrase.chords[at]),"undo rest");
            command(e,Action::lock,at);auto locked=e.session().phrase.chords[at];auto rejected=e.status().rejected;
            command(e,Action::restChord,at);command(e,Action::swapChord,at);
            check(e.status().rejected==rejected+2&&sameChord(locked,e.session().phrase.chords[at]),"lock protects both actions");
            command(e,Action::lock,at);
        }
    }
    auto s=makePreset(10);s.music.evolve=false;s.cycles={true,true,2,1};s.phrase.chords[1].locked=true;s.phrase.chords[2].rest=true;
    const auto locked=s.phrase.chords[1],rest=s.phrase.chords[2];const auto initialSeed=s.detail.seed;
    Clock c(s);c.through(95);check(c.e.session().detail.seed==initialSeed,"motif does not change before bar");
    c.through(96);check(c.e.session().detail.seed==nextMotifSeed(initialSeed)&&c.e.session().music.composition==Composition::still,"independent motif interval");
    c.through(191);check(c.e.status().compositionChanges==0,"composition not early");
    c.through(192);check(c.e.session().music.composition==Composition::uneasy&&c.e.status().compositionChanges==1&&c.e.status().motifChanges==2,"simultaneous cycle boundary");
    check(sameChord(c.e.session().phrase.chords[1],locked)&&sameChord(c.e.session().phrase.chords[2],rest),"cycles preserve locked and resting voicings");
    for(int i=0;i<s.music.slots;++i)check(c.e.session().phrase.chords[i].degree==s.phrase.chords[i].degree,"composition keeps root progression");
    c.through(384);check(c.e.session().music.composition==Composition::still&&valid(c.e.session()),"composition alternates back");
    auto paused=c.e.status();c.e.midi(0xfc,9);c.through(480);
    check(c.e.status().motifChanges==paused.motifChanges&&c.e.status().compositionChanges==paused.compositionChanges,"stop pauses cycles despite arriving clocks");
    c.e.midi(0xfb,11);c.through(575);check(c.e.status().motifChanges==paused.motifChanges,"continue retains phase before next boundary");
    c.through(576);check(c.e.status().motifChanges==paused.motifChanges+1,"continue advances retained cycle phase");
    render(c.e,24001);check(c.e.status().clock==ClockState::lost&&!c.e.status().playing,"clock loss stops cycles");
    c.e.midi(0xfb,20);c.through(600);check(!c.e.status().playing,"continue cannot bypass lost-clock fresh-start requirement");
    c.e.midi(0xfa,21);c.e.midi(0xf8,21.01);check(c.e.status().motifChanges==0&&c.e.status().compositionChanges==0&&c.e.status().motifBeats==4,"start resets cycle grid");
    // Quick edits at the next bar inside a long chord, not at the 16-bar loop end.
    s.cycles={};Clock quick(s);quick.through(12);auto old=quick.e.session();command(quick.e,Action::restChord,0);quick.through(95);
    check(!quick.e.session().phrase.chords[0].rest&&quick.e.status().manualPending,"bar edit waits");quick.through(96);
    check(quick.e.session().phrase.chords[0].rest&&!quick.e.status().manualPending&&quick.e.status().slot==0,"bar edit commits within sustained chord");
    check(quick.e.status().beat==4,"quick edit preserves phrase phase");
    command(quick.e,Action::swapChord,0);quick.through(192);check(!quick.e.session().phrase.chords[0].rest,"swap reintroduces a resting slot");
    for(int i=1;i<s.music.slots;++i)check(sameChord(old.phrase.chords[i],quick.e.session().phrase.chords[i]),"playing quick edit retains other slots");
    // A queued whole phrase retains its boundary; new local edits merge into it.
    {auto held=makePreset(0);held.music.evolve=false;held.music.bars=4;Clock remote(held);remote.through(12);
        const auto onsets=remote.e.status().notesTriggered;command(remote.e,Action::swapChord,1);remote.through(96);
        check(remote.e.status().notesTriggered==onsets,"editing future slot does not retrigger current sustained chord");}
    Clock merge(s);merge.through(12);command(merge.e,Action::generate);command(merge.e,Action::restChord,0);merge.through(96);
    check(!merge.e.session().phrase.chords[0].rest&&merge.e.status().manualPending,"local edit cannot prematurely commit whole phrase");
    merge.through(1536);check(merge.e.session().phrase.chords[0].rest&&!merge.e.status().manualPending&&merge.e.status().beat==0,"merged edit commits at full phrase boundary");
    // Manual edits take priority over both automatic controls on a shared boundary.
    s.cycles={true,true,1,1};Clock manual(s);manual.through(12);command(manual.e,Action::swapChord,0);manual.through(96);
    check(manual.e.status().compositionChanges==0&&manual.e.status().motifChanges==0,"manual edit wins coincident cycle boundary");
    manual.through(192);check(manual.e.status().compositionChanges==1&&manual.e.status().motifChanges==1,"cycles resume next interval");
    // Song does not get silently converted by an enabled composition cycle.
    auto song=makePreset(8);song.music.evolve=false;song.cycles={true,true,1,1};Clock songs(song);songs.through(96);
    check(songs.e.session().music.composition==Composition::song&&songs.e.status().compositionChanges==0&&songs.e.status().motifChanges==1,"Song composition cycle pauses independently");
    // All-rest phrases and an all-rest future must have bounded planner work and no new pitched events.
    auto silent=s;for(auto& chord:silent.phrase.chords)chord.rest=true;
    for(int step=0;step<512;++step)check(detailStep(silent,step).count==0,"rest generates no melodic or broken notes");
    MelodyContext context;context.upcoming=&silent;context.changeStep=1;
    auto audible=makePreset(8);check(detailStep(audible,0,context).count>0,"all-rest future safely terminates phrase planning");
    silent.percussion.enabled=true;Clock rests(silent);rests.through(768);check(rests.e.status().notesTriggered==0&&rests.e.status().detailNotes==0&&rests.e.status().percussionHits>0,"rests silence pitched sources but retain percussion");
    auto evolution=silent;evolution.music.evolve=true;evolution.music.evolveTarget=1;evolution.music.variation=1;
    Clock evolving(evolution);evolving.through(384);check(evolving.e.status().evolutions==0,"evolve does not fill rests");
    // Exercise actual sample rendering and the internal clock at the boundary.
    for(int rate:{44100,48000,96000}){Engine e;e.prepare(rate);auto internal=s;internal.bpm=120;load(e,internal);command(e,Action::play);
        render(e,rate*2);check(e.status().compositionChanges==0,"internal boundary not early");render(e,1);
        check(e.status().compositionChanges==1&&e.status().motifChanges==1,"internal and MIDI cycle grids agree");
        render(e,rate*3);command(e,Action::panic);float l[256],r[256];e.process(l,r,256);for(float v:l)check(v==0,"panic clears cycle audio");}
    // Updating one cycle does not restart the other; invalid settings are rejected.
    Clock controls(s);controls.through(48);Command update;update.action=Action::cycles;update.session=controls.e.session();update.session.cycles.compositionBars=4;controls.e.command(update);
    controls.through(96);check(controls.e.status().motifChanges==1&&controls.e.status().compositionChanges==0,"independent interval editing");
    update.session.cycles.motifBars=3;auto rejects=controls.e.status().rejected;controls.e.command(update);check(controls.e.status().rejected==rejects+1,"invalid interval rejected");
    auto liveSeed=controls.e.session().detail.seed;update.action=Action::detail;update.session.detail.seed=1;update.session.detail.motif=.5f;controls.e.command(update);
    check(controls.e.session().detail.seed==liveSeed&&controls.e.session().detail.motif==.5f,"stale mix gesture cannot undo automatic motif");
    command(controls.e,Action::newMotif);check(controls.e.session().detail.seed==nextMotifSeed(liveSeed),"manual motif uses current automatic identity");
    // Verify the engine uses the announced automatic boundary in its melodic lookahead.
    {auto planned=makePreset(10);planned.music.evolve=false;planned.detail.breathe=false;planned.cycles={true,true,1,1};Clock forecast(planned);
        MelodyContext expected;int checked=0;
        for(int tick=0;tick<96;++tick){const auto before=forecast.e.session();auto upcoming=before;alternateComposition(upcoming);upcoming.detail.seed=nextMotifSeed(before.detail.seed);
            expected.upcoming=&upcoming;expected.changeStep=8;
            const auto events=tick%12==0?detailStep(before,tick/12,expected):DetailStep{};forecast.through(tick);
            for(int i=0;i<events.count;++i)if(events.events[i].role==1){const auto note=events.events[i].note;
                check(forecast.e.status().lastMelodyNote==note,"engine melody includes composition and motif forecast");
                expected.previousNote=note;expected.previousPassing=!chordContains(before.phrase.chords[0],note);++checked;}}
        check(checked>0,"forecast fixture schedules a melody");}
    std::cout<<"PASS single-chord edits, rests, locks, undo, bar grid, independent cycles, priority, MIDI pause/restart/loss, silence and audio\n";
}
