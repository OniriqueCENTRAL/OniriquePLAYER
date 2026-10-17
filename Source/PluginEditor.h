#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

//==============================================================================
//  WebBrowserComponent qui intercepte un fichier video glisse sur la fenetre
//  (navigation vers le fichier, annulee) et signale la fin du chargement de la page.
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
    void handleCommand (const juce::String& name, const juce::String& value);
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

    // Derniere position envoyee a la page (pour ne pas la repeter quand le DAW est a l'arret)
    bool lastSentPlaying = true;
    double lastSentTime = -1.0;
    juce::uint32 lastSentMs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayerOniriqueEditor)
};
