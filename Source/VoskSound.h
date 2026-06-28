#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

//==============================================================================
//  A single sound that responds to every MIDI note/channel. VOSK has one
//  timbre, so one sound covers the whole keyboard.
//==============================================================================
class VoskSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote   (int /*midiNote*/) override { return true; }
    bool appliesToChannel (int /*midiChannel*/) override { return true; }
};
