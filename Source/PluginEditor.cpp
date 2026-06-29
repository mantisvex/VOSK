#include "PluginEditor.h"

using namespace vosk;
using namespace vosk::ui;

VoskAudioProcessorEditor::VoskAudioProcessorEditor (VoskAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      proc (p),
      presetManager (p.apvts),
      presetBar (presetManager),
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
      visualiser (p.scopeBuffer, p.meterL, p.meterR),
      global (p.apvts)
{
    setLookAndFeel (&lnf);

    for (auto* c : std::initializer_list<juce::Component*> {
             &osc1, &osc2, &osc3, &sources, &filter, &ampEnv, &filterEnv,
             &lfo1, &lfo2, &matrix, &chorus, &delay, &reverb, &macros, &visualiser, &global })
        addAndMakeVisible (c);

    addAndMakeVisible (presetBar);

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
    const auto W = (float) getWidth();
    const auto H = (float) getHeight();

    // Base gradient.
    g.setGradientFill (juce::ColourGradient (col::bg, 0.0f, 0.0f, col::bgDeep, 0.0f, H, false));
    g.fillAll();

    // Faint tech grid.
    g.setColour (juce::Colours::white.withAlpha (0.012f));
    for (float x = 0.0f; x < W; x += 26.0f) g.drawVerticalLine ((int) x, 0.0f, H);
    for (float y = 0.0f; y < H; y += 26.0f) g.drawHorizontalLine ((int) y, 0.0f, W);

    // Magenta glow behind the header + radial vignette.
    g.setGradientFill (juce::ColourGradient (col::magenta.withAlpha (0.10f), 90.0f, 30.0f,
                                             col::magenta.withAlpha (0.0f), 90.0f, 220.0f, true));
    g.fillRect (0.0f, 0.0f, W, 240.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, W * 0.5f, H * 0.5f,
                                             juce::Colours::black.withAlpha (0.45f), W * 0.5f, H, true));
    g.fillAll();

    // ---- Header ----
    auto header = getLocalBounds().reduced (14).removeFromTop (52);

    // MantisVex mark: an angular mantis head (triangle + antennae).
    auto markArea = header.removeFromLeft (46).toFloat();
    const float mx = markArea.getCentreX(), my = markArea.getCentreY() + 2.0f;
    juce::Path head;
    head.addTriangle (mx, my + 13.0f, mx - 11.0f, my - 7.0f, mx + 11.0f, my - 7.0f);
    g.setColour (col::magenta.withAlpha (0.18f));
    g.fillPath (head);
    g.setColour (col::magenta);
    g.strokePath (head, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.drawLine (mx - 5.0f, my - 7.0f, mx - 11.0f, my - 17.0f, 1.6f); // antennae
    g.drawLine (mx + 5.0f, my - 7.0f, mx + 11.0f, my - 17.0f, 1.6f);
    g.setColour (col::cyan);
    g.fillEllipse (mx - 4.5f, my - 3.0f, 3.0f, 3.0f); // eyes
    g.fillEllipse (mx + 1.5f, my - 3.0f, 3.0f, 3.0f);

    // Wordmark.
    auto title = header.removeFromLeft (180);
    g.setColour (col::text);
    g.setFont (vosk::ui::fontKerned (36.0f, 0.22f, true));
    g.drawText ("VOSK", title, juce::Justification::centredLeft);
    g.setColour (col::magenta);
    g.fillRect ((float) title.getX() + 2.0f, (float) header.getCentreY() + 17.0f, 142.0f, 2.0f);

    g.setColour (col::dim);
    g.setFont (vosk::ui::fontKerned (12.5f, 0.30f, true));
    g.drawText ("MANTISVEX  //  DARKSYNTH ENGINE",
                header.withTrimmedLeft (8), juce::Justification::centredLeft);

    // "PRESET" caption above the browser bar (which is positioned in resized()).
    g.setColour (col::dim);
    g.setFont (vosk::ui::fontKerned (9.0f, 0.3f, true));
    g.drawText ("PRESET", header.removeFromRight (560).removeFromTop (12).withTrimmedTop (1),
                juce::Justification::topLeft);

    // Divider with accent fade.
    auto divider = getLocalBounds().reduced (14).withTrimmedTop (58).removeFromTop (1).toFloat();
    g.setGradientFill (juce::ColourGradient (col::magenta.withAlpha (0.5f), divider.getX(), 0.0f,
                                             col::line.withAlpha (0.0f), divider.getRight(), 0.0f, false));
    g.fillRect (divider);
}

void VoskAudioProcessorEditor::resized()
{
    // Preset browser bar in the header (right side).
    {
        auto header = getLocalBounds().reduced (14).removeFromTop (52);
        auto pb = header.removeFromRight (540);
        presetBar.setBounds (pb.withSizeKeepingCentre (540, 30).translated (0, 4));
    }

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

    // --- Global + Scope + Macros bar at the bottom ---
    auto globalRow = r.removeFromBottom (118);
    auto macroCol  = globalRow.removeFromRight (520);
    auto scopeCol  = globalRow.removeFromRight (320);
    global.setBounds (globalRow.reduced (4));
    visualiser.setBounds (scopeCol.reduced (4));
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
