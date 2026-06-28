#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

//==============================================================================
//  VOSK look & feel — dark cyberpunk / darksynth: charcoal panels, neon arcs,
//  per-control accent colours. Each rotary's accent comes from its
//  rotarySliderFillColourId; faders from their thumbColourId.
//==============================================================================
namespace vosk::ui
{
    namespace col
    {
        inline const juce::Colour bg      { 0xff0b0c12 };
        inline const juce::Colour bgDeep  { 0xff070810 };
        inline const juce::Colour panel   { 0xff161823 };
        inline const juce::Colour panel2  { 0xff0f111a };
        inline const juce::Colour line    { 0xff2b3042 };
        inline const juce::Colour text    { 0xffd6dae8 };
        inline const juce::Colour dim     { 0xff7a8296 };

        // Section accents.
        inline const juce::Colour magenta { 0xffff2e88 };
        inline const juce::Colour cyan    { 0xff27d8ff };
        inline const juce::Colour amber   { 0xffffb13c };
        inline const juce::Colour green   { 0xff36e0a0 };
        inline const juce::Colour purple  { 0xff9b6cff };
    }

    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel()
        {
            setColour (juce::Slider::textBoxTextColourId,    col::text);
            setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            setColour (juce::Slider::rotarySliderFillColourId, col::magenta);
            setColour (juce::Slider::thumbColourId,          col::magenta);

            setColour (juce::ComboBox::backgroundColourId, col::panel2);
            setColour (juce::ComboBox::textColourId,       col::text);
            setColour (juce::ComboBox::outlineColourId,    col::line);
            setColour (juce::ComboBox::arrowColourId,      col::dim);

            setColour (juce::PopupMenu::backgroundColourId,            col::panel2);
            setColour (juce::PopupMenu::textColourId,                  col::text);
            setColour (juce::PopupMenu::highlightedBackgroundColourId, col::magenta.withAlpha (0.30f));
            setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);

            setColour (juce::Label::textColourId, col::dim);

            setColour (juce::TextButton::buttonColourId,   col::panel2);
            setColour (juce::TextButton::buttonOnColourId, col::magenta);
            setColour (juce::TextButton::textColourOnId,   juce::Colours::black);
            setColour (juce::TextButton::textColourOffId,  col::dim);
        }

        juce::Font getLabelFont (juce::Label&) override
        {
            return juce::Font (juce::FontOptions (11.5f)).withExtraKerningFactor (0.06f);
        }

        juce::Font getComboBoxFont (juce::ComboBox&) override
        {
            return juce::Font (juce::FontOptions (12.5f));
        }

        //----------------------------------------------------------------------
        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                               float pos, float startAngle, float endAngle,
                               juce::Slider& s) override
        {
            auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (5.0f);
            const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
            const float cx = bounds.getCentreX();
            const float cy = bounds.getCentreY();
            const float lineW = radius * 0.16f;
            const float arcR = radius - lineW * 0.6f;
            const float angle = startAngle + pos * (endAngle - startAngle);
            const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);

            // Track.
            juce::Path track;
            track.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
            g.setColour (col::line);
            g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

            // Bipolar sliders fill from centre; unipolar from the start.
            const bool bipolar = s.getMinimum() < -0.0001 && s.getMaximum() > 0.0001;
            const float fromAngle = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

            // Value arc (with a soft outer glow).
            juce::Path val;
            val.addCentredArc (cx, cy, arcR, arcR, 0.0f,
                               juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle), true);
            g.setColour (accent.withAlpha (0.22f));
            g.strokePath (val, juce::PathStrokeType (lineW * 2.1f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            g.setColour (accent);
            g.strokePath (val, juce::PathStrokeType (lineW, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

            // Knob body.
            const float kr = radius - lineW * 1.7f;
            juce::ColourGradient grad (col::panel.brighter (0.10f), cx, cy - kr,
                                       col::panel2.darker (0.30f),  cx, cy + kr, false);
            g.setGradientFill (grad);
            g.fillEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f);
            g.setColour (col::line);
            g.drawEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f, 1.0f);

            // Pointer.
            const float px = cx + std::sin (angle) * (kr * 0.82f);
            const float py = cy - std::cos (angle) * (kr * 0.82f);
            const float ix = cx + std::sin (angle) * (kr * 0.30f);
            const float iy = cy - std::cos (angle) * (kr * 0.30f);
            g.setColour (accent);
            g.drawLine (ix, iy, px, py, lineW * 0.75f);
            g.fillEllipse (px - lineW * 0.55f, py - lineW * 0.55f, lineW * 1.1f, lineW * 1.1f);
        }

        //----------------------------------------------------------------------
        void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                               float sliderPos, float, float,
                               const juce::Slider::SliderStyle style, juce::Slider& s) override
        {
            if (style != juce::Slider::LinearHorizontal)
            {
                juce::LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, 0, 0, style, s);
                return;
            }

            const float cy = y + h * 0.5f;
            juce::Rectangle<float> track ((float) x, cy - 2.5f, (float) w, 5.0f);
            g.setColour (col::line);
            g.fillRoundedRectangle (track, 2.5f);

            const double range = s.getMaximum() - s.getMinimum();
            const float zeroProp = range > 0.0 ? (float) ((0.0 - s.getMinimum()) / range) : 0.0f;
            const float zeroX = x + juce::jlimit (0.0f, 1.0f, zeroProp) * w;
            const auto accent = s.findColour (juce::Slider::thumbColourId);

            juce::Rectangle<float> fill (juce::jmin (zeroX, sliderPos), cy - 2.5f,
                                         std::abs (sliderPos - zeroX), 5.0f);
            g.setColour (accent);
            g.fillRoundedRectangle (fill, 2.5f);

            g.setColour (accent);
            g.fillEllipse (sliderPos - 6.0f, cy - 6.0f, 12.0f, 12.0f);
            g.setColour (col::bgDeep);
            g.fillEllipse (sliderPos - 2.5f, cy - 2.5f, 5.0f, 5.0f);
        }

        //----------------------------------------------------------------------
        void drawComboBox (juce::Graphics& g, int w, int h, bool,
                           int, int, int, int, juce::ComboBox& box) override
        {
            juce::Rectangle<float> r (0.0f, 0.0f, (float) w, (float) h);
            g.setColour (col::panel2);
            g.fillRoundedRectangle (r.reduced (0.5f), 5.0f);
            g.setColour (box.findColour (juce::ComboBox::outlineColourId));
            g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);

            const float arrowX = (float) w - 16.0f;
            juce::Path p;
            p.addTriangle (arrowX, h * 0.42f - 2.0f, arrowX + 8.0f, h * 0.42f - 2.0f,
                           arrowX + 4.0f, h * 0.42f + 4.0f);
            g.setColour (box.findColour (juce::ComboBox::arrowColourId));
            g.fillPath (p);
        }

        void drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                   const juce::Colour& backgroundColour,
                                   bool, bool isDown) override
        {
            auto r = b.getLocalBounds().toFloat().reduced (0.5f);
            const bool on = b.getToggleState();
            auto base = on ? backgroundColour : col::panel2;
            if (isDown) base = base.brighter (0.15f);
            g.setColour (base);
            g.fillRoundedRectangle (r, 5.0f);
            g.setColour (on ? backgroundColour : col::line);
            g.drawRoundedRectangle (r, 5.0f, 1.0f);
        }
    };
}
