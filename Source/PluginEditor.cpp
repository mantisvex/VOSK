#include "PluginEditor.h"

using namespace vosk;
using namespace vosk::ui;

VoskAudioProcessorEditor::VoskAudioProcessorEditor (VoskAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      proc (p),
      osc1 (p.apvts, 1, col::magenta),
      osc2 (p.apvts, 2, col::magenta),
      osc3 (p.apvts, 3, col::magenta),
      sources (p.apvts),
      filter (p.apvts),
      ampEnv (p.apvts, "AMP ENV", col::green,
              ids::kAmpAttack, ids::kAmpDecay, ids::kAmpSustain, ids::kAmpRelease, ids::kAmpVelSens),
      filterEnv (p.apvts, "FILTER ENV (ENV 2)", col::cyan,
              ids::kFilterAttack, ids::kFilterDecay, ids::kFilterSustain, ids::kFilterRelease, ids::kFilterVelSens),
      lfo1 (p.apvts, 1, col::purple),
      lfo2 (p.apvts, 2, col::purple),
      matrix (p.apvts),
      chorus (p.apvts),
      delay (p.apvts),
      reverb (p.apvts),
      macros (p.apvts),
      global (p.apvts)
{
    setLookAndFeel (&lnf);

    for (auto* c : std::initializer_list<juce::Component*> {
             &osc1, &osc2, &osc3, &sources, &filter, &ampEnv, &filterEnv,
             &lfo1, &lfo2, &matrix, &chorus, &delay, &reverb, &macros, &global })
        addAndMakeVisible (c);

    setResizable (true, true);
    setResizeLimits (1060, 820, 1900, 1500);
    setSize (1280, 1004);
}

VoskAudioProcessorEditor::~VoskAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void VoskAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background.
    g.setGradientFill (juce::ColourGradient (col::bg, 0.0f, 0.0f,
                                             col::bgDeep, 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Header.
    auto header = getLocalBounds().reduced (12).removeFromTop (50);

    g.setColour (col::text);
    g.setFont (juce::Font (juce::FontOptions (34.0f)).boldened().withExtraKerningFactor (0.20f));
    auto title = header.removeFromLeft (190);
    g.drawText ("VOSK", title, juce::Justification::centredLeft);

    // Accent glyph next to the wordmark.
    g.setColour (col::magenta);
    g.fillRect (title.getX() + 2, header.getCentreY() + 16, 150, 2);

    g.setColour (col::dim);
    g.setFont (juce::Font (juce::FontOptions (12.0f)).withExtraKerningFactor (0.25f));
    g.drawText ("MANTISVEX  //  DARKSYNTH ENGINE",
                header.withTrimmedLeft (6), juce::Justification::centredLeft);

    g.setColour (col::dim);
    g.setFont (juce::Font (juce::FontOptions (10.5f)).withExtraKerningFactor (0.2f));
    g.drawText ("STAGE 1-6", header, juce::Justification::centredRight);

    // Divider under the header.
    auto line = getLocalBounds().reduced (12).withTrimmedTop (54).removeFromTop (1);
    g.setColour (col::line);
    g.fillRect (line);
}

void VoskAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (12);
    r.removeFromTop (58); // header

    // --- Oscillator row ---
    auto oscRow = r.removeFromTop (250);
    auto srcCol = oscRow.removeFromRight (272);
    const int ow = oscRow.getWidth() / 3;
    osc1.setBounds (oscRow.removeFromLeft (ow).reduced (4));
    osc2.setBounds (oscRow.removeFromLeft (ow).reduced (4));
    osc3.setBounds (oscRow.reduced (4));
    sources.setBounds (srcCol.reduced (4));

    r.removeFromTop (4);

    // --- Filter + envelopes row ---
    auto mid = r.removeFromTop (152);
    auto fEnvCol = mid.removeFromRight (300);
    auto aEnvCol = mid.removeFromRight (300);
    filter.setBounds (mid.reduced (4));
    ampEnv.setBounds (aEnvCol.reduced (4));
    filterEnv.setBounds (fEnvCol.reduced (4));

    r.removeFromTop (4);

    // --- Global + Macros bar at the bottom ---
    auto globalRow = r.removeFromBottom (118);
    auto macroCol = globalRow.removeFromRight (560);
    global.setBounds (globalRow.reduced (4));
    macros.setBounds (macroCol.reduced (4));

    // --- FX row (above the global bar) ---
    auto fxRow = r.removeFromBottom (150);
    const int fxW = fxRow.getWidth() / 3;
    chorus.setBounds (fxRow.removeFromLeft (fxW).reduced (4));
    delay.setBounds  (fxRow.removeFromLeft (fxW).reduced (4));
    reverb.setBounds (fxRow.reduced (4));

    // --- LFOs + matrix row (fills the remaining middle) ---
    auto bottom = r;
    lfo1.setBounds (bottom.removeFromLeft (264).reduced (4));
    lfo2.setBounds (bottom.removeFromLeft (264).reduced (4));
    matrix.setBounds (bottom.reduced (4));
}
