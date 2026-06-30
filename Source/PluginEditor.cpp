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
      character (p.apvts),
      chorus (p.apvts),
      delay (p.apvts),
      reverb (p.apvts),
      tape (p.apvts),
      macros (p.apvts),
      visualiser (p.scopeBuffer, p.meterL, p.meterR),
      global (p.apvts)
{
    setLookAndFeel (&lnf);

    for (auto* c : std::initializer_list<juce::Component*> {
             &osc1, &osc2, &osc3, &sources, &filter, &ampEnv, &filterEnv,
             &lfo1, &lfo2, &matrix, &character, &chorus, &delay, &reverb, &tape, &macros, &visualiser, &global })
        addAndMakeVisible (c);

    addAndMakeVisible (presetBar);

    // Lower-section tabs (Modulation vs FX & Output) so panels can breathe.
    tabMod.setButtonText ("MODULATION");
    tabFx.setButtonText ("FX & OUTPUT");
    for (auto* b : { &tabMod, &tabFx })
    {
        b->setClickingTogglesState (true);
        b->setColour (juce::TextButton::buttonOnColourId, col::cyan);
        addAndMakeVisible (b);
    }
    tabMod.onClick = [this] { setTab (0); };
    tabFx.onClick  = [this] { setTab (1); };

    setResizable (true, true);
    setResizeLimits (1200, 840, 2200, 1500);
    setSize (1480, 956);
    setTab (0);
}

void VoskAudioProcessorEditor::setTab (int t)
{
    currentTab = t;
    const bool mod = (t == 0);
    tabMod.setToggleState (mod, juce::dontSendNotification);
    tabFx.setToggleState (! mod, juce::dontSendNotification);

    lfo1.setVisible (mod); lfo2.setVisible (mod); matrix.setVisible (mod);
    character.setVisible (! mod); chorus.setVisible (! mod); delay.setVisible (! mod);
    reverb.setVisible (! mod); tape.setVisible (! mod);
    repaint();
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
    auto header = getLocalBounds().reduced (16).removeFromTop (56);

    // MantisVex mark: an angular mantis head (triangle + antennae).
    auto markArea = header.removeFromLeft (52).toFloat();
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
    auto title = header.removeFromLeft (210);
    g.setColour (col::text);
    g.setFont (vosk::ui::fontKerned (42.0f, 0.22f, true));
    g.drawText ("VOSK", title, juce::Justification::centredLeft);
    g.setColour (col::magenta);
    g.fillRect ((float) title.getX() + 2.0f, (float) header.getBottom() - 2.0f, 168.0f, 2.5f);

    g.setColour (col::dim);
    g.setFont (vosk::ui::fontKerned (13.5f, 0.28f, true));
    g.drawText ("MANTISVEX  //  DARKSYNTH ENGINE",
                header.withTrimmedLeft (10), juce::Justification::centredLeft);

    // Divider with accent fade.
    auto divider = getLocalBounds().reduced (16).withTrimmedTop (62).removeFromTop (1).toFloat();
    g.setGradientFill (juce::ColourGradient (col::magenta.withAlpha (0.5f), divider.getX(), 0.0f,
                                             col::line.withAlpha (0.0f), divider.getRight(), 0.0f, false));
    g.fillRect (divider);
}

void VoskAudioProcessorEditor::resized()
{
    // Preset browser bar in the header (right side).
    {
        auto header = getLocalBounds().reduced (16).removeFromTop (56);
        auto pb = header.removeFromRight (620);
        presetBar.setBounds (pb.withSizeKeepingCentre (620, 32).translated (0, 4));
    }

    auto r = getLocalBounds().reduced (14);
    r.removeFromTop (64); // header

    // --- Oscillator row (3 osc + sub/noise/sync) ---
    auto oscRow = r.removeFromTop (258);
    auto srcCol = oscRow.removeFromRight (336);
    const int ow = oscRow.getWidth() / 3;
    osc1.setBounds (oscRow.removeFromLeft (ow).reduced (5));
    osc2.setBounds (oscRow.removeFromLeft (ow).reduced (5));
    osc3.setBounds (oscRow.reduced (5));
    sources.setBounds (srcCol.reduced (5));

    r.removeFromTop (4);

    // --- Core synthesis row: filter+drive + both envelopes ---
    auto core = r.removeFromTop (196);
    auto fEnvCol = core.removeFromRight (430);
    auto aEnvCol = core.removeFromRight (430);
    filter.setBounds (core.reduced (5));
    ampEnv.setBounds (aEnvCol.reduced (5));
    filterEnv.setBounds (fEnvCol.reduced (5));

    r.removeFromTop (6);

    // --- Global + Scope + Macros bar at the bottom ---
    auto globalRow = r.removeFromBottom (108);
    auto macroCol  = globalRow.removeFromRight (560);
    auto scopeCol  = globalRow.removeFromRight (360);
    global.setBounds (globalRow.reduced (5));
    visualiser.setBounds (scopeCol.reduced (5));
    macros.setBounds (macroCol.reduced (5));

    r.removeFromTop (4);

    // --- Tab bar + tabbed lower section ---
    auto tabBar = r.removeFromTop (32);
    tabMod.setBounds (tabBar.removeFromLeft (180).reduced (2, 1));
    tabFx.setBounds  (tabBar.removeFromLeft (180).reduced (2, 1));

    auto tabArea = r.reduced (0, 2);

    // MODULATION page: LFO 1 | LFO 2 | Matrix.
    {
        auto a = tabArea;
        lfo1.setBounds (a.removeFromLeft (360).reduced (5));
        lfo2.setBounds (a.removeFromLeft (360).reduced (5));
        matrix.setBounds (a.reduced (5));
    }
    // FX & OUTPUT page: character / chorus / delay / reverb / tape.
    {
        auto b = tabArea;
        const int w = b.getWidth();
        character.setBounds (b.removeFromLeft ((int) (w * 0.20f)).reduced (5));
        chorus.setBounds    (b.removeFromLeft ((int) (w * 0.18f)).reduced (5));
        delay.setBounds     (b.removeFromLeft ((int) (w * 0.24f)).reduced (5));
        reverb.setBounds    (b.removeFromLeft ((int) (w * 0.19f)).reduced (5));
        tape.setBounds      (b.reduced (5));
    }
}
