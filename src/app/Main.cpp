#include "MainComponent.h"
class UndertoneApplication final : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override{return "Undertone";}
    const juce::String getApplicationVersion() override{return JUCE_APPLICATION_VERSION_STRING;}
    bool moreThanOneInstanceAllowed() override{return juce::SystemStats::getEnvironmentVariable("UNDERTONE_DATA_DIR",{}).isNotEmpty();}
    void initialise(const juce::String& arguments) override{
        window=std::make_unique<Window>();
        if(arguments.startsWith("--smoke ")){
            const juce::File directory(arguments.fromFirstOccurrenceOf("--smoke ",false,false).unquoted());
            juce::Timer::callAfterDelay(1600,[this,directory]{if(window){window->main().smokeTest(directory);quit();}});
        }
    }
    void shutdown() override{window.reset();}
    void systemRequestedQuit() override{quit();}
    void anotherInstanceStarted(const juce::String&) override{if(window)window->toFront(true);}
private:
    class Window final : public juce::DocumentWindow {
    public:
        Window():DocumentWindow("Undertone / generative accompaniment",juce::Colour(0xff111416),allButtons){
            setUsingNativeTitleBar(true);setContentOwned(new View(),true);
            setResizable(true,false);setResizeLimits(760,540,1920,1400);
            auto area=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
            centreWithSize(std::min(1140,area.getWidth()-40),std::min(860,area.getHeight()-50));setVisible(true);
        }
        undertone::MainComponent& main(){return *static_cast<undertone::MainComponent*>(static_cast<View*>(getContentComponent())->getViewedComponent());}
        void closeButtonPressed() override{juce::JUCEApplication::getInstance()->systemRequestedQuit();}
    private:
        class View final : public juce::Viewport {
        public:
            View(){setViewedComponent(new undertone::MainComponent(),true);setSize(1140,840);}
            void resized() override{juce::Viewport::resized();if(auto* c=static_cast<undertone::MainComponent*>(getViewedComponent()))c->setSize(std::max(1080,getWidth()-16),std::max(c->preferredHeight(),getHeight()-16));}
        };
    };
    std::unique_ptr<Window> window;
};
START_JUCE_APPLICATION(UndertoneApplication)
