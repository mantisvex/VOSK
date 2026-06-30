#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>
#include <functional>
#include "VoskLookAndFeel.h"
#include "Parameters.h"
#include "ModMatrix.h"
#include "PresetManager.h"
#include "Scope.h"
#include <atomic>

//==============================================================================
//  Reusable VOSK GUI widgets + section panels. Everything attaches to the APVTS.
//==============================================================================
namespace vosk::ui
{
    using APVTS = juce::AudioProcessorValueTreeState;

    //--------------------------------------------------------------------------
    //  Waveform / shape glyphs drawn into a rect (one cycle, centred).
    enum class GlyphKind { OscWave, SubWave, LfoShape };

    inline void drawShapeGlyph (juce::Graphics& g, juce::Rectangle<float> a,
                                GlyphKind kind, int index, juce::Colour c, float lineW)
    {
        const float x = a.getX(), y = a.getY(), w = a.getWidth(), h = a.getHeight();
        const float mid = y + h * 0.5f;
        juce::Path p;

        auto sine = [&] (float cycles)
        {
            const int N = 48;
            for (int i = 0; i <= N; ++i)
            {
                const float t = (float) i / N;
                const float px = x + t * w;
                const float py = mid - std::sin (t * cycles * juce::MathConstants<float>::twoPi) * h * 0.42f;
                if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
            }
        };

        // Resolve (kind,index) to a concrete shape id: 0 saw-down,1 pulse,2 tri,
        // 3 sine,4 saw-up,5 square,6 S&H.
        int shape = 3;
        if (kind == GlyphKind::OscWave)  shape = (index == 0 ? 0 : index == 1 ? 1 : index == 2 ? 2 : 3);
        else if (kind == GlyphKind::SubWave) shape = (index == 0 ? 3 : 5);
        else /* LfoShape */ shape = (index == 0 ? 3 : index == 1 ? 2 : index == 2 ? 4 : index == 3 ? 0 : index == 4 ? 5 : 6);

        switch (shape)
        {
            case 0: // saw down (two teeth)
                p.startNewSubPath (x, y); p.lineTo (x + w * 0.5f, y + h);
                p.lineTo (x + w * 0.5f, y); p.lineTo (x + w, y + h);
                break;
            case 4: // saw up
                p.startNewSubPath (x, y + h); p.lineTo (x + w * 0.5f, y);
                p.lineTo (x + w * 0.5f, y + h); p.lineTo (x + w, y);
                break;
            case 1: // pulse (~40% duty)
            case 5: // square (50%)
            {
                const float duty = (shape == 1) ? 0.4f : 0.5f;
                p.startNewSubPath (x, mid - h * 0.42f);
                p.lineTo (x + w * duty, mid - h * 0.42f);
                p.lineTo (x + w * duty, mid + h * 0.42f);
                p.lineTo (x + w, mid + h * 0.42f);
                break;
            }
            case 2: // triangle
                p.startNewSubPath (x, mid); p.lineTo (x + w * 0.25f, y);
                p.lineTo (x + w * 0.75f, y + h); p.lineTo (x + w, mid);
                break;
            case 6: // sample & hold (random steps)
            {
                const float lv[5] = { 0.2f, 0.75f, 0.45f, 0.95f, 0.55f };
                for (int i = 0; i < 5; ++i)
                {
                    const float px0 = x + w * (i / 5.0f), px1 = x + w * ((i + 1) / 5.0f);
                    const float py = y + h * (1.0f - lv[i]);
                    if (i == 0) p.startNewSubPath (px0, py); else p.lineTo (px0, py);
                    p.lineTo (px1, py);
                }
                break;
            }
            default: sine (1.0f); break;
        }

        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (lineW, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    }

    //--------------------------------------------------------------------------
    //  Segmented waveform selector — replaces a choice combo with clickable
    //  glyph cells. Stays in sync with the parameter (automation/presets).
    class WaveSelector : public juce::Component
    {
    public:
        WaveSelector (APVTS& s, const juce::String& paramID, GlyphKind k, juce::Colour a)
            : kind (k), accent (a)
        {
            param = s.getParameter (paramID);
            if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (param))
                num = cp->choices.size();
            if (param != nullptr)
            {
                attachment = std::make_unique<juce::ParameterAttachment> (
                    *param, [this] (float v) { index = juce::jlimit (0, num - 1, (int) std::round (v)); repaint(); });
                attachment->sendInitialUpdate();
            }
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (attachment == nullptr || num <= 0) return;
            const int cell = juce::jlimit (0, num - 1, (int) (e.position.x / (getWidth() / (float) num)));
            attachment->setValueAsCompleteGesture ((float) cell);
        }

        // Live phase playhead (0..1) drawn on the selected cell; <0 disables.
        void setLivePhase (float p) { if (livePhase != p) { livePhase = p; repaint(); } }

        void paint (juce::Graphics& g) override
        {
            if (num <= 0) return;
            const float cw = getWidth() / (float) num;
            for (int i = 0; i < num; ++i)
            {
                auto cell = juce::Rectangle<float> (i * cw, 0.0f, cw, (float) getHeight()).reduced (2.0f);
                const bool sel = (i == index);
                g.setColour (sel ? accent.withAlpha (0.16f) : col::panel2);
                g.fillRoundedRectangle (cell, 4.0f);
                g.setColour (sel ? accent : col::line);
                g.drawRoundedRectangle (cell, 4.0f, sel ? 1.4f : 1.0f);
                drawShapeGlyph (g, cell.reduced (cell.getWidth() * 0.22f, cell.getHeight() * 0.30f),
                                kind, i, sel ? accent : col::dim, sel ? 1.9f : 1.4f);

                if (sel && livePhase >= 0.0f)
                {
                    const float px = cell.getX() + livePhase * cell.getWidth();
                    g.setColour (accent.brighter (0.3f).withAlpha (0.85f));
                    g.drawLine (px, cell.getY() + 3.0f, px, cell.getBottom() - 3.0f, 1.5f);
                }
            }
        }

    private:
        juce::RangedAudioParameter* param = nullptr;
        std::unique_ptr<juce::ParameterAttachment> attachment;
        GlyphKind kind;
        juce::Colour accent;
        int num = 0, index = 0;
        float livePhase = -1.0f;
    };

