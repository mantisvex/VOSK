#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>
#include "Parameters.h"
#include "ModMatrix.h"

//==============================================================================
//  VOSK FX chain (stage 5). Global, post-voice, fixed order, each bypassable:
//      1. Juno-style BBD chorus
//      2. Tempo-synced stereo delay (tone in feedback, ping-pong)
//      3. Dark reverb (Freeverb topology, dark-biased)
//
//  Order is fixed by design (synthwave never reorders these). Processes the
//  summed stereo voice bus; master volume is applied after, by the processor.
//==============================================================================
namespace vosk::fx
{
    //--------------------------------------------------------------------------
    //  Fractional-delay circular buffer.
    struct DelayLine
    {
        std::vector<float> buf;
        int size = 0, writePos = 0;

        void prepare (int maxSamples)
        {
            size = juce::jmax (4, maxSamples);
            buf.assign ((size_t) size, 0.0f);
            writePos = 0;
        }
        void clear() { std::fill (buf.begin(), buf.end(), 0.0f); writePos = 0; }

        void write (float x) { buf[(size_t) writePos] = x; if (++writePos >= size) writePos = 0; }

        float readFrac (float delaySamples) const
        {
            float rp = (float) writePos - delaySamples;
            while (rp < 0.0f) rp += (float) size;
            const int i0 = (int) rp;
            const float fr = rp - (float) i0;
            const int i1 = (i0 + 1 >= size) ? 0 : i0 + 1;
            return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * fr;
        }
    };

    //==========================================================================
    //  Juno-style BBD chorus: a short modulated delay per channel, with the L/R
    //  LFOs in anti-phase for width. Mode II runs faster + deeper than Mode I.
    class Chorus
    {
    public:
        void prepare (double sampleRate)
        {
            sr = sampleRate;
            const int maxS = (int) (0.05 * sr) + 8;
            dl[0].prepare (maxS); dl[1].prepare (maxS);
            lfoPhase = 0.0f;
        }
        void reset() { dl[0].clear(); dl[1].clear(); lfoPhase = 0.0f; }

        void process (juce::AudioBuffer<float>& b, float rate, float depth, float mix, int mode)
        {
            const int n = b.getNumSamples();
            const int nch = juce::jmin (2, b.getNumChannels());
            float* L = b.getWritePointer (0);
            float* R = nch > 1 ? b.getWritePointer (1) : nullptr;

            const float rateHz = rate * (mode == 1 ? 1.6f : 1.0f);
            const float baseMs = 7.5f;
            const float depthMs = (mode == 1 ? 5.0f : 3.0f) * depth;
            const float incr = rateHz / (float) sr;
            const float wetG = 0.7f * mix;
            const float dryG = 1.0f - 0.5f * mix;

            for (int i = 0; i < n; ++i)
            {
                const float lfo = std::sin (kTwoPi * lfoPhase);

                const float inL = L[i];
                const float dsL = (baseMs + lfo * depthMs) * 0.001f * (float) sr;
                const float wetL = dl[0].readFrac (dsL);
                dl[0].write (inL);
                L[i] = inL * dryG + wetL * wetG;

                if (R != nullptr)
                {
                    const float inR = R[i];
                    const float dsR = (baseMs - lfo * depthMs) * 0.001f * (float) sr; // anti-phase
                    const float wetR = dl[1].readFrac (dsR);
                    dl[1].write (inR);
                    R[i] = inR * dryG + wetR * wetG;
                }

                lfoPhase += incr;
                if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            }
        }

    private:
        static constexpr float kTwoPi = 6.28318530718f;
        DelayLine dl[2];
        double sr = 44100.0;
        float lfoPhase = 0.0f;
    };

    //==========================================================================
    //  Tempo-synced stereo delay: one-pole tone filter in the feedback path,
    //  optional ping-pong (cross-fed channels).
    class Delay
    {
    public:
        void prepare (double sampleRate)
        {
            sr = sampleRate;
            const int maxS = (int) (2.5 * sr) + 8;
            dl[0].prepare (maxS); dl[1].prepare (maxS);
            tone[0] = tone[1] = 0.0f;
        }
        void reset() { dl[0].clear(); dl[1].clear(); tone[0] = tone[1] = 0.0f; }

