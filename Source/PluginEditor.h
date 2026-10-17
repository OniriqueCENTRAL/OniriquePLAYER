#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

//==============================================================================
//  WebBrowserComponent qui intercepte les "commandes" envoyees par la page
//  (la page navigue vers http://vcam.local/cmd?... et on annule la navigation).
//==============================================================================
class PlayerBrowser : public juce::WebBrowserComponent
{
public:
    explicit PlayerBrowser (const Options& o) : juce::WebBrowserComponent (o) {}

    std::function<bool (const juce::String&)> onNavigate;   // renvoie true = autoriser
    std::function<void (const juce::String&)> onLoaded;

    bool pageAboutToLoad (const juce::String& url) override
    {
        return onNavigate ? onNavigate (url) : true;
    }

    void pageFinishedLoading (const juce::String& url) override
    {
        if (onLoaded) onLoaded (url);
    }
};

//==============================================================================
class PlayerOniriqueEditor : public juce::AudioProcessorEditor,
                             private juce::Timer,
                             private juce::ChangeListener
{
public:
    explicit PlayerOniriqueEditor (PlayerOniriqueProcessor&);
    ~PlayerOniriqueEditor() override;

    void resized() override;

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    bool handleNavigation (const juce::String& url);
    void handleCommand (const juce::String& query);
    void chooseVideo();
    void setVideoFile (const juce::File&);
    void applyAspect (double ratio);     // adapte la fenetre au format de la video
    void pushInit();
    void runJS (const juce::String&);

    PlayerOniriqueProcessor& proc;

    juce::File pageFile;
    std::unique_ptr<PlayerBrowser> browser;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::ComponentBoundsConstrainer constrainer;
    bool pageReady = false;
    bool sizeRestored = false;           // true une fois la taille initiale appliquee

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayerOniriqueEditor)
};
