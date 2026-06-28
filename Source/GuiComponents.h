#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>
#include "VoskLookAndFeel.h"
#include "Parameters.h"
#include "ModMatrix.h"

//==============================================================================
//  Reusable VOSK GUI widgets + section panels. Everything attaches to the APVTS.
//==============================================================================
namespace vosk::ui
{
    using APVTS = juce::AudioProcessorValueTreeState;

    //--------------------------------------------------------------------------
    //  Rotary knob with a name above and value box below.
    class Knob : public juce::Component
    {
    public:
        Knob (APVTS& s, const juce::String& paramID, const juce::String& name,
              juce::Colour accent)
        {
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 15);
            slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
            slider.setColour (juce::Slider::textBoxTextColourId, col::text);
            addAndMakeVisible (slider);

            label.setText (name, juce::dontSendNotification);
            label.setJustificationType (juce::Justification::centred);
            label.setColour (juce::Label::textColourId, col::dim);
            label.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (label);

            attachment = std::make_unique<APVTS::SliderAttachment> (s, paramID, slider);
        }

        void resized() override
        {
            auto r = getLocalBounds();
            label.setBounds (r.removeFromTop (14));
            slider.setBounds (r);
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
                addAndMakeVisible (label);
            }
            attachment = std::make_unique<APVTS::ComboBoxAttachment> (s, paramID, box);
        }

        void resized() override
        {
            auto r = getLocalBounds();
            if (hasLabel) label.setBounds (r.removeFromTop (14));
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
            g.setColour (col::panel);
            g.fillRoundedRectangle (r, 9.0f);
            g.setColour (col::line);
            g.drawRoundedRectangle (r, 9.0f, 1.0f);

            // Title bar with an accent tab.
            auto tb = r.removeFromTop (26.0f);
            g.setColour (accent);
            g.fillRoundedRectangle (tb.getX() + 9.0f, tb.getY() + 8.0f, 3.0f, 10.0f, 1.5f);
            g.setColour (accent.brighter (0.1f));
            g.setFont (juce::Font (juce::FontOptions (12.5f)).boldened().withExtraKerningFactor (0.12f));
            g.drawText (title, tb.withTrimmedLeft (18).toNearestInt(),
                        juce::Justification::centredLeft);
        }

        juce::Rectangle<int> content() const { return getLocalBounds().reduced (10).withTrimmedTop (28); }

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
              wave    (s, ids::osc (n, ids::kWave),      "WAVE"),
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
            auto top = area.removeFromTop (38);
            const int cw = top.getWidth() / 3;
            wave.setBounds  (top.removeFromLeft (cw).reduced (3, 0));
            phase.setBounds (top.removeFromLeft (cw).reduced (3, 0));
            uniPh.setBounds (top.reduced (3, 0));
            gridLayout (area, { &oct, &semi, &fine, &level, &pw, &voices, &detune, &spread }, 4);
        }

    private:
        Choice wave, phase, uniPh;
        Knob   oct, semi, fine, level, pw, voices, detune, spread;
    };

    //==========================================================================
    //  Sub + Noise + Sync/PM (right of the oscillator row).
    class SourcesPanel : public Panel
    {
    public:
        SourcesPanel (APVTS& s)
            : Panel ("SUB / NOISE / SYNC", col::cyan),
              subWave (s, ids::kSubWave, "SUB WAVE"),
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
            subWave.setBounds (combos.removeFromLeft (combos.getWidth() / 2).reduced (3, 0));
            subOct.setBounds  (combos.reduced (3, 0));

            gridLayout (area.removeFromTop (area.getHeight() / 2), { &subLvl, &nLvl, &nCol }, 3);

            auto bottom = area;
            syncOn.setBounds (bottom.removeFromTop (28).reduced (3, 2));
            gridLayout (bottom, { &syncDep, &pm }, 2);
        }

    private:
        Choice subWave, subOct;
        Knob   subLvl, nLvl, nCol;
        Toggle syncOn;
        Knob   syncDep, pm;
    };

    //==========================================================================
    //  Filter + pre-filter drive.
    class FilterPanel : public Panel
    {
    public:
        FilterPanel (APVTS& s)
            : Panel ("FILTER  //  DRIVE", col::amber),
              type    (s, ids::kFilterType, "TYPE"),
              cutoff  (s, ids::kCutoff,     "CUTOFF", col::amber),
              reso    (s, ids::kResonance,  "RESO",   col::amber),
              drive   (s, ids::kDrive,      "DRIVE",  col::amber),
              keyTrk  (s, ids::kKeyTrack,   "KEY TRK", col::amber),
              envAmt  (s, ids::kFilterEnvAmt, "ENV AMT", col::amber)
        {
            for (auto* c : std::initializer_list<juce::Component*> {
                     &type, &cutoff, &reso, &drive, &keyTrk, &envAmt })
                addAndMakeVisible (c);
        }

        void resized() override
        {
            auto area = content();
            type.setBounds (area.removeFromTop (32).removeFromLeft (160).reduced (2, 1));
            gridLayout (area, { &cutoff, &reso, &drive, &keyTrk, &envAmt }, 5);
        }

    private:
        Choice type;
        Knob   cutoff, reso, drive, keyTrk, envAmt;
    };

    //==========================================================================
    //  Envelope (amp or filter), ADSR + velocity sensitivity.
    class EnvPanel : public Panel
    {
    public:
        EnvPanel (APVTS& s, juce::String title, juce::Colour accent,
                  const char* a, const char* d, const char* sus, const char* r, const char* vel)
            : Panel (std::move (title), accent),
              atk (s, a,   "A",   accent),
              dec (s, d,   "D",   accent),
              sst (s, sus, "S",   accent),
              rel (s, r,   "R",   accent),
              vs  (s, vel, "VEL", accent)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &atk, &dec, &sst, &rel, &vs })
                addAndMakeVisible (c);
        }

        void resized() override { gridLayout (content(), { &atk, &dec, &sst, &rel, &vs }, 5); }

    private:
        Knob atk, dec, sst, rel, vs;
    };

    //==========================================================================
    //  LFO panel (one of two).
    class LfoPanel : public Panel
    {
    public:
        LfoPanel (APVTS& s, int n, juce::Colour accent)
            : Panel ("LFO " + juce::String (n), accent),
              shape (s, ids::lfo (n, ids::kLfoShape),    "SHAPE"),
              trig  (s, ids::lfo (n, ids::kLfoTrigger),  "TRIG"),
              div   (s, ids::lfo (n, ids::kLfoDivision), "DIV"),
              sync  (s, ids::lfo (n, ids::kLfoSync), "SYNC", accent),
              rate  (s, ids::lfo (n, ids::kLfoRate),  "RATE",  accent),
              phase (s, ids::lfo (n, ids::kLfoPhase), "PHASE", accent),
              fade  (s, ids::lfo (n, ids::kLfoFade),  "FADE",  accent)
        {
            for (auto* c : std::initializer_list<juce::Component*> {
                     &shape, &trig, &div, &sync, &rate, &phase, &fade })
                addAndMakeVisible (c);
        }

        void resized() override
        {
            auto area = content();
            auto row1 = area.removeFromTop (32);
            shape.setBounds (row1.removeFromLeft (row1.getWidth() / 2).reduced (3, 0));
            trig.setBounds  (row1.reduced (3, 0));

            auto row2 = area.removeFromTop (32);
            div.setBounds  (row2.removeFromLeft (row2.getWidth() / 2).reduced (3, 0));
            sync.setBounds (row2.reduced (3, 1).withTrimmedTop (14));

            gridLayout (area, { &rate, &phase, &fade }, 3);
        }

    private:
        Choice shape, trig, div;
        Toggle sync;
        Knob   rate, phase, fade;
    };

    //==========================================================================
    //  8-slot modulation matrix.
    class MatrixPanel : public Panel
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
                addAndMakeVisible (*src.back());
                addAndMakeVisible (*dst.back());
                addAndMakeVisible (*amt.back());
            }
        }

        void paint (juce::Graphics& g) override
        {
            Panel::paint (g);
            auto area = content();
            auto header = area.removeFromTop (16);
            g.setColour (col::dim);
            g.setFont (juce::Font (juce::FontOptions (10.5f)).withExtraKerningFactor (0.1f));
            const int sw = (int) (area.getWidth() * 0.32f);
            g.drawText ("SOURCE", header.removeFromLeft (sw), juce::Justification::centredLeft);
            g.drawText ("DEST",   header.removeFromLeft (sw), juce::Justification::centredLeft);
            g.drawText ("AMOUNT", header, juce::Justification::centredLeft);
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
        std::vector<std::unique_ptr<Choice>> src, dst;
        std::vector<std::unique_ptr<Fader>>  amt;
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
