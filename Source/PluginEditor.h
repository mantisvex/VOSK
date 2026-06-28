#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "GuiComponents.h"

//==============================================================================
//  VOSK editor — custom darksynth GUI (stages 1–4 controls).
//==============================================================================
class VoskAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VoskAudioProcessorEditor (VoskAudioProcessor&);
    ~VoskAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VoskAudioProcessor& proc;
    vosk::ui::LookAndFeel lnf;

    vosk::ui::OscPanel     osc1, osc2, osc3;
    vosk::ui::SourcesPanel sources;
    vosk::ui::FilterPanel  filter;
    vosk::ui::EnvPanel     ampEnv, filterEnv;
    vosk::ui::LfoPanel     lfo1, lfo2;
    vosk::ui::MatrixPanel  matrix;
    vosk::ui::ChorusPanel  chorus;
    vosk::ui::DelayPanel   delay;
    vosk::ui::ReverbPanel  reverb;
    vosk::ui::MacrosPanel  macros;
    vosk::ui::GlobalPanel  global;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoskAudioProcessorEditor)
};
