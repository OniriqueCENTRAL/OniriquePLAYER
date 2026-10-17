#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
//  PLAYER ONIRIQUE : effet "passthrough" (l'audio n'est pas modifie).
//  Le plugin sert uniquement de fenetre video dans FL Studio.
//  Il lit aussi la position de lecture de l'hote pour synchroniser la video.
//==============================================================================
class PlayerOniriqueProcessor : public juce::AudioProcessor,
                          public juce::ChangeBroadcaster
{
public:
    struct Settings
    {
        juce::String path;      // chemin de la video
        bool sync  = true;      // suivre la lecture de FL Studio
        bool muted = true;      // son de la video coupe (le son vient de FL)
        double aspect = 4.0 / 3.0;   // ratio largeur / hauteur de la video
        int width  = 640;            // taille de la fenetre (memorisee)
        int height = 480;
    };

    PlayerOniriqueProcessor();
    ~PlayerOniriqueProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PLAYER ONIRIQUE"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    Settings getSettings() const;
    void setSettings (const Settings&);

    // Ecrits par le thread audio, lus par l'editeur
    std::atomic<bool>          hostPlaying     { false };
    std::atomic<double>        hostTime        { 0.0 };   // secondes
    std::atomic<juce::uint32>  lastProcessTick { 0 };     // ms, pour detecter un hote a l'arret

private:
    mutable juce::CriticalSection lock;
    Settings settings;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayerOniriqueProcessor)
};
