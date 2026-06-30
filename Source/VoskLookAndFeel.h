#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include "BinaryData.h"

//==============================================================================
//  VOSK look & feel — MantisVex darksynth: industrial condensed type, neon
//  rotary arcs with tick rings + glow, LED toggles, per-control accent colours.
//==============================================================================
namespace vosk::ui
{
    namespace col
    {
        inline const juce::Colour bg      { 0xff0a0b11 };
        inline const juce::Colour bgDeep  { 0xff05060c };
        inline const juce::Colour panel   { 0xff15171f };
        inline const juce::Colour panelHi { 0xff1c1f2b };
        inline const juce::Colour panel2  { 0xff0d0f16 };
        inline const juce::Colour line    { 0xff2c3142 };
        inline const juce::Colour text    { 0xffe2e6f2 };
        inline const juce::Colour dim     { 0xff828aa0 };

        inline const juce::Colour magenta { 0xffff2e88 };
        inline const juce::Colour cyan    { 0xff27d8ff };
        inline const juce::Colour amber   { 0xffffb13c };
        inline const juce::Colour green   { 0xff39e6a4 };
        inline const juce::Colour purple  { 0xff9b6cff };
    }

    // MantisVex type system — Rajdhani (SIL OFL), embedded so the UI is identical
    // on every machine regardless of installed system fonts.
    inline juce::Font font (float h, bool bold = false)
    {
        static const juce::Typeface::Ptr reg = juce::Typeface::createSystemTypefaceFor (
            BinaryData::VoskFontRegular_ttf, (size_t) BinaryData::VoskFontRegular_ttfSize);
        static const juce::Typeface::Ptr bld = juce::Typeface::createSystemTypefaceFor (
            BinaryData::VoskFontBold_ttf, (size_t) BinaryData::VoskFontBold_ttfSize);

        return juce::Font (juce::FontOptions().withTypeface (bold ? bld : reg).withHeight (h));
    }
    inline juce::Font fontKerned (float h, float kerning, bool bold = false)
    {
        return font (h, bold).withExtraKerningFactor (kerning);
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
            setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
            setColour (juce::TextButton::textColourOffId,  col::dim);
        }

        juce::Font getLabelFont (juce::Label&) override               { return fontKerned (14.0f, 0.04f); }
        juce::Font getComboBoxFont (juce::ComboBox&) override         { return font (15.0f); }
        juce::Font getPopupMenuFont() override                        { return font (16.0f); }
        juce::Font getTextButtonFont (juce::TextButton&, int) override{ return fontKerned (13.0f, 0.06f, true); }
        juce::Font getSliderPopupFont (juce::Slider&) override        { return font (15.0f); }

