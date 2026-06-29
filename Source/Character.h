#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include "Parameters.h"

//==============================================================================
//  VOSK output character / drive stage (post-voice, pre-FX).
//
//  This is where the "record" grit lives — the design doc parked output
//  saturation as a v2 candidate; this is it. Four voicings, a post tone tilt and
//  a wet/dry mix. The pre-filter per-voice drive mangles harmonics into the
//  filter; THIS stage colours the whole summed bus on the way out.
//
//   Tube  : asymmetric soft clip (warm, even harmonics)
//   Diode : harder asymmetric clip (aggressive)
//   Fold  : sine wavefolder (metallic / screaming as it's pushed)
//   Crush : bit + sample-rate reduction (lo-fi digital)
//==============================================================================
namespace vosk
{
    class CharacterStage
    {
    public:
        void setParams (const ParamPtrs* p) noexcept { params = p; }
        void prepare (double sampleRate) { sr = sampleRate; reset(); }
        void reset() { tone[0] = tone[1] = 0.0f; shVal[0] = shVal[1] = 0.0f; shCnt[0] = shCnt[1] = 0; }

        void process (juce::AudioBuffer<float>& b)
        {
            if (params == nullptr || params->charOn->load() < 0.5f)
                return;

            const float amt  = params->charDrive->load();
            const int   mode = (int) params->charMode->load();
            const float tCut = 1200.0f * std::pow (15.0f, params->charTone->load()); // 1.2k..18k
            const float coeff = 1.0f - std::exp (-6.28318530718f * juce::jmin (tCut, 0.45f * (float) sr) / (float) sr);
            const float mix  = params->charMix->load();
            const int   hold = 1 + (int) (amt * amt * 14.0f); // crush sample-rate reduction

            const int nch = juce::jmin (2, b.getNumChannels());
            for (int ch = 0; ch < nch; ++ch)
            {
                float* d = b.getWritePointer (ch);
                for (int i = 0; i < b.getNumSamples(); ++i)
                {
                    const float x = d[i];
                    float w = shape (x, mode, amt, ch, hold);

                    // Post tone tilt (one-pole low-pass).
                    tone[ch] += coeff * (w - tone[ch]);
                    w = tone[ch];

                    d[i] = x * (1.0f - mix) + w * mix;
                }
            }
        }

    private:
        float shape (float x, int mode, float amt, int ch, int hold) noexcept
        {
            switch (mode)
            {
                case 1: // Diode — harder, asymmetric
                {
                    const float g = 1.0f + amt * 11.0f;
                    const float y = g * x;
                    const float s = (y >= 0.0f) ? std::tanh (y) : std::tanh (y * 0.7f) * 0.85f;
                    return s / (1.0f + amt * 0.8f);
                }
                case 2: // Fold — sine wavefolder
                {
                    const float g = 1.0f + amt * 5.0f;
                    return std::sin (g * x * 1.4f) * (0.8f - 0.2f * amt);
                }
                case 3: // Crush — bit + sample-rate reduction
                {
                    const float bits = juce::jmap (amt, 0.0f, 1.0f, 16.0f, 3.0f);
                    const float levels = std::pow (2.0f, bits - 1.0f);
                    const float q = std::round (x * levels) / levels;
                    if (++shCnt[ch] >= hold) { shVal[ch] = q; shCnt[ch] = 0; }
                    return shVal[ch];
                }
                default: // Tube — asymmetric soft clip
                {
                    const float g = 1.0f + amt * 6.0f;
                    const float bias = 0.12f * amt;
                    const float y = std::tanh (g * x + bias) - std::tanh (bias);
                    return y / (1.0f + amt * 0.7f);
                }
            }
        }

        const ParamPtrs* params = nullptr;
        double sr = 44100.0;
        float tone[2] { 0.0f, 0.0f };
        float shVal[2] { 0.0f, 0.0f };
        int   shCnt[2] { 0, 0 };
    };
}