    //--------------------------------------------------------------------------
    //  Preset browser bar: [<] [Category v] [Preset v] [>]  [SAVE] [INIT]
    class PresetBar : public juce::Component, private juce::ChangeListener
    {
    public:
        explicit PresetBar (vosk::PresetManager& pm) : manager (pm)
        {
            cat.addItemList (manager.getCategories(), 1);
            cat.setSelectedItemIndex (0, juce::dontSendNotification);
            cat.onChange = [this] { rebuildPresetList(); };
            addAndMakeVisible (cat);

            preset.setTextWhenNothingSelected ("-- presets --");
            preset.onChange = [this]
            {
                if (! updating && preset.getSelectedId() > 0)
                    manager.load (cat.getText(), preset.getText());
            };
            addAndMakeVisible (preset);

            prev.setButtonText ("<");
            next.setButtonText (">");
            mutate.setButtonText ("MUTATE");
            save.setButtonText ("SAVE");
            init.setButtonText ("INIT");
            mutate.setColour (juce::TextButton::textColourOffId, col::magenta); // signature gesture
            prev.onClick = [this] { manager.step (-1, cat.getText()); };
            next.onClick = [this] { manager.step (+1, cat.getText()); };
            mutate.onClick = [this] { manager.randomise(); };
            init.onClick = [this] { manager.loadInit(); };
            save.onClick = [this] { promptSave(); };
            for (auto* b : std::initializer_list<juce::Button*> { &prev, &next, &mutate, &save, &init })
                addAndMakeVisible (b);

            manager.addChangeListener (this);
            rebuildPresetList();
        }

        ~PresetBar() override { manager.removeChangeListener (this); }

        void resized() override
        {
            auto r = getLocalBounds();
            prev.setBounds   (r.removeFromLeft (24).reduced (1, 2));
            cat.setBounds    (r.removeFromLeft (104).reduced (2, 2));
            preset.setBounds (r.removeFromLeft (190).reduced (2, 2));
            next.setBounds   (r.removeFromLeft (24).reduced (1, 2));
            r.removeFromLeft (8);
            mutate.setBounds (r.removeFromLeft (70).reduced (2, 2));
            save.setBounds   (r.removeFromLeft (58).reduced (2, 2));
            init.setBounds   (r.removeFromLeft (52).reduced (2, 2));
        }

    private:
        juce::String currentCat() const { return cat.getText(); }

        void rebuildPresetList()
        {
            updating = true;
            preset.clear (juce::dontSendNotification);
            preset.addItemList (manager.namesInCategory (currentCat()), 1);
            const int idx = manager.namesInCategory (currentCat()).indexOf (manager.getCurrentName());
            if (idx >= 0) preset.setSelectedItemIndex (idx, juce::dontSendNotification);
            updating = false;
        }

        void changeListenerCallback (juce::ChangeBroadcaster*) override
        {
            // A load happened (prev/next/save): refresh categories + selection.
            updating = true;
            const auto savedCat = cat.getText();
            cat.clear (juce::dontSendNotification);
            cat.addItemList (manager.getCategories(), 1);
            int ci = manager.getCategories().indexOf (savedCat);
            if (ci < 0) ci = 0;
            cat.setSelectedItemIndex (ci, juce::dontSendNotification);
            updating = false;
            rebuildPresetList();
        }

        void promptSave()
        {
            auto* w = new juce::AlertWindow ("Save Preset", "Preset name:",
                                             juce::MessageBoxIconType::NoIcon);
            w->addTextEditor ("name", manager.getCurrentName());
            w->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
            w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            w->enterModalState (true, juce::ModalCallbackFunction::create (
                [this, w] (int res)
                {
                    if (res == 1)
                    {
                        const auto nm = w->getTextEditorContents ("name").trim();
                        if (nm.isNotEmpty()) manager.saveUser (nm);
                    }
                    delete w;
                }), false);
        }

        vosk::PresetManager& manager;
        juce::ComboBox cat, preset;
        juce::TextButton prev, next, mutate, save, init;
        bool updating = false;
    };

    //--------------------------------------------------------------------------
    //  Rotary knob with a name above and value box below.
    class Knob : public juce::Component
    {
    public:
        Knob (APVTS& s, const juce::String& paramID, const juce::String& name,
              juce::Colour accent)
        {
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
            slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
            slider.setColour (juce::Slider::textBoxTextColourId, col::text);
            addAndMakeVisible (slider);

            label.setText (name, juce::dontSendNotification);
            label.setJustificationType (juce::Justification::centred);
            label.setColour (juce::Label::textColourId, col::dim);
            label.setFont (fontKerned (13.0f, 0.05f, true));
            label.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (label);

            attachment = std::make_unique<APVTS::SliderAttachment> (s, paramID, slider);
        }

        void resized() override
        {
            auto r = getLocalBounds();
            label.setBounds (r.removeFromTop (17));
            slider.setBounds (r);
        }

        void parentHierarchyChanged() override
        {
            // Value read-out popup on hover/drag, hosted by the top-level editor.
            slider.setPopupDisplayEnabled (true, true, getTopLevelComponent());
        }

        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<APVTS::SliderAttachment> attachment;
    };

    //--------------------------------------------------------------------------
    //  Choice combo. Empty name => no label (box fills the bounds).
    class Choice : public juce::Component
    {
    public:
        Choice (APVTS& s, const juce::String& paramID, const juce::String& name)
        {
            if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (paramID)))
                box.addItemList (cp->choices, 1);
            box.setColour (juce::ComboBox::textColourId, col::text);
            addAndMakeVisible (box);