        //----------------------------------------------------------------------
        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                               float pos, float startAngle, float endAngle,
                               juce::Slider& s) override
        {
            auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
            const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
            const float cx = bounds.getCentreX();
            const float cy = bounds.getCentreY();
            const float lineW = radius * 0.15f;
            const float arcR = radius - lineW * 0.7f;
            const float angle = startAngle + pos * (endAngle - startAngle);
            const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);

            // Tick ring.
            const int ticks = 11;
            for (int i = 0; i < ticks; ++i)
            {
                const float a = startAngle + (float) i / (ticks - 1) * (endAngle - startAngle);
                const float r0 = radius + 1.0f, r1 = radius + 4.0f;
                const float sinA = std::sin (a), cosA = std::cos (a);
                const bool past = a <= angle + 0.001f;
                g.setColour (past ? accent.withAlpha (0.55f) : col::line);
                g.drawLine (cx + sinA * r0, cy - cosA * r0,
                            cx + sinA * r1, cy - cosA * r1, 1.4f);
            }

            // Track.
            juce::Path track;
            track.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
            g.setColour (col::line);
            g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

            const bool bipolar = s.getMinimum() < -0.0001 && s.getMaximum() > 0.0001;
            const float fromAngle = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

            juce::Path val;
            val.addCentredArc (cx, cy, arcR, arcR, 0.0f,
                               juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle), true);
            g.setColour (accent.withAlpha (0.20f));
            g.strokePath (val, juce::PathStrokeType (lineW * 2.4f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            g.setColour (accent);
            g.strokePath (val, juce::PathStrokeType (lineW, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

            // Body with bevel.
            const float kr = radius - lineW * 1.9f;
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillEllipse (cx - kr, cy - kr + 1.5f, kr * 2.0f, kr * 2.0f);
            juce::ColourGradient grad (col::panelHi.brighter (0.06f), cx, cy - kr,
                                       col::panel2.darker (0.45f),    cx, cy + kr, false);
            g.setGradientFill (grad);
            g.fillEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f);
            g.setColour (col::line.brighter (0.05f));
            g.drawEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f, 1.0f);

            // Pointer + glowing tip.
            const float px = cx + std::sin (angle) * (kr * 0.84f);
            const float py = cy - std::cos (angle) * (kr * 0.84f);
            const float ix = cx + std::sin (angle) * (kr * 0.28f);
            const float iy = cy - std::cos (angle) * (kr * 0.28f);
            g.setColour (accent.withAlpha (0.30f));
            g.fillEllipse (px - lineW * 1.4f, py - lineW * 1.4f, lineW * 2.8f, lineW * 2.8f);
            g.setColour (accent);
            g.drawLine (ix, iy, px, py, lineW * 0.8f);
            g.fillEllipse (px - lineW * 0.7f, py - lineW * 0.7f, lineW * 1.4f, lineW * 1.4f);
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
            g.setColour (col::panel2);
            g.fillRoundedRectangle (track, 2.5f);
            g.setColour (col::line);
            g.drawRoundedRectangle (track, 2.5f, 1.0f);

            const double range = s.getMaximum() - s.getMinimum();
            const float zeroProp = range > 0.0 ? (float) ((0.0 - s.getMinimum()) / range) : 0.0f;
            const float zeroX = x + juce::jlimit (0.0f, 1.0f, zeroProp) * w;
            const auto accent = s.findColour (juce::Slider::thumbColourId);

            juce::Rectangle<float> fill (juce::jmin (zeroX, sliderPos), cy - 2.5f,
                                         std::abs (sliderPos - zeroX), 5.0f);
            g.setColour (accent.withAlpha (0.85f));
            g.fillRoundedRectangle (fill, 2.5f);

            g.setColour (accent.withAlpha (0.3f));
            g.fillEllipse (sliderPos - 8.0f, cy - 8.0f, 16.0f, 16.0f);
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

            const float ax = (float) w - 15.0f;
            juce::Path p;
            p.addTriangle (ax, h * 0.42f - 2.0f, ax + 7.0f, h * 0.42f - 2.0f, ax + 3.5f, h * 0.42f + 3.5f);
            g.setColour (box.findColour (juce::ComboBox::arrowColourId));
            g.fillPath (p);
        }

        void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
        {
            label.setBounds (9, 1, box.getWidth() - 24, box.getHeight() - 2);
            label.setFont (font (15.0f));
        }

        //----------------------------------------------------------------------
        //  LED-style toggle.
        void drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                   const juce::Colour& backgroundColour,
                                   bool isOver, bool isDown) override
        {
            auto r = b.getLocalBounds().toFloat().reduced (0.5f);
            const bool toggle = b.getClickingTogglesState();
            const bool on = b.getToggleState();
            auto base = (toggle && on) ? backgroundColour.withAlpha (0.18f) : col::panel2;
            if (isDown) base = base.brighter (0.10f);
            else if (isOver) base = base.brighter (0.05f);
            g.setColour (base);
            g.fillRoundedRectangle (r, 5.0f);
            g.setColour ((toggle && on) ? backgroundColour : col::line);
            g.drawRoundedRectangle (r, 5.0f, (toggle && on) ? 1.4f : 1.0f);

            if (! toggle) return; // momentary buttons: no LED

            const float d = 6.0f;
            const float lx = r.getX() + 8.0f;
            const float ly = r.getCentreY() - d * 0.5f;
            if (on)
            {
                g.setColour (backgroundColour.withAlpha (0.4f));
                g.fillEllipse (lx - 2.0f, ly - 2.0f, d + 4.0f, d + 4.0f);
            }
            g.setColour (on ? backgroundColour : col::line.brighter (0.1f));
            g.fillEllipse (lx, ly, d, d);
        }

        void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
        {
            g.setColour (b.findColour (b.getToggleState() ? juce::TextButton::textColourOnId
                                                          : juce::TextButton::textColourOffId));
            g.setFont (getTextButtonFont (b, b.getHeight()));
            auto area = b.getClickingTogglesState() ? b.getLocalBounds().withTrimmedLeft (16)
                                                    : b.getLocalBounds();
            g.drawText (b.getButtonText(), area, juce::Justification::centred, false);
        }
    };
}
