#pragma once

#include <cmath>

//==============================================================================
//  VOSK band-limited oscillator math (PolyBLEP)
//
//  All oscillators here run on a normalized phase in [0, 1). The phase
//  increment per sample is dt = freq / sampleRate (also normalized), so dt is
//  literally "how much of a cycle we advance each sample".
//
//  ---------------------------------------------------------------------------
//  PolyBLEP — what and why
//  ---------------------------------------------------------------------------
//  A naive saw/square has instantaneous steps at its discontinuities. Those
//  steps contain energy above Nyquist, which folds back as inharmonic aliasing
//  ("foldover garbage"). PolyBLEP replaces the ideal step with a short, 2-sample
//  polynomial approximation of a band-limited step (a BLEP residual), evaluated
//  only in the 2 samples straddling the discontinuity.
//
//  polyBlep(t, dt) returns the residual for the canonical *rising* saw step of
//  height 2 (it is SUBTRACTED from a rising saw, ADDED to a falling saw). It is
//  active only when the phase t is within dt of a 0/1 boundary:
//     - just after the wrap (t < dt):       returns -(1 - t/dt)^2
//     - just before the wrap (t > 1 - dt):  returns  ( (t-1)/dt + 1 )^2
//  Elsewhere it is 0 (no correction needed mid-cycle).
//==============================================================================

namespace vosk
{
    constexpr float kTwoPi = 6.28318530717958647692f;

    enum class Wave { Saw = 0, Pulse = 1, Triangle = 2, Sine = 3 };

    // 2-sample PolyBLEP residual. See header comment for the sign convention.
    inline float polyBlep (float t, float dt) noexcept
    {
        if (t < dt)
        {
            t /= dt;                       // 0..1 across the sample after the wrap
            return t + t - t * t - 1.0f;   // = -(1 - t)^2
        }
        if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;           // -1..0 across the sample before the wrap
            return t * t + t + t + 1.0f;   // = (t + 1)^2
        }
        return 0.0f;
    }

    // Band-limited square / pulse with PolyBLEP on BOTH edges.
    //   lookupPh : phase used to decide which side of the duty cycle we are on
    //              (this is where phase-mod, if any, is applied)
    //   blepPh   : the un-modulated base phase, used to time the BLEP corrections
    //   pw       : pulse width / duty cycle in (0, 1)
    // For oscillators with no phase mod, lookupPh == blepPh.
    inline float pulseValue (float lookupPh, float blepPh, float pw, float dt) noexcept
    {
        float v = (lookupPh < pw) ? 1.0f : -1.0f;

        // Rising edge sits at the phase wrap (phase 0): upward step  -> ADD blep.
        v += polyBlep (blepPh, dt);

        // Falling edge sits at phase == pw: downward step -> SUBTRACT blep.
        // Shift so the falling edge lands on the 0/1 boundary polyBlep watches.
        float t2 = blepPh + (1.0f - pw);
        if (t2 >= 1.0f) t2 -= 1.0f;
        v -= polyBlep (t2, dt);

        return v;
    }

    // Naive (non-band-limited) value of a waveform at a given phase. Used to
    // measure the *true* size of a discontinuity (e.g. the step a hard-sync
    // reset introduces), which we then feed to a scaled BLEP correction.
    inline float naiveValue (Wave w, float ph, float pw) noexcept
    {
        switch (w)
        {
            case Wave::Saw:      return 1.0f - 2.0f * ph;                   // downward ramp, [-1, 1]
            case Wave::Pulse:    return (ph < pw) ? 1.0f : -1.0f;
            case Wave::Triangle: return 1.0f - 4.0f * std::abs (ph - 0.5f);
            case Wave::Sine:     return std::sin (kTwoPi * ph);
        }
        return 0.0f;
    }

    // Band-limited render of a "plain" oscillator (no sync, no phase mod):
    // used for osc2, osc3 and the sub. triState is the per-oscillator leaky
    // integrator state used by the Triangle waveform; pass a unique reference
    // per oscillator and call exactly once per output sample.
    //
    //  Saw:      downward ramp + PolyBLEP at the wrap.
    //  Pulse:    dual-edge PolyBLEP (see pulseValue).
    //  Triangle: leaky-integrate the band-limited square. The integrator turns
    //            the (already alias-free) square's steps into alias-free slope
    //            changes, so the triangle never aliases either. The 4x scale and
    //            the dt term normalise the integrated amplitude back to ~[-1, 1].
    //  Sine:     direct, no correction needed.
    inline float renderValue (Wave w, float ph, float pw, float dt, float& triState) noexcept
    {
        switch (w)
        {
            case Wave::Saw:
                return (1.0f - 2.0f * ph) + polyBlep (ph, dt);

            case Wave::Pulse:
                return pulseValue (ph, ph, pw, dt);

            case Wave::Triangle:
            {
                const float sq = pulseValue (ph, ph, 0.5f, dt);
                triState = dt * sq + (1.0f - dt) * triState; // one-pole leaky integrator
                return 4.0f * triState;
            }

            case Wave::Sine:
                return std::sin (kTwoPi * ph);
        }
        return 0.0f;
    }

    inline float midiToHz (float midiNote) noexcept
    {
        return 440.0f * std::exp2 ((midiNote - 69.0f) / 12.0f);
    }

    inline float wrap01 (float x) noexcept
    {
        x -= std::floor (x);
        return x;
    }
}