            hasLabel = name.isNotEmpty();
            if (hasLabel)
            {
                label.setText (name, juce::dontSendNotification);
                label.setJustificationType (juce::Justification::centredLeft);
                label.setColour (juce::Label::textColourId, col::dim);
                label.setFont (fontKerned (13.0f, 0.05f, true));
                addAndMakeVisible (label);
            }
            attachment = std::make_unique<APVTS::ComboBoxAttachment> (s, paramID, box);
        }

        void resized() override
        {
            auto r = getLocalBounds();
            if (hasLabel) label.setBounds (r.removeFromTop (16));
            box.setBounds (r.reduced (0, 1));
        }

        juce::ComboBox box;
        juce::Label    label;
        bool           hasLabel = false;
        std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
    };

    //--------------------------------------------------------------------------
    //  Toggle button.
    class Toggle : public juce::Component
    {
    public:
        Toggle (APVTS& s, const juce::String& paramID, const juce::String& text,
                juce::Colour accent)
        {
            button.setButtonText (text);
            button.setClickingTogglesState (true);
            button.setColour (juce::TextButton::buttonOnColourId, accent);
            button.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
            addAndMakeVisible (button);
            attachment = std::make_unique<APVTS::ButtonAttachment> (s, paramID, button);
        }

        void resized() override { button.setBounds (getLocalBounds().reduced (0, 2)); }

        juce::TextButton button;
        std::unique_ptr<APVTS::ButtonAttachment> attachment;
    };

    //--------------------------------------------------------------------------
    //  Horizontal fader (used for mod-matrix amounts).
    class Fader : public juce::Component
    {
    public:
        Fader (APVTS& s, const juce::String& paramID, juce::Colour accent)
        {
            slider.setSliderStyle (juce::Slider::LinearHorizontal);
            slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 18);
            slider.setColour (juce::Slider::thumbColourId, accent);
            slider.setColour (juce::Slider::textBoxTextColourId, col::dim);
            addAndMakeVisible (slider);
            attachment = std::make_unique<APVTS::SliderAttachment> (s, paramID, slider);
        }
        void resized() override { slider.setBounds (getLocalBounds()); }

        juce::Slider slider;
        std::unique_ptr<APVTS::SliderAttachment> attachment;
    };

    //--------------------------------------------------------------------------
    //  Titled rounded panel base.
    class Panel : public juce::Component
    {
    public:
        Panel (juce::String t, juce::Colour a) : title (std::move (t)), accent (a) {}

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (1.0f);

            // Soft drop shadow.
            g.setColour (juce::Colours::black.withAlpha (0.30f));
            g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 9.0f);

            // Body gradient.
            g.setGradientFill (juce::ColourGradient (col::panelHi, r.getCentreX(), r.getY(),
                                                     col::panel, r.getCentreX(), r.getBottom(), false));
            g.fillRoundedRectangle (r, 9.0f);
            g.setColour (col::line);
            g.drawRoundedRectangle (r, 9.0f, 1.0f);

            // Title row: accent tab, wordmark, and a hairline underglow.
            auto tb = r.removeFromTop (30.0f);
            g.setColour (accent);
            g.fillRoundedRectangle (tb.getX() + 10.0f, tb.getY() + 8.0f, 3.5f, 14.0f, 1.5f);
            g.setColour (accent.brighter (0.15f));
            g.setFont (fontKerned (15.0f, 0.12f, true));
            g.drawText (title, tb.withTrimmedLeft (20).toNearestInt(), juce::Justification::centredLeft);

            juce::ColourGradient ug (accent.withAlpha (0.45f), r.getX() + 10.0f, 0.0f,
                                     accent.withAlpha (0.0f), r.getRight() - 20.0f, 0.0f, false);
            g.setGradientFill (ug);
            g.fillRect (r.getX() + 10.0f, tb.getBottom() + 1.0f, r.getWidth() - 20.0f, 1.0f);
        }

        juce::Rectangle<int> content() const { return getLocalBounds().reduced (11).withTrimmedTop (30); }

        juce::String title;
        juce::Colour accent;
    };

    // Lay out a list of components as a grid of `cols` columns inside `area`.
    inline void gridLayout (juce::Rectangle<int> area,
                            const std::vector<juce::Component*>& items, int cols, int gap = 4)
    {
        if (items.empty() || cols <= 0) return;
        const int rows = (int) ((items.size() + cols - 1) / cols);
        const int cw = area.getWidth() / cols;
        const int rh = area.getHeight() / rows;
        for (int i = 0; i < (int) items.size(); ++i)
        {
            const int rr = i / cols, cc = i % cols;
            juce::Rectangle<int> cell (area.getX() + cc * cw, area.getY() + rr * rh, cw, rh);
            items[(size_t) i]->setBounds (cell.reduced (gap));
        }
    }

    //==========================================================================
    //  Oscillator panel (one of three).
    class OscPanel : public Panel
    {
    public:
        OscPanel (APVTS& s, int n, juce::Colour accent)
            : Panel ("OSC " + juce::String (n), accent),
              wave    (s, ids::osc (n, ids::kWave), GlyphKind::OscWave, accent),
              phase   (s, ids::osc (n, ids::kPhaseMode), "PHASE"),
              uniPh   (s, ids::osc (n, ids::kUniPhase),  "UNI PH"),
              oct     (s, ids::osc (n, ids::kOct),       "OCT",    accent),
              semi    (s, ids::osc (n, ids::kSemi),      "SEMI",   accent),
              fine    (s, ids::osc (n, ids::kFine),      "FINE",   accent),
              level   (s, ids::osc (n, ids::kLevel),     "LEVEL",  accent),
              pw      (s, ids::osc (n, ids::kPW),        "PW",     accent),
              voices  (s, ids::osc (n, ids::kUniVoices), "VOICES", accent),
              detune  (s, ids::osc (n, ids::kUniDetune), "DETUNE", accent),
              spread  (s, ids::osc (n, ids::kUniSpread), "SPREAD", accent)
        {
            for (auto* c : std::initializer_list<juce::Component*> {
                     &wave, &phase, &uniPh, &oct, &semi, &fine, &level, &pw, &voices, &detune, &spread })
                addAndMakeVisible (c);
        }

        void resized() override
        {
            auto area = content();
            wave.setBounds (area.removeFromTop (32).reduced (2, 1));
            auto combos = area.removeFromTop (34);
            phase.setBounds (combos.removeFromLeft (combos.getWidth() / 2).reduced (3, 1));
            uniPh.setBounds (combos.reduced (3, 1));
            gridLayout (area, { &oct, &semi, &fine, &level, &pw, &voices, &detune, &spread }, 4);
        }

    private:
        WaveSelector wave;
        Choice phase, uniPh;
        Knob   oct, semi, fine, level, pw, voices, detune, spread;
    };

    //==========================================================================
    //  Sub + Noise + Sync/PM (right of the oscillator row).
    class SourcesPanel : public Panel
    {
    public:
        SourcesPanel (APVTS& s)
            : Panel ("SUB / NOISE / SYNC", col::cyan),
              subWave (s, ids::kSubWave, GlyphKind::SubWave, col::cyan),
              subOct  (s, ids::kSubOct,  "SUB OCT"),
              subLvl  (s, ids::kSubLevel,  "SUB",    col::cyan),
              nLvl    (s, ids::kNoiseLevel, "NOISE",  col::cyan),
              nCol    (s, ids::kNoiseColor, "COLOR",  col::cyan),
              syncOn  (s, ids::kSyncOn, "HARD SYNC", col::magenta),
              syncDep (s, ids::kSyncDepth, "SYNC DEPTH", col::magenta),
              pm      (s, ids::kPmAmount,  "PM AMT",  col::magenta)
        {
            for (auto* c : std::initializer_list<juce::Component*> {
                     &subWave, &subOct, &subLvl, &nLvl, &nCol, &syncOn, &syncDep, &pm })
                addAndMakeVisible (c);
        }

        void resized() override
        {
            auto area = content();
            auto combos = area.removeFromTop (32);
            subWave.setBounds (combos.removeFromLeft (combos.getWidth() / 2).reduced (3, 1));
            subOct.setBounds  (combos.reduced (3, 1).withTrimmedTop (14)); // align with selector

            gridLayout (area.removeFromTop (area.getHeight() / 2), { &subLvl, &nLvl, &nCol }, 3);

            auto bottom = area;
            syncOn.setBounds (bottom.removeFromTop (28).reduced (3, 2));
            gridLayout (bottom, { &syncDep, &pm }, 2);
        }

    private:
        WaveSelector subWave;
        Choice subOct;
        Knob   subLvl, nLvl, nCol;
        Toggle syncOn;
        Knob   syncDep, pm;
    };

    //==========================================================================
    //  Live filter response curve (approximate magnitude display).
    class FilterResponse : public juce::Component, private juce::Timer
    {
    public:
        FilterResponse (APVTS& s, juce::Colour accent)
            : col_ (accent),
              pCut (s.getRawParameterValue (ids::kCutoff)),
              pRes (s.getRawParameterValue (ids::kResonance)),
              pType (s.getRawParameterValue (ids::kFilterType))
        {
            startTimerHz (20);
        }
        ~FilterResponse() override { stopTimer(); }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (col::panel2);
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (col::line);
            g.drawRoundedRectangle (r, 4.0f, 1.0f);

            auto a = r.reduced (4.0f, 5.0f);
            // Decade grid (100 / 1k / 10k).
            g.setColour (col::line.withAlpha (0.35f));
            for (float f : { 100.0f, 1000.0f, 10000.0f })
            {
                const float x = a.getX() + a.getWidth() * fpos (f);
                g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
            }

            const float fc  = juce::jlimit (20.0f, 20000.0f, pCut->load());
            const float res = pRes->load();
            const bool  ladder = pType->load() < 0.5f;
            const float order = ladder ? 4.0f : 2.0f;

            const float dbTop = 24.0f, dbBot = -60.0f;
            auto yFor = [&] (float db)
            { return a.getY() + (dbTop - db) / (dbTop - dbBot) * a.getHeight(); };

            juce::Path p;
            const int N = (int) a.getWidth();
            for (int i = 0; i <= N; ++i)
            {
                const float x = a.getX() + (float) i;
                const float f = 20.0f * std::pow (1000.0f, (float) i / juce::jmax (1, N));
                const float lr = std::log2 (f / fc);                          // octaves from cutoff
                const float rolloff = -order * 6.0f * juce::jmax (0.0f, lr);  // slope above cutoff
                const float peak = res * 24.0f * std::exp (-(lr * lr) / (2.0f * 0.25f));
                const float y = yFor (rolloff + peak);
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }

            juce::Path fill = p;
            fill.lineTo (a.getRight(), a.getBottom());
            fill.lineTo (a.getX(), a.getBottom());
            fill.closeSubPath();
            g.setColour (col_.withAlpha (0.13f));
            g.fillPath (fill);
            g.setColour (col_.withAlpha (0.25f));
            g.strokePath (p, juce::PathStrokeType (3.0f));
            g.setColour (col_);
            g.strokePath (p, juce::PathStrokeType (1.6f));

            // Cutoff marker.
            const float cx = a.getX() + a.getWidth() * fpos (fc);
            g.setColour (col_.withAlpha (0.5f));
            g.drawVerticalLine ((int) cx, a.getY(), a.getBottom());
        }

    private:
        static float fpos (float f) { return juce::jlimit (0.0f, 1.0f, std::log (f / 20.0f) / std::log (1000.0f)); }
        void timerCallback() override
        {
            const float s = pCut->load() + pRes->load() * 1000.0f + pType->load() * 7.0f;
            if (std::abs (s - last) > 1.0e-3f) { last = s; repaint(); }
        }

        juce::Colour col_;
        std::atomic<float>* pCut; std::atomic<float>* pRes; std::atomic<float>* pType;
        float last = -1.0f;
    };

    //==========================================================================
    //  Filter + pre-filter drive (with live response graph).
    class FilterPanel : public Panel
    {
    public:
        FilterPanel (APVTS& s)
            : Panel ("FILTER  //  DRIVE", col::amber),
              resp    (s, col::amber),
              type    (s, ids::kFilterType, "TYPE"),
              cutoff  (s, ids::kCutoff,     "CUTOFF", col::amber),
              reso    (s, ids::kResonance,  "RESO",   col::amber),
              drive   (s, ids::kDrive,      "DRIVE",  col::amber),
              keyTrk  (s, ids::kKeyTrack,   "KEY TRK", col::amber),
              envAmt  (s, ids::kFilterEnvAmt, "ENV AMT", col::amber)
        {
            addAndMakeVisible (resp);
            for (auto* c : std::initializer_list<juce::Component*> {
                     &type, &cutoff, &reso, &drive, &keyTrk, &envAmt })
                addAndMakeVisible (c);
        }

        void resized() override
        {
            auto area = content();
            resp.setBounds (area.removeFromLeft ((int) (area.getWidth() * 0.32f)).reduced (0, 2));
            area.removeFromLeft (8);
            type.setBounds (area.removeFromTop (32).removeFromLeft (160).reduced (2, 1));
            gridLayout (area, { &cutoff, &reso, &drive, &keyTrk, &envAmt }, 5);
        }

    private:
        FilterResponse resp;
        Choice type;
        Knob   cutoff, reso, drive, keyTrk, envAmt;
    };

    //==========================================================================
    //  Live ADSR curve display — reads the four params and draws the shape.
    class EnvelopeDisplay : public juce::Component, private juce::Timer
    {
    public:
        EnvelopeDisplay (APVTS& s, const char* a, const char* d, const char* sus,
                         const char* r, juce::Colour accent)
            : col_ (accent),
              pA (s.getRawParameterValue (a)), pD (s.getRawParameterValue (d)),
              pS (s.getRawParameterValue (sus)), pR (s.getRawParameterValue (r))
        {
            startTimerHz (20);
        }
        ~EnvelopeDisplay() override { stopTimer(); }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (col::panel2);
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (col::line);
            g.drawRoundedRectangle (r, 4.0f, 1.0f);

            auto area = r.reduced (7.0f, 6.0f);
            const float x0 = area.getX(), y0 = area.getY();
            const float w = area.getWidth(), h = area.getHeight();

            auto seg = [] (float t, float maxT) { return std::sqrt (juce::jlimit (0.0f, 1.0f, t / maxT)); };
            const float a = seg (pA->load(), 5.0f);
            const float d = seg (pD->load(), 5.0f);
            const float rr = seg (pR->load(), 8.0f);
            const float sLvl = juce::jlimit (0.0f, 1.0f, pS->load());

            const float sustainW = 0.18f;
            float total = a + d + sustainW + rr;
            if (total < 0.001f) total = 1.0f;
            const float wA = a / total * w, wD = d / total * w;
            const float wSus = sustainW / total * w, wR = rr / total * w;

            const float bottom = y0 + h, top = y0;
            const float sustY = bottom - sLvl * h;

            juce::Path p;
            p.startNewSubPath (x0, bottom);
            p.lineTo (x0 + wA, top);
            p.lineTo (x0 + wA + wD, sustY);
            p.lineTo (x0 + wA + wD + wSus, sustY);
            p.lineTo (x0 + wA + wD + wSus + wR, bottom);

            // Fill under the curve.
            juce::Path fill = p;
            fill.lineTo (x0, bottom);
            fill.closeSubPath();
            g.setColour (col_.withAlpha (0.14f));
            g.fillPath (fill);

            g.setColour (col_.withAlpha (0.25f));
            g.strokePath (p, juce::PathStrokeType (3.0f));
            g.setColour (col_);
            g.strokePath (p, juce::PathStrokeType (1.6f));

            // Node dots.
            auto dot = [&] (float x, float y) { g.fillEllipse (x - 2.0f, y - 2.0f, 4.0f, 4.0f); };
            dot (x0 + wA, top);
            dot (x0 + wA + wD, sustY);
        }

    private:
        void timerCallback() override
        {
            const float sum = pA->load() + pD->load() + pS->load() + pR->load();
            if (std::abs (sum - last) > 1.0e-5f) { last = sum; repaint(); }
        }

        juce::Colour col_;
        std::atomic<float>* pA; std::atomic<float>* pD; std::atomic<float>* pS; std::atomic<float>* pR;
        float last = -1.0f;
    };

    //==========================================================================
    //  Envelope (amp or filter): live ADSR display + ADSR/vel knobs.
    class EnvPanel : public Panel
    {
    public:
        EnvPanel (APVTS& s, juce::String title, juce::Colour accent,
                  const char* a, const char* d, const char* sus, const char* r, const char* vel)
            : Panel (std::move (title), accent),
              disp (s, a, d, sus, r, accent),
              atk (s, a,   "A",   accent),
              dec (s, d,   "D",   accent),
              sst (s, sus, "S",   accent),
              rel (s, r,   "R",   accent),
              vs  (s, vel, "VEL", accent)
        {
            addAndMakeVisible (disp);
            for (auto* c : std::initializer_list<juce::Component*> { &atk, &dec, &sst, &rel, &vs })
                addAndMakeVisible (c);
        }

        void resized() override
        {
            auto area = content();
            disp.setBounds (area.removeFromTop (juce::jmin (62, area.getHeight() / 3)).reduced (0, 0));
            area.removeFromTop (4);
            gridLayout (area, { &atk, &dec, &sst, &rel, &vs }, 5);
        }

    private:
        EnvelopeDisplay disp;
        Knob atk, dec, sst, rel, vs;
    };

    //==========================================================================
    //  LFO panel (one of two).
    class LfoPanel : public Panel, private juce::Timer
    {
    public:
        LfoPanel (APVTS& s, int n, juce::Colour accent, std::function<float()> phaseFn = {})
            : Panel ("LFO " + juce::String (n), accent),
              shape (s, ids::lfo (n, ids::kLfoShape), GlyphKind::LfoShape, accent),
              trig  (s, ids::lfo (n, ids::kLfoTrigger),  "TRIG"),
              div   (s, ids::lfo (n, ids::kLfoDivision), "DIV"),
              sync  (s, ids::lfo (n, ids::kLfoSync), "SYNC", accent),
              rate  (s, ids::lfo (n, ids::kLfoRate),  "RATE",  accent),
              phase (s, ids::lfo (n, ids::kLfoPhase), "PHASE", accent),
              fade  (s, ids::lfo (n, ids::kLfoFade),  "FADE",  accent),
              getPhase (std::move (phaseFn))
        {
            for (auto* c : std::initializer_list<juce::Component*> {
                     &shape, &trig, &div, &sync, &rate, &phase, &fade })
                addAndMakeVisible (c);
            if (getPhase) startTimerHz (30);
        }
        ~LfoPanel() override { stopTimer(); }

        void resized() override
        {
            auto area = content();
            shape.setBounds (area.removeFromTop (30).reduced (2, 1));    // 6-glyph selector
            auto row = area.removeFromTop (34);
            trig.setBounds (row.removeFromLeft (row.getWidth() / 2).reduced (3, 1));
            div.setBounds  (row.reduced (3, 1));
            sync.setBounds (area.removeFromTop (26).reduced (3, 1));
            gridLayout (area, { &rate, &phase, &fade }, 3);
        }

    private:
        void timerCallback() override
        {
            const float p = getPhase ? getPhase() : -1.0f;
            shape.setLivePhase (p - std::floor (p));
        }

        WaveSelector shape;
        Choice trig, div;
        Toggle sync;
        Knob   rate, phase, fade;
        std::function<float()> getPhase;
    };

    //==========================================================================
    //  8-slot modulation matrix.
    class MatrixPanel : public Panel, private juce::Timer
    {
    public:
        MatrixPanel (APVTS& s) : Panel ("MOD MATRIX", col::purple)
        {
            for (int i = 0; i < 8; ++i)
            {
                const int n = i + 1;
                src.push_back (std::make_unique<Choice> (s, ids::mod (n, ids::kModSource), juce::String()));
                dst.push_back (std::make_unique<Choice> (s, ids::mod (n, ids::kModDest),   juce::String()));
                amt.push_back (std::make_unique<Fader>  (s, ids::mod (n, ids::kModAmount), col::purple));
                srcP[i] = s.getRawParameterValue (ids::mod (n, ids::kModSource));
                dstP[i] = s.getRawParameterValue (ids::mod (n, ids::kModDest));
                addAndMakeVisible (*src.back());
                addAndMakeVisible (*dst.back());
                addAndMakeVisible (*amt.back());
            }
            startTimerHz (8);
        }
        ~MatrixPanel() override { stopTimer(); }

        void paint (juce::Graphics& g) override
        {
            Panel::paint (g);
            auto area = content();
            auto header = area.removeFromTop (16);
            g.setColour (col::dim);
            g.setFont (fontKerned (12.0f, 0.1f, true));
            const int sw = (int) (area.getWidth() * 0.32f);
            g.drawText ("SOURCE", header.removeFromLeft (sw), juce::Justification::centredLeft);
            g.drawText ("DEST",   header.removeFromLeft (sw), juce::Justification::centredLeft);
            g.drawText ("AMOUNT", header, juce::Justification::centredLeft);

            // Highlight active routings (source and dest both set).
            auto rows = area.withTrimmedTop (16);
            const int rh = rows.getHeight() / 8;
            for (int i = 0; i < 8; ++i)
            {
                auto row = rows.removeFromTop (rh);
                const bool active = srcP[i] != nullptr && dstP[i] != nullptr
                                    && (int) srcP[i]->load() != 0 && (int) dstP[i]->load() != 0;
                if (active)
                {
                    g.setColour (col::purple.withAlpha (0.12f));
                    g.fillRoundedRectangle (row.toFloat().reduced (1.0f, 1.5f), 3.0f);
                    g.setColour (col::purple.withAlpha (0.5f));
                    g.fillRoundedRectangle (row.toFloat().removeFromLeft (3.0f).reduced (0.0f, 2.0f), 1.5f);
                }
            }
        }

        void resized() override
        {
            auto area = content().withTrimmedTop (16);
            const int rh = area.getHeight() / 8;
            const int sw = (int) (area.getWidth() * 0.32f);
            for (int i = 0; i < 8; ++i)
            {
                auto row = area.removeFromTop (rh).reduced (0, 2);
                src[(size_t) i]->setBounds (row.removeFromLeft (sw).reduced (2, 0));
                dst[(size_t) i]->setBounds (row.removeFromLeft (sw).reduced (2, 0));
                amt[(size_t) i]->setBounds (row.reduced (4, 0));
            }
        }

    private:
        void timerCallback() override
        {
            int sig = 0;
            for (int i = 0; i < 8; ++i)
                if (srcP[i] && dstP[i] && (int) srcP[i]->load() != 0 && (int) dstP[i]->load() != 0)
                    sig |= (1 << i);
            if (sig != lastSig) { lastSig = sig; repaint(); }
        }

        std::vector<std::unique_ptr<Choice>> src, dst;
        std::vector<std::unique_ptr<Fader>>  amt;
        std::atomic<float>* srcP[8] { nullptr };
        std::atomic<float>* dstP[8] { nullptr };
        int lastSig = -1;
    };

    //==========================================================================
    //  Output character / drive stage.
    class CharacterPanel : public Panel
    {
    public:
        CharacterPanel (APVTS& s)
            : Panel ("DRIVE / CHARACTER", col::amber),
              on   (s, ids::kCharOn, "ON", col::amber),
              mode (s, ids::kCharMode, "MODE"),
              drive(s, ids::kCharDrive, "DRIVE", col::amber),
              tone (s, ids::kCharTone,  "TONE",  col::amber),
              mix  (s, ids::kCharMix,   "MIX",   col::amber)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &on, &mode, &drive, &tone, &mix })
                addAndMakeVisible (c);
        }
        void resized() override
        {
            auto area = content();
            auto top = area.removeFromTop (30);
            on.setBounds (top.removeFromLeft (60).reduced (2, 1));
            mode.setBounds (top.removeFromLeft (110).withSizeKeepingCentre (110, 26));
            gridLayout (area, { &drive, &tone, &mix }, 3);
        }
    private:
        Toggle on; Choice mode; Knob drive, tone, mix;
    };

    //==========================================================================
    //  Tape / VHS final color.
    class TapePanel : public Panel
    {
    public:
        TapePanel (APVTS& s)
            : Panel ("TAPE / VHS", col::green),
              on   (s, ids::kTapeOn, "ON", col::green),
              wow  (s, ids::kTapeWow,  "WOW",  col::green),
              sat  (s, ids::kTapeSat,  "SAT",  col::green),
              hiss (s, ids::kTapeHiss, "HISS", col::green),
              tone (s, ids::kTapeTone, "TONE", col::green)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &on, &wow, &sat, &hiss, &tone })
                addAndMakeVisible (c);
        }
        void resized() override
        {
            auto area = content();
            on.setBounds (area.removeFromTop (28).removeFromLeft (62).reduced (2, 1));
            gridLayout (area, { &wow, &sat, &hiss, &tone }, 4);
        }
    private:
        Toggle on; Knob wow, sat, hiss, tone;
    };

    //==========================================================================
    //  FX: chorus.
    class ChorusPanel : public Panel
    {
    public:
        ChorusPanel (APVTS& s)
            : Panel ("CHORUS", col::cyan),
              on   (s, ids::kChorusOn, "ON", col::cyan),
              mode (s, ids::kChorusMode, "MODE"),
              rate (s, ids::kChorusRate,  "RATE",  col::cyan),
              depth(s, ids::kChorusDepth, "DEPTH", col::cyan),
              mix  (s, ids::kChorusMix,   "MIX",   col::cyan)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &on, &mode, &rate, &depth, &mix })
                addAndMakeVisible (c);
        }
        void resized() override
        {
            auto area = content();
            auto top = area.removeFromTop (30);
            on.setBounds (top.removeFromLeft (70).reduced (2, 1));
            mode.setBounds (top.removeFromLeft (110).withSizeKeepingCentre (110, 26));
            gridLayout (area, { &rate, &depth, &mix }, 3);
        }
    private:
        Toggle on; Choice mode; Knob rate, depth, mix;
    };

    //==========================================================================
    //  FX: tempo-synced delay.
    class DelayPanel : public Panel
    {
    public:
        DelayPanel (APVTS& s)
            : Panel ("DELAY", col::green),
              on   (s, ids::kDelayOn,   "ON",   col::green),
              sync (s, ids::kDelaySync, "SYNC", col::green),
              ping (s, ids::kDelayPingpong, "PING-PONG", col::green),
              div  (s, ids::kDelayDivision, "DIV"),
              time (s, ids::kDelayTime,     "TIME",  col::green),
              fb   (s, ids::kDelayFeedback, "FBK",   col::green),
              tone (s, ids::kDelayTone,     "TONE",  col::green),
              mix  (s, ids::kDelayMix,      "MIX",   col::green)
        {
            for (auto* c : std::initializer_list<juce::Component*> {
                     &on, &sync, &ping, &div, &time, &fb, &tone, &mix })
                addAndMakeVisible (c);
        }
        void resized() override
        {
            auto area = content();
            auto top = area.removeFromTop (30);
            on.setBounds   (top.removeFromLeft (54).reduced (2, 1));
            sync.setBounds (top.removeFromLeft (60).reduced (2, 1));
            ping.setBounds (top.removeFromLeft (96).reduced (2, 1));
            div.setBounds  (top.removeFromLeft (90).withSizeKeepingCentre (86, 26));
            gridLayout (area, { &time, &fb, &tone, &mix }, 4);
        }
    private:
        Toggle on, sync, ping; Choice div; Knob time, fb, tone, mix;
    };

    //==========================================================================
    //  FX: dark reverb.
    class ReverbPanel : public Panel
    {
    public:
        ReverbPanel (APVTS& s)
            : Panel ("REVERB", col::purple),
              on    (s, ids::kReverbOn, "ON", col::purple),
              size  (s, ids::kReverbSize,    "SIZE",   col::purple),
              decay (s, ids::kReverbDecay,   "DECAY",  col::purple),
              damp  (s, ids::kReverbDamping, "DAMP",   col::purple),
              pre   (s, ids::kReverbPredelay,"PRE-DLY",col::purple),
              mix   (s, ids::kReverbMix,     "MIX",    col::purple)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &on, &size, &decay, &damp, &pre, &mix })
                addAndMakeVisible (c);
        }
        void resized() override
        {
            auto area = content();
            on.setBounds (area.removeFromTop (30).removeFromLeft (70).reduced (2, 1));
            gridLayout (area, { &size, &decay, &damp, &pre, &mix }, 5);
        }
    private:
        Toggle on; Knob size, decay, damp, pre, mix;
    };

    //==========================================================================
    //  Macros: Hero ("ROT") gesture + 4 assignable macros.
    class MacrosPanel : public Panel
    {
    public:
        MacrosPanel (APVTS& s)
            : Panel ("MACROS", col::magenta),
              hero (s, ids::kHero,    "ROT", col::magenta),
              m1   (s, ids::kMacro1,  "M1",  col::cyan),
              m2   (s, ids::kMacro2,  "M2",  col::cyan),
              m3   (s, ids::kMacro3,  "M3",  col::cyan),
              m4   (s, ids::kMacro4,  "M4",  col::cyan)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &hero, &m1, &m2, &m3, &m4 })
                addAndMakeVisible (c);
        }
        void resized() override
        {
            auto area = content();
            // Hero gets a wider dedicated slot on the left.
            hero.setBounds (area.removeFromLeft (juce::jmin (110, area.getWidth() / 3)).reduced (4, 0));
            gridLayout (area, { &m1, &m2, &m3, &m4 }, 4);
        }
    private:
        Knob hero, m1, m2, m3, m4;
    };

    //==========================================================================
    //  Oscilloscope + stereo output meter.
    class VisualiserPanel : public Panel, private juce::Timer
    {
    public:
        VisualiserPanel (vosk::ScopeBuffer& sb, std::atomic<float>& l, std::atomic<float>& r)
            : Panel ("SCOPE", col::cyan), scopeBuf (sb), srcL (l), srcR (r)
        {
            startTimerHz (30);
        }
        ~VisualiserPanel() override { stopTimer(); }

        void paint (juce::Graphics& g) override
        {
            Panel::paint (g);
            auto area = content();
            auto meterCol = area.removeFromRight (30);
            area.removeFromRight (6);

            // Scope screen.
            auto sc = area.toFloat();
            g.setColour (col::panel2);
            g.fillRoundedRectangle (sc, 4.0f);
            g.setColour (col::line);
            g.drawRoundedRectangle (sc, 4.0f, 1.0f);

            // Faint grid.
            g.setColour (col::line.withAlpha (0.35f));
            for (int c = 1; c < 6; ++c)
            {
                const float gx = sc.getX() + sc.getWidth() * c / 6.0f;
                g.drawVerticalLine ((int) gx, sc.getY() + 3.0f, sc.getBottom() - 3.0f);
            }
            const float q = sc.getHeight() * 0.25f;
            g.drawHorizontalLine ((int) (sc.getY() + q), sc.getX() + 2.0f, sc.getRight() - 2.0f);
            g.drawHorizontalLine ((int) (sc.getBottom() - q), sc.getX() + 2.0f, sc.getRight() - 2.0f);
            g.setColour (col::line.withAlpha (0.7f));
            g.drawHorizontalLine ((int) sc.getCentreY(), sc.getX() + 2.0f, sc.getRight() - 2.0f);

            // Trigger on a rising zero-crossing for a stable trace.
            int start = 0;
            for (int i = 1; i < kN / 2; ++i)
                if (scope[i - 1] <= 0.0f && scope[i] > 0.0f) { start = i; break; }

            const int disp = kN / 2;
            juce::Path path;
            for (int i = 0; i < disp; ++i)
            {
                const float s = scope[start + i];
                const float x = sc.getX() + 3.0f + (float) i / disp * (sc.getWidth() - 6.0f);
                const float y = sc.getCentreY() - s * sc.getHeight() * 0.46f;
                if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
            }
            g.setColour (accent.withAlpha (0.22f));
            g.strokePath (path, juce::PathStrokeType (3.2f));
            g.setColour (accent);
            g.strokePath (path, juce::PathStrokeType (1.4f));

            // Stereo meter.
            auto mL = meterCol.removeFromLeft (meterCol.getWidth() / 2).reduced (2, 0);
            auto mR = meterCol.reduced (2, 0);
            drawMeter (g, mL.toFloat(), levelL, peakL);
            drawMeter (g, mR.toFloat(), levelR, peakR);
        }

    private:
        void timerCallback() override
        {
            scopeBuf.copyLatest (scope, kN);

            auto step = [] (float& lvl, float& pk, float in)
            {
                lvl = juce::jmax (in, lvl * 0.80f);          // fast attack, slow release
                pk  = (in > pk) ? in : juce::jmax (0.0f, pk - 0.012f);
            };
            step (levelL, peakL, srcL.load (std::memory_order_relaxed));
            step (levelR, peakR, srcR.load (std::memory_order_relaxed));
            repaint();
        }

        void drawMeter (juce::Graphics& g, juce::Rectangle<float> r, float level, float peak)
        {
            g.setColour (col::panel2);
            g.fillRoundedRectangle (r, 2.0f);
            g.setColour (col::line);
            g.drawRoundedRectangle (r, 2.0f, 1.0f);

            auto fillFor = [&] (float v)
            {
                const float h = juce::jlimit (0.0f, 1.0f, v) * r.getHeight();
                return r.withTop (r.getBottom() - h);
            };
            juce::ColourGradient grad (col::green, 0, r.getBottom(), col::magenta, 0, r.getY(), false);
            grad.addColour (0.7, col::amber);
            g.setGradientFill (grad);
            g.fillRect (fillFor (level).reduced (1.0f, 0.0f));

            // Peak cap.
            const float py = r.getBottom() - juce::jlimit (0.0f, 1.0f, peak) * r.getHeight();
            g.setColour (peak > 0.99f ? col::magenta : col::text);
            g.fillRect (r.getX() + 1.0f, py - 1.0f, r.getWidth() - 2.0f, 1.5f);
        }

        static constexpr int kN = 1024;
        vosk::ScopeBuffer& scopeBuf;
        std::atomic<float>& srcL;
        std::atomic<float>& srcR;
        float scope[kN] { 0.0f };
        float levelL = 0.0f, levelR = 0.0f, peakL = 0.0f, peakR = 0.0f;
    };

    //==========================================================================
    //  Global bar (master, voice mode, glide, bend).
    class GlobalPanel : public Panel
    {
    public:
        GlobalPanel (APVTS& s)
            : Panel ("GLOBAL", col::green),
              mode  (s, ids::kVoiceMode, "MODE"),
              master (s, ids::kMasterVol,      "MASTER", col::green),
              glide  (s, ids::kGlideTime,      "GLIDE",  col::green),
              bend   (s, ids::kPitchBendRange, "BEND",   col::green)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &mode, &master, &glide, &bend })
                addAndMakeVisible (c);
        }

        void resized() override
        {
            auto area = content();
            mode.setBounds (area.removeFromLeft (150).withSizeKeepingCentre (150, 32));
            gridLayout (area, { &master, &glide, &bend }, 3);
        }

    private:
        Choice mode;
        Knob   master, glide, bend;
    };
}