        void process (juce::AudioBuffer<float>& b, float timeSamples, float feedback,
                      float mix, float toneHz, bool pingpong)
        {
            const int n = b.getNumSamples();
            const int nch = juce::jmin (2, b.getNumChannels());
            float* L = b.getWritePointer (0);
            float* R = nch > 1 ? b.getWritePointer (1) : L;

            const float ts = juce::jlimit (1.0f, (float) dl[0].size - 4.0f, timeSamples);
            const float toneCoeff = 1.0f - std::exp (-6.28318530718f * toneHz / (float) sr);

            for (int i = 0; i < n; ++i)
            {
                const float inL = L[i];
                const float inR = R[i];
                const float dLv = dl[0].readFrac (ts);
                const float dRv = dl[1].readFrac (ts);

                tone[0] += toneCoeff * (dLv - tone[0]);
                tone[1] += toneCoeff * (dRv - tone[1]);

                const float fL = pingpong ? tone[1] : tone[0];
                const float fR = pingpong ? tone[0] : tone[1];

                dl[0].write (inL + fL * feedback);
                dl[1].write (inR + fR * feedback);

                L[i] = inL * (1.0f - mix) + dLv * mix;
                if (nch > 1) R[i] = inR * (1.0f - mix) + dRv * mix;
            }
        }

    private:
        DelayLine dl[2];
        double sr = 44100.0;
        float tone[2] { 0.0f, 0.0f };
    };

    //==========================================================================
    //  Dark reverb — Freeverb topology (8 combs + 4 allpass per channel) with
    //  size scaling the delay lengths, decay the comb feedback, a dark-biased
    //  damping LP in each comb plus a dark output LP, and a pre-delay line.
    class Reverb
    {
    public:
        void prepare (double sampleRate)
        {
            sr = sampleRate;
            srRatio = (float) (sr / 44100.0);
            for (int i = 0; i < NC; ++i)
            {
                const int maxL = (int) ((float) combTune[i] * srRatio * 1.25f) + spread + 16;
                combL[i].prepare (maxL); combR[i].prepare (maxL);
            }
            for (int i = 0; i < NA; ++i)
            {
                const int maxL = (int) ((float) apTune[i] * srRatio) + spread + 16;
                apL[i].prepare (maxL); apR[i].prepare (maxL);
            }
            pd[0].prepare ((int) (0.25 * sr) + 8);
            pd[1].prepare ((int) (0.25 * sr) + 8);
            outLP[0] = outLP[1] = 0.0f;
        }

        void reset()
        {
            for (int i = 0; i < NC; ++i) { combL[i].clear(); combR[i].clear(); }
            for (int i = 0; i < NA; ++i) { apL[i].clear(); apR[i].clear(); }
            pd[0].clear(); pd[1].clear();
            outLP[0] = outLP[1] = 0.0f;
        }

        void process (juce::AudioBuffer<float>& b, float size, float decay,
                      float damping, float predelayMs, float mix)
        {
            const int n = b.getNumSamples();
            const int nch = juce::jmin (2, b.getNumChannels());
            float* L = b.getWritePointer (0);
            float* R = nch > 1 ? b.getWritePointer (1) : L;

            const float sc = juce::jmap (size, 0.0f, 1.0f, 0.45f, 1.10f);
            const float fb = 0.70f + 0.279f * decay;                 // tail length
            const float damp = juce::jlimit (0.0f, 0.95f, 0.15f + 0.80f * damping);
            for (int i = 0; i < NC; ++i)
            {
                combL[i].setLen ((int) ((float) combTune[i] * srRatio * sc));
                combR[i].setLen ((int) ((float) (combTune[i] + spread) * srRatio * sc));
            }
            for (int i = 0; i < NA; ++i)
            {
                apL[i].setLen ((int) ((float) apTune[i] * srRatio));
                apR[i].setLen ((int) ((float) (apTune[i] + spread) * srRatio));
            }

            const float pdS = juce::jmax (0.0f, predelayMs * 0.001f * (float) sr);
            const float fc = juce::jmap (damping, 0.0f, 1.0f, 9000.0f, 2500.0f); // dark bias
            const float outDamp = 1.0f - std::exp (-6.28318530718f * fc / (float) sr);
            const float inGain = 0.015f;

            for (int s = 0; s < n; ++s)
            {
                const float dryL = L[s];
                const float dryR = nch > 1 ? R[s] : L[s];

                const float pL = pdS > 0.5f ? pd[0].readFrac (pdS) : dryL;
                const float pR = pdS > 0.5f ? pd[1].readFrac (pdS) : dryR;
                pd[0].write (dryL); pd[1].write (dryR);

                const float inp = (pL + pR) * inGain;

                float accL = 0.0f, accR = 0.0f;
                for (int i = 0; i < NC; ++i)
                {
                    accL += combL[i].process (inp, fb, damp);
                    accR += combR[i].process (inp, fb, damp);
                }
                for (int i = 0; i < NA; ++i)
                {
                    accL = apL[i].process (accL, 0.5f);
                    accR = apR[i].process (accR, 0.5f);
                }

                outLP[0] += outDamp * (accL - outLP[0]);
                outLP[1] += outDamp * (accR - outLP[1]);

                L[s] = dryL * (1.0f - mix) + outLP[0] * mix;
                if (nch > 1) R[s] = dryR * (1.0f - mix) + outLP[1] * mix;
            }
        }