//==============================================================================
//  Szabo supersaw unison constants.
//
//  Detune polynomial, the 7-wide relative offset table, and the center/side
//  gain curves are reproduced VERBATIM from Adam Szabo, "How to Emulate the
//  Super Saw" (BTH thesis, 2010). Do not simplify them to an even spacing /
//  equal gains — the exact (non-linear) detune spread and the detune-dependent
//  gain split are precisely what separate a pro supersaw from a cheap one.
//==============================================================================
namespace vosk::szabo
{
    // detune knob x in [0,1] -> detune factor. Evaluated in double via Horner;
    // the high-order coefficients need the precision.
    inline float detuneCurve (float xf) noexcept
    {
        const double x = (double) xf;
        double r =  10028.7312891634;
        r = r * x -  50818.8652045924;
        r = r * x + 111363.4808729368;
        r = r * x - 138150.6761080548;
        r = r * x + 106649.6679158292;
        r = r * x -  53046.9642751875;
        r = r * x +  17019.9518580080;
        r = r * x -   3425.0836591318;
        r = r * x +    404.2703938388;
        r = r * x -     24.1878824391;
        r = r * x +      0.6717417634;
        r = r * x +      0.0030115596;
        return (float) r;
    }

    // Fixed relative voice offsets. Index 3 (0.0) is the center voice.
    inline constexpr float offsets[7] =
        { -0.11002313f, -0.06288439f, -0.01952356f, 0.0f, 0.01952356f, 0.06288439f, 0.11002313f };

    inline float centerGain (float d) noexcept { return -0.55366f * d + 0.99785f; }
    inline float sideGain   (float d) noexcept { return -0.73764f * d * d + 1.2841f * d + 0.044372f; }

    // Symmetric subset of offset-table indices for a given voice count (1..7).
    // Kept balanced around the center so the detune stays even around the pitch.
    //   1 -> [3]            2 -> [2,4]           3 -> [2,3,4]
    //   4 -> [1,2,4,5]      5 -> [1,2,3,4,5]     6 -> [0,1,2,4,5,6]
    //   7 -> [0..6]
    inline constexpr int selection[8][7] =
    {
        {  0,  0,  0,  0,  0,  0,  0 }, // 0 (unused)
        {  3,  0,  0,  0,  0,  0,  0 }, // 1
        {  2,  4,  0,  0,  0,  0,  0 }, // 2
        {  2,  3,  4,  0,  0,  0,  0 }, // 3
        {  1,  2,  4,  5,  0,  0,  0 }, // 4
        {  1,  2,  3,  4,  5,  0,  0 }, // 5
        {  0,  1,  2,  4,  5,  6,  0 }, // 6
        {  0,  1,  2,  3,  4,  5,  6 }  // 7
    };
}
