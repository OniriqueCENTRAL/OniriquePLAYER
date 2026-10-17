#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

PlayerOniriqueProcessor::PlayerOniriqueProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true))
{
}

void PlayerOniriqueProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
}

bool PlayerOniriqueProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();

    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void PlayerOniriqueProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    // L'audio passe tel quel (traitement en place) : on ne touche a rien.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    lastProcessTick = Time::getMillisecondCounter();

    // Position de lecture de FL Studio
    if (auto* ph = getPlayHead())
    {
        auto pos = ph->getPosition();

        if (pos.hasValue())
        {
            hostPlaying = pos->getIsPlaying();

            double t = 0.0;
            auto seconds = pos->getTimeInSeconds();
            auto samples = pos->getTimeInSamples();
            auto ppq     = pos->getPpqPosition();
            auto bpm     = pos->getBpm();

            if (seconds.hasValue())
                t = *seconds;
            else if (samples.hasValue())
                t = (double) *samples / currentSampleRate;
            else if (ppq.hasValue() && bpm.hasValue() && *bpm > 0.0)
                t = *ppq * 60.0 / *bpm;

            hostTime = jmax (0.0, t);
        }
    }
}

AudioProcessorEditor* PlayerOniriqueProcessor::createEditor()
{
    return new PlayerOniriqueEditor (*this);
}

//==============================================================================
PlayerOniriqueProcessor::Settings PlayerOniriqueProcessor::getSettings() const
{
    const ScopedLock sl (lock);
    return settings;
}

void PlayerOniriqueProcessor::setSettings (const Settings& s)
{
    const ScopedLock sl (lock);
    settings = s;
}

void PlayerOniriqueProcessor::getStateInformation (MemoryBlock& destData)
{
    const auto s = getSettings();

    XmlElement xml ("PlayerOniriqueState");
    xml.setAttribute ("path",  s.path);
    xml.setAttribute ("sync",  s.sync);
    xml.setAttribute ("muted", s.muted);
    xml.setAttribute ("aspect", s.aspect);
    xml.setAttribute ("width",  s.width);
    xml.setAttribute ("height", s.height);

    copyXmlToBinary (xml, destData);
}

void PlayerOniriqueProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName ("PlayerOniriqueState"))
        {
            Settings s;
            s.path  = xml->getStringAttribute ("path");
            s.sync  = xml->getBoolAttribute ("sync",  true);
            s.muted = xml->getBoolAttribute ("muted", true);
            s.aspect = xml->getDoubleAttribute ("aspect", 4.0 / 3.0);
            s.width  = xml->getIntAttribute ("width", 640);
            s.height = xml->getIntAttribute ("height", 480);
            setSettings (s);

            sendChangeMessage();   // previent l'editeur s'il est deja ouvert
        }
    }
}

//==============================================================================
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlayerOniriqueProcessor();
}
