#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "GuiComponents.h"

//==============================================================================
//  VOSK editor — custom darksynth GUI. All content lives on a fixed-size
//  "canvas" that is scaled by an AffineTransform for the UI-scale control, so
//  the whole interface (including fonts/graphics) scales crisply.
//==============================================================================
class VoskAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VoskAudioProcessorEditor (VoskAudioProcessor&);
    ~VoskAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Lightweight forwarding container that gets the scale transform.
    struct Canvas : juce::Component
    {
        std::function<void (juce::Graphics&)> onPaint;
        void paint (juce::Graphics& g) override { if (onPaint) onPaint (g); }
    };

    static constexpr int kDesignW = 1480;
    static constexpr int kDesignH = 988;

    void paintCanvas (juce::Graphics&);
    void layoutCanvas();
    void setTab (int t);
    void setScale (float s);

    VoskAudioProcessor& proc;
    vosk::ui::LookAndFeel lnf;

    Canvas canvas;
    float uiScale = 1.0f;
    juce::ComboBox scaleBox;
    juce::MidiKeyboardComponent keyboard;
    juce::TooltipWindow tooltips { nullptr, 600 };

    vosk::PresetManager   presetManager;
    vosk::ui::PresetBar   presetBar;

    juce::TextButton tabMod, tabFx;
    int currentTab = 0;

    vosk::ui::OscPanel     osc1, osc2, osc3;
    vosk::ui::SourcesPanel sources;
    vosk::ui::FilterPanel  filter;
    vosk::ui::EnvPanel     ampEnv, filterEnv;
    vosk::ui::LfoPanel     lfo1, lfo2;
    vosk::ui::MatrixPanel  matrix;
    vosk::ui::CharacterPanel character;
    vosk::ui::ChorusPanel  chorus;
    vosk::ui::DelayPanel   delay;
    vosk::ui::ReverbPanel  reverb;
    vosk::ui::TapePanel    tape;
    vosk::ui::MacrosPanel  macros;
    vosk::ui::VisualiserPanel visualiser;
    vosk::ui::GlobalPanel  global;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoskAudioProcessorEditor)
};
