#include "MainComponent.h"
#include "Design.h"

namespace undertone {
void MainComponent::showBrowser(){browserView=true;browser->refresh();resized();repaint();}
bool MainComponent::hasSessionEdits() const {return shown.manualPending||shown.sequence<sentSequence||document.edited(shown.session,shown.external);}
void MainComponent::refreshDocumentLabel(){
    const auto status=shown.manualPending?"QUEUED EDIT":shown.sequence<sentSequence?"UPDATING":document.edited(shown.session,shown.external)?"EDITED":document.file!=juce::File()?"SAVED":"UNSAVED";
    documentLabel.setText(document.name+"  /  "+status,juce::dontSendNotification);
    documentLabel.setColour(juce::Label::textColourId,hasSessionEdits()?design::accent:design::muted);
    documentLabel.setTooltip(document.file!=juce::File()?document.file.getFullPathName():"Save a variation to keep this session in your library.");
}
void MainComponent::requestReplace(std::function<void()> action){
    if(fileDialogOpen||replacePromptOpen)return;
    if(!hasSessionEdits()){action();return;}
    replacePromptOpen=true;auto safe=juce::Component::SafePointer<MainComponent>(this);
    juce::AlertWindow::showAsync(juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::NoIcon)
        .withTitle("Keep your current session?").withMessage("This session has edits. Save a separate copy before loading, discard the edits, or keep working. Queued edits must be applied before using Save copy & load.")
        .withButton("Save copy & load").withButton("Discard & load").withButton("Cancel").withAssociatedComponent(this),
        [safe,action=std::move(action)](int result){if(!safe)return;safe->replacePromptOpen=false;
            if(result==1){if(safe->shown.manualPending||safe->shown.sequence<safe->sentSequence){safe->notification="Apply queued edits first (Play / restart), then save and load. Your session is unchanged.";return;}safe->chooseSave(action);}
            else if(result==2)action();
        });
}
void MainComponent::applyLibrarySession(const Session& state,const juce::String& name,const juce::File& file){
    editing=state;
    if(!send(Action::load)){editing=shown.session;showSession(editing);return;}
    document.establish(state,name,file);showSession(state);browserView=false;resized();refreshDocumentLabel();
    notification="Loaded "+name+"; playback stopped.";
    if(file!=juce::File()){const auto result=library->remember(file);if(result.failed())notification+=" "+result.getErrorMessage();}
}
void MainComponent::loadFactoryPreset(int index){
    if(index<0||index>=presetCount)return;
    // Reset the quick picker while the user decides whether to keep their current edits.
    character.setSelectedId(editing.preset+1,juce::dontSendNotification);
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    requestReplace([safe,index]{if(!safe)return;auto state=makePreset(index,safe->shown.session.seed);
        state.scenes=safe->shown.session.scenes;state.sceneCycle=safe->shown.session.sceneCycle;state.sceneBars=safe->shown.session.sceneBars;
        state.sceneVariations=safe->shown.session.sceneVariations;state.sceneVariationSeed=safe->shown.session.sceneVariationSeed;
        safe->applyLibrarySession(state,presetName(index),{});
    });
}
void MainComponent::loadLibraryFile(const juce::File& file,bool recovery){
    Session candidate;auto result=loadSession(file,candidate);if(result.failed()){notification=result.getErrorMessage();return;}
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    requestReplace([safe,file,recovery]{if(!safe)return;Session state;auto result=loadSession(file,state);
        if(result.failed()){safe->notification=result.getErrorMessage();return;}
        safe->applyLibrarySession(state,recovery?"Recovered session":file.getFileNameWithoutExtension(),recovery?juce::File():file);
    });
}
bool MainComponent::saveDocument(const juce::File& file,const Session& snapshot){
    const auto result=saveSession(file,snapshot);if(result.failed()){notification=result.getErrorMessage();return false;}
    document.establish(snapshot,file.getFileNameWithoutExtension(),file);
    const auto history=library->remember(file);browser->refresh();refreshDocumentLabel();
    notification="Saved "+file.getFileName()+". Current phrase and settings captured; queued changes are not included.";
    if(history.failed())notification+=" "+history.getErrorMessage();return true;
}
void MainComponent::saveCurrentSession(){
    if(fileDialogOpen||replacePromptOpen)return;
    if(shown.sequence<sentSequence){notification="Waiting for your last edit; try Save again in a moment.";return;}
    if(document.file==juce::File()){chooseSave();return;}
    saveDocument(document.file,shown.session);
}
void MainComponent::chooseSave(std::function<void()> afterSave){
    if(fileDialogOpen)return;
    if(shown.sequence<sentSequence){notification="Waiting for your last edit; try Save again in a moment.";return;}
    const auto snapshot=shown.session;
    if(library->sessionDirectory().createDirectory().failed()){notification="Cannot create the session library folder.";return;}
    const auto filename=juce::File::createLegalFileName(document.name.replace(" / "," - ")+" variation");
    const auto initial=library->sessionDirectory().getNonexistentChildFile(filename,".undertone");
    chooser=std::make_unique<juce::FileChooser>("Save session variation",initial,"*.undertone");fileDialogOpen=true;
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,
        [safe,snapshot,afterSave=std::move(afterSave)](const juce::FileChooser& fc){
            if(!safe)return;safe->fileDialogOpen=false;const auto result=fc.getResult();if(result==juce::File())return;
            const auto target=result.withFileExtension("undertone");
            // The native overwrite warning covered result, not a different extension-normalized path.
            if(target!=result&&target.existsAsFile()){safe->notification="That .undertone file already exists. Choose its exact filename to confirm replacement, or save a new variation.";return;}
            if(safe->saveDocument(target,snapshot)&&afterSave)afterSave();
        });
}
void MainComponent::openSession(){
    if(fileDialogOpen||replacePromptOpen)return;
    chooser=std::make_unique<juce::FileChooser>("Open accompaniment session",library->sessionDirectory(),"*.undertone");fileDialogOpen=true;
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe](const juce::FileChooser& fc){
        if(!safe)return;safe->fileDialogOpen=false;const auto file=fc.getResult();
        if(file!=juce::File())juce::MessageManager::callAsync([safe,file]{if(safe)safe->loadLibraryFile(file);});
    });
}
void MainComponent::addLibraryFolder(){
    if(fileDialogOpen||replacePromptOpen)return;
    chooser=std::make_unique<juce::FileChooser>("Add a session folder",library->sessionDirectory());fileDialogOpen=true;
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories,[safe](const juce::FileChooser& fc){
        if(!safe)return;safe->fileDialogOpen=false;const auto folder=fc.getResult();if(folder==juce::File())return;
        const auto result=safe->library->addFolder(folder);safe->notification=result.wasOk()?"Folder added. Only .undertone files directly inside it are listed.":result.getErrorMessage();safe->browser->refresh();
    });
}
}
