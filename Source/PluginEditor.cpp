#include "PluginEditor.h"

using namespace vosk;
using namespace vosk::ui;

VoskAudioProcessorEditor::VoskAudioProcessorEditor (VoskAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
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
      lfo1 (p.apvts, 1, col::purple, [&p] { return p.getLfoPhase (0); }),
      lfo2 (p.apvts, 2, col::purple, [&p] { return p.getLfoPhase (1); }),
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

    addAndMakeVisible (canvas);
    canvas.onPaint = [this] (juce::Graphics& g) { paintCanvas (g); };
    canvas.setInterceptsMouseClicks (false, true); // pass-through; children handle clicks

    // Everything lives on the canvas so it scales together.
    for (auto* c : std::initializer_list<juce::Component*> {
             &osc1, &osc2, &osc3, &sources, &filter, &ampEnv, &filterEnv,
             &lfo1, &lfo2, &matrix, &character, &chorus, &delay, &reverb, &tape,
             &macros, &visualiser, &global, &presetBar, &tabMod, &tabFx })
        canvas.addAndMakeVisible (c);

    // Dark keyboard.
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, col::panelHi.brighter (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, col::bgDeep);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, col::line);
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.4f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, col::magenta.withAlpha (0.3f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, col::magenta.withAlpha (0.6f));
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, col::panel2);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, col::dim);
    keyboard.setKeyWidth (22.0f);
    keyboard.setLowestVisibleKey (24);
    canvas.addAndMakeVisible (keyboard);

    // Lower-section tabs (Modulation vs FX & Output).
    tabMod.setButtonText ("MODULATION");
    tabFx.setButtonText ("FX & OUTPUT");
    for (auto* b : { &tabMod, &tabFx })
    {
        b->setClickingTogglesState (true);
        b->setColour (juce::TextButton::buttonOnColourId, col::cyan);
    }
    tabMod.onClick = [this] { setTab (0); };
    tabFx.onClick  = [this] { setTab (1); };

    // UI scale control.
    scaleBox.addItemList ({ "75%", "100%", "125%", "150%" }, 1);
    scaleBox.setSelectedItemIndex (1, juce::dontSendNotification);
    scaleBox.setTooltip ("UI scale");
    scaleBox.onChange = [this]
    {
        const float s[] { 0.75f, 1.0f, 1.25f, 1.5f };
        setScale (s[juce::jlimit (0, 3, scaleBox.getSelectedItemIndex())]);
    };
    canvas.addAndMakeVisible (scaleBox);

    layoutCanvas();
    setTab (0);

    setResizable (false, false);
    setScale (1.0f);
}

VoskAudioProcessorEditor::~VoskAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void VoskAudioProcessorEditor::setScale (float s)
{
    uiScale = s;
    setSize (juce::roundToInt (kDesignW * s), juce::roundToInt (kDesignH * s));
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
    canvas.repaint();
}

void VoskAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (col::bgDeep);
}

void VoskAudioProcessorEditor::resized()
{
    canvas.setTransform ({});
    canvas.setBounds (0, 0, kDesignW, kDesignH);
    canvas.setTransform (juce::AffineTransform::scale (uiScale));
}