    private:
        struct Comb
        {
            std::vector<float> buf; int idx = 0, len = 1; float store = 0.0f;
            void prepare (int maxLen) { buf.assign ((size_t) juce::jmax (1, maxLen), 0.0f); idx = 0; len = (int) buf.size(); store = 0.0f; }
            void setLen (int L) { len = juce::jlimit (1, (int) buf.size(), L); if (idx >= len) idx = 0; }
            void clear() { std::fill (buf.begin(), buf.end(), 0.0f); idx = 0; store = 0.0f; }
            float process (float x, float fb, float damp)
            {
                const float y = buf[(size_t) idx];
                store = y * (1.0f - damp) + store * damp;
                buf[(size_t) idx] = x + store * fb;
                if (++idx >= len) idx = 0;
                return y;
            }
        };
        struct Allpass
        {
            std::vector<float> buf; int idx = 0, len = 1;
            void prepare (int maxLen) { buf.assign ((size_t) juce::jmax (1, maxLen), 0.0f); idx = 0; len = (int) buf.size(); }
            void setLen (int L) { len = juce::jlimit (1, (int) buf.size(), L); if (idx >= len) idx = 0; }
            void clear() { std::fill (buf.begin(), buf.end(), 0.0f); idx = 0; }
            float process (float x, float fb)
            {
                const float y = buf[(size_t) idx];
                const float out = -x + y;
                buf[(size_t) idx] = x + y * fb;
                if (++idx >= len) idx = 0;
                return out;
            }
        };

        static constexpr int NC = 8, NA = 4, spread = 23;
        static constexpr int combTune[NC] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static constexpr int apTune[NA]   = { 556, 441, 341, 225 };

        Comb combL[NC], combR[NC];
        Allpass apL[NA], apR[NA];
        DelayLine pd[2];
        float outLP[2] { 0.0f, 0.0f };
        double sr = 44100.0;
        float srRatio = 1.0f;
    };

    //==========================================================================
    //  The full chain, reading its parameters from the APVTS pointer view.
    class FxChain
    {
    public:
        void setParams (const ParamPtrs* p) noexcept { params = p; }

        void prepare (double sampleRate)
        {
            sr = sampleRate;
            chorus.prepare (sampleRate);
            delay.prepare (sampleRate);
            reverb.prepare (sampleRate);
        }

        void reset() { chorus.reset(); delay.reset(); reverb.reset(); }

        void process (juce::AudioBuffer<float>& buffer, double bpm)
        {
            if (params == nullptr) return;

            if (params->chorusOn->load() > 0.5f)
                chorus.process (buffer,
                                params->chorusRate->load(),
                                params->chorusDepth->load(),
                                params->chorusMix->load(),
                                (int) params->chorusMode->load());

            if (params->delayOn->load() > 0.5f)
            {
                float timeSamples;
                if (params->delaySync->load() > 0.5f)
                {
                    const float beats = modsys::lfoDivisionBeats ((int) params->delayDivision->load());
                    const double seconds = beats * (60.0 / juce::jmax (20.0, bpm));
                    timeSamples = (float) (seconds * sr);
                }
                else
                {
                    timeSamples = (float) (params->delayTime->load() * 0.001 * sr);
                }
                delay.process (buffer, timeSamples,
                               params->delayFeedback->load(),
                               params->delayMix->load(),
                               params->delayTone->load(),
                               params->delayPingpong->load() > 0.5f);
            }

            if (params->reverbOn->load() > 0.5f)
                reverb.process (buffer,
                                params->reverbSize->load(),
                                params->reverbDecay->load(),
                                params->reverbDamping->load(),
                                params->reverbPredelay->load(),
                                params->reverbMix->load());
        }

    private:
        const ParamPtrs* params = nullptr;
        double sr = 44100.0;
        Chorus chorus;
        Delay  delay;
        Reverb reverb;
    };
}
