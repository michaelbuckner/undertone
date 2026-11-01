#include "SessionFile.h"
namespace undertone {
static juce::String encodeSessionInternal(const Session& s,bool bank) {
    auto* object=new juce::DynamicObject(); juce::var root(object);
    auto set=[&](const char* key,juce::var value){object->setProperty(key,value);};
    set("format","undertone-session");set("schema",8);set("generator",s.generatorVersion);
    if(bank){
        set("activeScene",s.activeScene);set("sceneCycle",s.sceneCycle);set("sceneBars",s.sceneBars);set("sceneVariations",s.sceneVariations);
        set("sceneVariationSeed",juce::int64(s.sceneVariationSeed));set("swapStyle",int(s.swapStyle));
        juce::Array<juce::var> scenes;
        for(const auto& scene:s.scenes){if(!scene.saved){scenes.add(juce::var());continue;}
            Session state;static_cast<MusicalState&>(state)=scene.state;scenes.add(juce::JSON::parse(encodeSessionInternal(state,false)));}
        set("scenes",scenes);
    }
    set("compositionCycle",s.cycles.composition);set("motifCycle",s.cycles.motif);
    set("compositionCycleBars",s.cycles.compositionBars);set("motifCycleBars",s.cycles.motifBars);
    set("seed",juce::int64(s.seed));set("bpm",s.bpm);set("preset",s.preset);
    const auto& m=s.music;
    set("key",m.key);set("mode",int(m.mode));set("slots",m.slots);set("bars",m.bars);
    set("composition",int(m.composition));set("surface",s.detail.surface);
    set("complexity",m.complexity);set("floor",m.floor);set("spacing",m.spacing);set("rootless",m.rootless);
    set("evolve",m.evolve);set("variation",m.variation);set("rhythm",int(m.rhythm));
    set("evolveTarget",m.evolveTarget);set("evolveBars",m.evolveBars);set("tonePhase",s.tonePhase);
    const auto& p=s.patch;
    set("brightness",p.brightness);set("motion",p.motion);set("space",p.space);set("level",p.level);
    set("attack",p.attack);set("release",p.release);set("drive",p.drive);set("chorus",p.chorus);
    set("highpass",p.highpass);set("detune",p.detune);
    set("model",int(p.model));set("tone",p.tone);set("texture",p.texture);set("resonance",p.resonance);
    set("decay",p.decay);set("sustain",p.sustain);set("filterEnv",p.filterEnv);
    const auto& d=s.detail;set("bed",d.bed);set("broken",d.broken);set("motif",d.motif);set("wear",d.wear);set("echo",d.echo);
    set("breathe",d.breathe);set("motifSeed",juce::int64(d.seed));
    set("melodyVoice",int(d.voice));
    set("phrasing",d.phrasing);set("motifMode",int(d.motifMode));set("motifVariation",juce::int64(d.variationSeed));
    set("figureSeed",juce::int64(d.figureSeed));set("room",d.room);
    const auto& percussion=s.percussion;set("percussionEnabled",percussion.enabled);set("percussionLevel",percussion.level);
    set("percussionDensity",percussion.density);set("percussionGlitch",percussion.glitch);set("percussionColor",percussion.color);set("percussionSeed",juce::int64(percussion.seed));
    juce::Array<juce::var> chords;
    for(int i=0;i<m.slots;++i) {
        auto* ch=new juce::DynamicObject();const auto& c=s.phrase.chords[i];
        ch->setProperty("degree",c.degree);ch->setProperty("root",c.root);ch->setProperty("quality",c.quality);
        ch->setProperty("extension",c.extension);ch->setProperty("locked",c.locked);
        ch->setProperty("color",c.color);
        ch->setProperty("rest",c.rest);
        juce::Array<juce::var> notes;for(int n=0;n<c.count;++n)notes.add(c.notes[n]);
        ch->setProperty("notes",notes);chords.add(juce::var(ch));
    }
    set("chords",chords);return juce::JSON::toString(root,true);
}
juce::String encodeSession(const Session& s){return encodeSessionInternal(s,true);}
static juce::Result decodeSessionInternal(const juce::String& text,Session& destination,int depth) {
    juce::var root;const auto parsed=juce::JSON::parse(text,root);
    if(parsed.failed()||!root.isObject())return juce::Result::fail("Not a valid session JSON object.");
    const auto required={"format","schema","generator","seed","bpm","preset","key","mode","slots","bars","complexity","floor","spacing","rootless","evolve","variation","rhythm","brightness","motion","space","level","attack","release","drive","chorus","highpass","detune","chords"};
    for(const auto* k:required)if(!root.hasProperty(k))return juce::Result::fail(juce::String("Missing field: ")+k);
    if(root["format"].toString()!="undertone-session"||(int(root["schema"])<1||int(root["schema"])>8)||int(root["generator"])!=1)
        return juce::Result::fail("Unsupported session or generator version. The original file was not changed.");
    for(const auto* k:required) {
        if(juce::String(k)=="format"||juce::String(k)=="chords")continue;
        const auto v=root[k];
        if(juce::String(k)=="rootless"||juce::String(k)=="evolve") { if(!v.isBool())return juce::Result::fail("Boolean field has wrong type."); }
        else if(!v.isInt()&&!v.isInt64()&&!v.isDouble())return juce::Result::fail(juce::String("Numeric field has wrong type: ")+k);
    }
    Session s;
    for(const auto* k:{"schema","generator","seed","preset","key","mode","slots","bars","complexity","floor","spacing","rhythm"})
        if(!root[k].isInt()&&!root[k].isInt64())return juce::Result::fail(juce::String("Expected an integer: ")+k);
    for(const auto* k:{"schema","generator","preset","key","mode","slots","bars","complexity","floor","spacing","rhythm"})
        if(juce::int64(root[k])<0||juce::int64(root[k])>2147483647LL)return juce::Result::fail("Integer field out of range.");
    const auto seed=juce::int64(root["seed"]);if(seed<0||seed>4294967295LL)return juce::Result::fail("Invalid seed.");
    s.seed=uint32_t(seed);s.bpm=double(root["bpm"]);s.preset=int(root["preset"]);
    auto& m=s.music;m.key=int(root["key"]);m.mode=Mode(int(root["mode"]));m.slots=int(root["slots"]);m.bars=int(root["bars"]);
    m.complexity=int(root["complexity"]);m.floor=int(root["floor"]);m.spacing=int(root["spacing"]);
    m.rootless=bool(root["rootless"]);m.evolve=bool(root["evolve"]);m.variation=float(root["variation"]);m.rhythm=Rhythm(int(root["rhythm"]));
    auto& p=s.patch;p.brightness=float(root["brightness"]);p.motion=float(root["motion"]);p.space=float(root["space"]);p.level=float(root["level"]);
    p.attack=float(root["attack"]);p.release=float(root["release"]);p.drive=float(root["drive"]);p.chorus=float(root["chorus"]);
    p.highpass=float(root["highpass"]);p.detune=float(root["detune"]);
    if(int(root["schema"])>=2) {
        for(const auto* k:{"evolveTarget","evolveBars","model","tone","tonePhase","texture","resonance","decay","sustain","filterEnv"}) {
            const auto v=root[k];
            if(!root.hasProperty(k)||(!v.isInt()&&!v.isInt64()&&!v.isDouble()))return juce::Result::fail(juce::String("Missing or invalid tone/evolution field: ")+k);
        }
        for(const auto* k:{"evolveTarget","evolveBars","model","tone"})
            if((!root[k].isInt()&&!root[k].isInt64())||juce::int64(root[k])<0||juce::int64(root[k])>2147483647LL)return juce::Result::fail("Invalid tone/evolution integer.");
        m.evolveTarget=int(root["evolveTarget"]);m.evolveBars=int(root["evolveBars"]);s.tonePhase=double(root["tonePhase"]);
        p.model=ToneModel(int(root["model"]));p.tone=int(root["tone"]);p.texture=float(root["texture"]);p.resonance=float(root["resonance"]);
        p.decay=float(root["decay"]);p.sustain=float(root["sustain"]);p.filterEnv=float(root["filterEnv"]);
    } else {
        // Preserve original sound and realized chords. Legacy Evolve was chord-only, once per phrase.
        m.evolveTarget=1;m.evolveBars=m.slots*m.bars;p.model=ToneModel::warm;
    }
    if(int(root["schema"])>=3){
        for(const auto* k:{"bed","broken","motif","wear","echo"})if(!root.hasProperty(k)||(!root[k].isInt()&&!root[k].isInt64()&&!root[k].isDouble()))return juce::Result::fail("Missing or invalid arrangement value.");
        if(!root["breathe"].isBool()||(!root["motifSeed"].isInt()&&!root["motifSeed"].isInt64()))return juce::Result::fail("Invalid motif state.");
        const auto motifSeed=juce::int64(root["motifSeed"]);if(motifSeed<0||motifSeed>4294967295LL)return juce::Result::fail("Invalid motif seed.");
        s.detail.bed=float(root["bed"]);s.detail.broken=float(root["broken"]);s.detail.motif=float(root["motif"]);
        s.detail.wear=float(root["wear"]);s.detail.echo=float(root["echo"]);s.detail.breathe=bool(root["breathe"]);s.detail.seed=uint32_t(motifSeed);
    }
    if(int(root["schema"])>=4){
        for(const auto* k:{"percussionLevel","percussionDensity","percussionGlitch","percussionColor"})
            if(!root.hasProperty(k)||(!root[k].isInt()&&!root[k].isInt64()&&!root[k].isDouble()))return juce::Result::fail("Missing or invalid percussion value.");
        if(!root["percussionEnabled"].isBool()||(!root["percussionSeed"].isInt()&&!root["percussionSeed"].isInt64()))return juce::Result::fail("Invalid percussion state.");
        const auto seedValue=juce::int64(root["percussionSeed"]);if(seedValue<0||seedValue>4294967295LL)return juce::Result::fail("Invalid percussion seed.");
        auto& percussion=s.percussion;percussion.enabled=bool(root["percussionEnabled"]);percussion.level=float(root["percussionLevel"]);
        percussion.density=float(root["percussionDensity"]);percussion.glitch=float(root["percussionGlitch"]);percussion.color=float(root["percussionColor"]);percussion.seed=uint32_t(seedValue);
    }
    if(int(root["schema"])>=5){
        const auto voice=root["melodyVoice"];
        if((!voice.isInt()&&!voice.isInt64())||juce::int64(voice)<0||juce::int64(voice)>(int(root["schema"])>=6?5:3))return juce::Result::fail("Invalid or missing melody voice.");
        s.detail.voice=MelodyVoice(int(voice));
    }else s.detail.voice=MelodyVoice::original;
    if(int(root["schema"])>=6){const auto composition=root["composition"],surface=root["surface"];
        if((!composition.isInt()&&!composition.isInt64())||juce::int64(composition)<0||juce::int64(composition)>2)return juce::Result::fail("Invalid composition mode.");
        if(!surface.isDouble()&&!surface.isInt()&&!surface.isInt64())return juce::Result::fail("Invalid surface amount.");
        m.composition=Composition(int(composition));s.detail.surface=float(surface);}
    if(int(root["schema"])>=7){
        for(const auto* k:{"compositionCycle","motifCycle"})if(!root[k].isBool())return juce::Result::fail("Invalid cycle switch.");
        for(const auto* k:{"compositionCycleBars","motifCycleBars"})
            if(!root[k].isInt()||int(root[k])<1||int(root[k])>64)return juce::Result::fail("Invalid cycle interval.");
        s.cycles.composition=bool(root["compositionCycle"]);s.cycles.motif=bool(root["motifCycle"]);
        s.cycles.compositionBars=int(root["compositionCycleBars"]);s.cycles.motifBars=int(root["motifCycleBars"]);
    }
    const auto* chords=root["chords"].getArray();
    if(!chords||m.slots<2||m.slots>8||chords->size()!=m.slots)return juce::Result::fail("Invalid chord list.");
    for(int i=0;i<m.slots;++i) {
        const auto& value=chords->getReference(i);if(!value.isObject())return juce::Result::fail("Invalid chord.");
        for(const auto* k:{"degree","root","quality","extension","locked","notes"})if(!value.hasProperty(k))return juce::Result::fail("Incomplete chord.");
        for(const auto* k:{"degree","root","quality","extension"})if(!value[k].isInt())return juce::Result::fail("Invalid chord number.");
        if(!value["locked"].isBool())return juce::Result::fail("Invalid chord lock.");
        auto& c=s.phrase.chords[i];c.degree=int(value["degree"]);c.root=int(value["root"]);c.quality=int(value["quality"]);
        c.extension=int(value["extension"]);c.locked=bool(value["locked"]);
        if(int(root["schema"])>=7){if(!value["rest"].isBool())return juce::Result::fail("Invalid rest state.");c.rest=bool(value["rest"]);}
        if(int(root["schema"])>=6){const auto color=value["color"];if(!color.isInt()||int(color)<0||int(color)>2)return juce::Result::fail("Invalid harmonic color.");c.color=int(color);}
        const auto* notes=value["notes"].getArray();if(!notes||notes->size()<2||notes->size()>6)return juce::Result::fail("Invalid chord notes.");
        c.count=notes->size();for(int n=0;n<c.count;++n){if(!(*notes)[n].isInt())return juce::Result::fail("Invalid MIDI note.");c.notes[n]=int((*notes)[n]);}
    }
    if(int(root["schema"])>=8){
        if(!root["phrasing"].isBool()||!root["motifMode"].isInt()||int(root["motifMode"])<0||int(root["motifMode"])>2)
            return juce::Result::fail("Invalid phrasing or motif mode.");
        for(const auto* key:{"motifVariation","figureSeed"})if((!root[key].isInt()&&!root[key].isInt64())||juce::int64(root[key])<0||juce::int64(root[key])>4294967295LL)
            return juce::Result::fail("Invalid motif identity.");
        if(!root["room"].isDouble()&&!root["room"].isInt())return juce::Result::fail("Invalid space control.");
        s.detail.phrasing=bool(root["phrasing"]);s.detail.motifMode=MotifMode(int(root["motifMode"]));s.detail.variationSeed=uint32_t(juce::int64(root["motifVariation"]));
        s.detail.figureSeed=uint32_t(juce::int64(root["figureSeed"]));s.detail.room=float(root["room"]);
        if(depth==0){
            if(!root["activeScene"].isInt()||int(root["activeScene"])< -1||int(root["activeScene"])>1||!root["sceneCycle"].isBool()||!root["sceneVariations"].isBool()
                ||!root["sceneBars"].isInt()||int(root["sceneBars"])<1||int(root["sceneBars"])>64||!root["swapStyle"].isInt()||int(root["swapStyle"])<0||int(root["swapStyle"])>2)
                return juce::Result::fail("Invalid scene controls.");
            const auto seed=root["sceneVariationSeed"];
            if((!seed.isInt()&&!seed.isInt64())||juce::int64(seed)<0||juce::int64(seed)>4294967295LL)return juce::Result::fail("Invalid scene variation seed.");
            s.activeScene=int(root["activeScene"]);s.sceneCycle=bool(root["sceneCycle"]);s.sceneBars=int(root["sceneBars"]);s.sceneVariations=bool(root["sceneVariations"]);
            s.sceneVariationSeed=uint32_t(juce::int64(seed));s.swapStyle=SwapStyle(int(root["swapStyle"]));
            const auto* scenes=root["scenes"].getArray();if(!scenes||scenes->size()!=2)return juce::Result::fail("Expected two scene slots.");
            for(int i=0;i<2;++i){const auto& value=(*scenes)[i];if(value.isVoid())continue;
                if(!value.isObject()||int(value["schema"])!=8||value.hasProperty("scenes"))return juce::Result::fail("Invalid nested scene.");
                Session state;const auto result=decodeSessionInternal(juce::JSON::toString(value),state,1);if(result.failed())return result;
                s.scenes[i]={true,static_cast<const MusicalState&>(state)};
            }
        }else if(root.hasProperty("scenes"))return juce::Result::fail("Nested scene banks are not supported.");
    }else {s.detail.figureSeed=s.detail.seed;s.detail.phrasing=false;}
    if(!valid(s))return juce::Result::fail("Session contains out-of-range settings or invalid voicings.");
    destination=s;return juce::Result::ok();
}
juce::Result decodeSession(const juce::String& text,Session& destination){return decodeSessionInternal(text,destination,0);}
juce::Result saveSession(const juce::File& file,const Session& s) {
    if(!valid(s))return juce::Result::fail("Session is invalid; save cancelled.");
    if(file.getParentDirectory().createDirectory().failed())return juce::Result::fail("Cannot create session directory.");
    juce::TemporaryFile temporary(file);
    if(!temporary.getFile().replaceWithText(encodeSession(s)))return juce::Result::fail("Cannot write temporary session.");
    if(file.existsAsFile()&&!file.copyFileTo(file.getSiblingFile(file.getFileName()+".bak")))return juce::Result::fail("Cannot preserve previous session backup.");
    if(!temporary.overwriteTargetFileWithTemporary())return juce::Result::fail("Cannot replace session file. Previous version retained.");
    return juce::Result::ok();
}
juce::Result loadSession(const juce::File& file,Session& session) {
    if(!file.existsAsFile()||file.getSize()>1024*1024)return juce::Result::fail("Session is missing or larger than 1 MB.");
    return decodeSession(file.loadFileAsString(),session);
}
}