//==============================================================================
void VoskAudioProcessorEditor::paintCanvas (juce::Graphics& g)
{
    const float W = (float) kDesignW;
    const float H = (float) kDesignH;

    g.setGradientFill (juce::ColourGradient (col::bg, 0.0f, 0.0f, col::bgDeep, 0.0f, H, false));
    g.fillAll();

    g.setColour (juce::Colours::white.withAlpha (0.012f));
    for (float x = 0.0f; x < W; x += 26.0f) g.drawVerticalLine ((int) x, 0.0f, H);
    for (float y = 0.0f; y < H; y += 26.0f) g.drawHorizontalLine ((int) y, 0.0f, W);

    g.setGradientFill (juce::ColourGradient (col::magenta.withAlpha (0.10f), 90.0f, 30.0f,
                                             col::magenta.withAlpha (0.0f), 90.0f, 220.0f, true));
    g.fillRect (0.0f, 0.0f, W, 240.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, W * 0.5f, H * 0.5f,
                                             juce::Colours::black.withAlpha (0.45f), W * 0.5f, H, true));
    g.fillAll();

    // ---- Header ----
    auto header = juce::Rectangle<int> (0, 0, kDesignW, kDesignH).reduced (16).removeFromTop (56);

    auto markArea = header.removeFromLeft (52).toFloat();
    const float mx = markArea.getCentreX(), my = markArea.getCentreY() + 2.0f;
    juce::Path headP;
    headP.addTriangle (mx, my + 13.0f, mx - 11.0f, my - 7.0f, mx + 11.0f, my - 7.0f);
    g.setColour (col::magenta.withAlpha (0.18f));
    g.fillPath (headP);
    g.setColour (col::magenta);
    g.strokePath (headP, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.drawLine (mx - 5.0f, my - 7.0f, mx - 11.0f, my - 17.0f, 1.6f);
    g.drawLine (mx + 5.0f, my - 7.0f, mx + 11.0f, my - 17.0f, 1.6f);
    g.setColour (col::cyan);
    g.fillEllipse (mx - 4.5f, my - 3.0f, 3.0f, 3.0f);
    g.fillEllipse (mx + 1.5f, my - 3.0f, 3.0f, 3.0f);

    auto title = header.removeFromLeft (210);
    g.setColour (col::text);
    g.setFont (fontKerned (42.0f, 0.22f, true));
    g.drawText ("VOSK", title, juce::Justification::centredLeft);
    g.setColour (col::magenta);
    g.fillRect ((float) title.getX() + 2.0f, (float) header.getBottom() - 2.0f, 168.0f, 2.5f);

    g.setColour (col::dim);
    g.setFont (fontKerned (13.5f, 0.28f, true));
    g.drawText ("MANTISVEX  //  DARKSYNTH ENGINE", header.withTrimmedLeft (10),
                juce::Justification::centredLeft);

    auto divider = juce::Rectangle<int> (0, 0, kDesignW, kDesignH).reduced (16).withTrimmedTop (62)
                       .removeFromTop (1).toFloat();
    g.setGradientFill (juce::ColourGradient (col::magenta.withAlpha (0.5f), divider.getX(), 0.0f,
                                             col::line.withAlpha (0.0f), divider.getRight(), 0.0f, false));
    g.fillRect (divider);
}

//==============================================================================
void VoskAudioProcessorEditor::layoutCanvas()
{
    auto full = juce::Rectangle<int> (0, 0, kDesignW, kDesignH);

    // Header: scale combo + preset bar on the right.
    {
        auto header = full.reduced (16).removeFromTop (56);
        auto pb = header.removeFromRight (620);
        presetBar.setBounds (pb.withSizeKeepingCentre (620, 32).translated (0, 4));
        auto sc = header.removeFromRight (86);
        scaleBox.setBounds (sc.withSizeKeepingCentre (78, 26).translated (-6, 4));
    }

    auto r = full.reduced (14);
    r.removeFromTop (64); // header

    // Oscillator row.
    auto oscRow = r.removeFromTop (252);
    auto srcCol = oscRow.removeFromRight (336);
    const int ow = oscRow.getWidth() / 3;
    osc1.setBounds (oscRow.removeFromLeft (ow).reduced (5));
    osc2.setBounds (oscRow.removeFromLeft (ow).reduced (5));
    osc3.setBounds (oscRow.reduced (5));
    sources.setBounds (srcCol.reduced (5));

    r.removeFromTop (4);

    // Core synthesis row.
    auto core = r.removeFromTop (192);
    auto fEnvCol = core.removeFromRight (430);
    auto aEnvCol = core.removeFromRight (430);
    filter.setBounds (core.reduced (5));
    ampEnv.setBounds (aEnvCol.reduced (5));
    filterEnv.setBounds (fEnvCol.reduced (5));

    r.removeFromTop (6);

    // On-screen keyboard at the very bottom.
    keyboard.setBounds (r.removeFromBottom (54).reduced (2, 2));
    r.removeFromBottom (4);

    // Global + Scope + Macros bar.
    auto globalRow = r.removeFromBottom (104);
    auto macroCol  = globalRow.removeFromRight (560);
    auto scopeCol  = globalRow.removeFromRight (360);
    global.setBounds (globalRow.reduced (5));
    visualiser.setBounds (scopeCol.reduced (5));
    macros.setBounds (macroCol.reduced (5));

    r.removeFromTop (4);

    // Tab bar + tabbed lower section.
    auto tabBar = r.removeFromTop (30);
    tabMod.setBounds (tabBar.removeFromLeft (180).reduced (2, 1));
    tabFx.setBounds  (tabBar.removeFromLeft (180).reduced (2, 1));

    auto tabArea = r.reduced (0, 2);
    {
        auto a = tabArea;
        lfo1.setBounds (a.removeFromLeft (360).reduced (5));
        lfo2.setBounds (a.removeFromLeft (360).reduced (5));
        matrix.setBounds (a.reduced (5));
    }
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
