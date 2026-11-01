#include "MainComponent.h"
#include "InterfaceScale.h"
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
            setResizable(true,false);setResizeLimits(760,540,8192,8192);
            auto area=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
            auto& view=*static_cast<View*>(getContentComponent());view.updateScale(area);
            centreWithSize(std::min(int(1140*view.scale),area.getWidth()-40),std::min(int(866*view.scale),area.getHeight()-50));setVisible(true);
        }
        undertone::MainComponent& main(){return static_cast<View*>(getContentComponent())->canvas.main;}
        void closeButtonPressed() override{juce::JUCEApplication::getInstance()->systemRequestedQuit();}
        void resized() override{juce::DocumentWindow::resized();moved();}
        void moved() override{if(auto* view=static_cast<View*>(getContentComponent()))if(auto* display=juce::Desktop::getInstance().getDisplays().getDisplayForRect(getBounds()))view->updateScale(display->userArea);}
    private:
        class View final : public juce::Viewport {
        public:
            struct Canvas final : juce::Component {
                undertone::MainComponent main;
                Canvas(){addAndMakeVisible(main);}
            } canvas;
            float scale=1;
            View(){
                const auto overrideDirectory=juce::SystemStats::getEnvironmentVariable("UNDERTONE_DATA_DIR",{});
                const auto directory=overrideDirectory.isNotEmpty()?juce::File(overrideDirectory):juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Undertone");
                settings=directory.getChildFile("interface.json");
                const auto saved=juce::JSON::parse(settings)["scaleChoice"];
                if(saved.isInt()&&int(saved)>=0&&int(saved)<=5)choice=int(saved);
                setViewedComponent(&canvas,false);setSize(1140,850);
                for(auto* bar:{&getVerticalScrollBar(),&getHorizontalScrollBar()})bar->setColour(juce::ScrollBar::thumbColourId,juce::Colour(0xffe9b477).withAlpha(.55f));
                canvas.main.onViewSizeChanged=[this]{setViewPosition(0,0);layout();};
                canvas.main.onInterfaceScaleChanged=[this](int value){
                    choice=value;auto* object=new juce::DynamicObject();object->setProperty("scaleChoice",choice);
                    if(settings.getParentDirectory().createDirectory().wasOk())settings.replaceWithText(juce::JSON::toString(juce::var(object)));
                    updateScale(displayArea);setViewPosition(0,0);
                };
            }
            ~View() override{setViewedComponent(nullptr,false);}
            void updateScale(juce::Rectangle<int> area){
                displayArea=area;const float next=undertone::interfaceScale(choice,area.getWidth(),area.getHeight());
                scale=next;canvas.main.setInterfaceScaleChoice(choice,scale);layout();
            }
            void resized() override{juce::Viewport::resized();layout();}
            void visibleAreaChanged(const juce::Rectangle<int>&) override{layout();}
        private:
            int choice=0;bool layingOut=false;
            juce::File settings;juce::Rectangle<int> displayArea{0,0,1920,1080};
            void layout(){
                if(layingOut)return;const juce::ScopedValueSetter<bool> guard(layingOut,true);
                // Reserve scrollbar space consistently to avoid resize oscillation at a threshold.
                const int width=std::max(1080,int((getWidth()-16)/scale));
                const int height=std::max(canvas.main.preferredHeight(),int((getHeight()-16)/scale));
                canvas.main.setTransform(juce::AffineTransform::scale(scale));canvas.main.setBounds(0,0,width,height);
                canvas.setSize(int(std::ceil(width*scale)),int(std::ceil(height*scale)));
            }
        };
    };
    std::unique_ptr<Window> window;
};
START_JUCE_APPLICATION(UndertoneApplication)
