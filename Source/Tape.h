#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include "Parameters.h"
#include "FxChain.h"   // vosk::fx::DelayLine

//==============================================================================
//  VOSK tape / VHS stage — the synthwave nostalgia color. Final output stage
//  (after FX, before master): wow & flutter pitch warble, tape saturation, a
//  vintage HF rolloff, and hiss. This is a signature genre flavour most synths
//  don't bundle well.
//==============================================================================
namespace vosk
{
    class TapeStage
    {
    public:
        void setParams (const ParamPtrs* p) noexcept { params = p; }

        void prepare (double sampleRate)
        {
            sr = sampleRate;
            dl[0].prepare ((int) (0.05 * sr) + 8);
            dl[1].prepare ((int) (0.05 * sr) + 8);
            reset();
        }

        void reset()
        {
            dl[0].clear(); dl[1].clear();
            wowPhase = flutterPhase = 0.0f;
            lp[0] = lp[1] = 0.0f;
        }

        void process (juce::AudioBuffer<float>& b)
        {
            if (params == nullptr || params->tapeOn->load() < 0.5f)
                return;

            const float wow  = params->tapeWow->load();
            const float sat  = params->tapeSat->load();
            const float hiss = params->tapeHiss->load() * 0.03f;
            const float tone = params->tapeTone->load();

            const float baseMs      = 10.0f;
            const float wowDepthMs  = wow * 4.0f;
            const float flutDepthMs = wow * 0.8f;
            const float wowInc      = 0.7f  / (float) sr;
            const float flutInc     = 6.5f  / (float) sr;

            const float fc = 3500.0f * std::pow (5.1f, tone); // ~3.5k..18k
            const float coeff = 1.0f - std::exp (-6.28318530718f * juce::jmin (fc, 0.45f * (float) sr) / (float) sr);

            const float satG = 1.0f + sat * 4.0f;
            const float satMk = 1.0f / (1.0f + sat * 0.6f);

            const int nch = juce::jmin (2, b.getNumChannels());
            float* d0 = b.getWritePointer (0);
            float* d1 = nch > 1 ? b.getWritePointer (1) : d0;

            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                // Correlated wow+flutter delay (whole image warbles together).
                const float dms = baseMs
                                + std::sin (6.28318530718f * wowPhase) * wowDepthMs
                                + std::sin (6.28318530718f * flutterPhase) * flutDepthMs;
                const float ds = dms * 0.001f * (float) sr;

                for (int ch = 0; ch < nch; ++ch)
                {
                    float* d = (ch == 0) ? d0 : d1;
                    const float x = d[i];
                    float w = dl[ch].readFrac (ds);
                    dl[ch].write (x);

                    w = std::tanh (satG * w) * satMk;           // tape saturation/compression
                    lp[ch] += coeff * (w - lp[ch]);              // vintage HF rolloff
                    w = lp[ch];
                    w += (rng.nextFloat() * 2.0f - 1.0f) * hiss; // hiss
                    d[i] = w;
                }

                wowPhase += wowInc;          if (wowPhase     >= 1.0f) wowPhase     -= 1.0f;
                flutterPhase += flutInc;     if (flutterPhase >= 1.0f) flutterPhase -= 1.0f;
            }
        }

    private:
        const ParamPtrs* params = nullptr;
        double sr = 44100.0;
        fx::DelayLine dl[2];
        float wowPhase = 0.0f, flutterPhase = 0.0f;
        float lp[2] { 0.0f, 0.0f };
        juce::Random rng;
    };
}
