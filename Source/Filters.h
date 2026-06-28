#pragma once

#include <cmath>

//==============================================================================
//  VOSK filters + pre-filter drive.
//
//  Both filters are zero-delay-feedback / TPT designs (Vadim Zavalishin, "The
//  Art of VA Filter Design"). Each is built from TPT one-pole cells whose output
//  for input x is  y = G*x + s/(1+g),  with  g = tan(pi*fc/fs),  G = g/(1+g),
//  and the trapezoidal state update  s <- 2*y - s.
//
//   - LadderZDF : Moog-style 4-pole (24 dB/oct), global resonance feedback with
//                 a tanh saturator in the loop ("Moog warmth"). Self-oscillates
//                 at k ≈ 4 independent of cutoff (each pole gives 45° at fc, so
//                 the loop hits 180°/unity gain there).
//   - Korg35    : MS-20 / Korg35-style Sallen-Key low-pass (Pirkle VA topology):
//                 a series one-pole LP feeding a resonant SK core (one-pole LP +
//                 one-pole HP in the feedback) with a saturating element in the
//                 resonance path. Gets deliberately dirty/screaming at high res.
//
//  Both keep per-channel state so a voice can run them in stereo. They run
//  INSIDE the per-voice oversampled region, so the feedback nonlinearities and
//  self-oscillation don't alias.
//==============================================================================

namespace vosk
{
    //--------------------------------------------------------------------------
    //  Pre-filter drive: asymmetric soft clip. The DC bias makes it asymmetric
    //  so it generates EVEN harmonics too (dirtier / more "analog"), and the
    //  bias term is subtracted back out so it doesn't leave a DC offset. Makeup
    //  gain keeps loudness roughly constant as drive rises while the harmonic
    //  content keeps growing.
    inline float driveShape (float x, float amount) noexcept
    {
        if (amount < 1.0e-4f)
            return x;

        const float pre  = 1.0f + amount * 8.0f;      // input gain into the clip
        const float bias = 0.18f * amount;            // asymmetry
        const float y    = std::tanh (pre * x + bias) - std::tanh (bias);
        const float makeup = 1.0f / (1.0f + amount * 3.0f);
        return y * makeup;
    }

    //--------------------------------------------------------------------------
    //  Moog-style 4-pole ZDF ladder (Zavalishin TPT). Saturated feedback uses
    //  the previous output (1 sample at the oversampled rate -> negligible),
    //  which keeps it unconditionally stable and self-oscillating.
    struct LadderZDF
    {
        float s[2][4] { { 0.0f } };
        float zfb[2]  { 0.0f, 0.0f };
        float g = 0.0f, G = 0.0f;
        float k = 0.0f, comp = 1.0f;

        void reset() noexcept
        {
            for (int c = 0; c < 2; ++c)
            {
                zfb[c] = 0.0f;
                for (int i = 0; i < 4; ++i) s[c][i] = 0.0f;
            }
        }

        void setCutoff (float fc, float fs) noexcept
        {
            g = std::tan (3.14159265358979f * fc / fs);
            G = g / (1.0f + g);
        }

        // resonance in [0,1] -> feedback k in [0,4]; comp restores passband level.
        // comp is gentle (not 1+k) so resonance + pre-filter drive don't stack into
        // a runaway: self-oscillation is still reachable near the top, but bounded.
        void setResonance (float r) noexcept
        {
            k = r * 3.8f;
            comp = 1.0f + 0.35f * k;
        }

        float process (int ch, float x) noexcept
        {
            const float a = 1.0f / (1.0f + g);
            const float fb = k * std::tanh (zfb[ch]);   // saturated resonance
            const float in0 = comp * x - fb;

            const float S1 = s[ch][0] * a;
            const float S2 = s[ch][1] * a;
            const float S3 = s[ch][2] * a;
            const float S4 = s[ch][3] * a;

            const float y1 = G * in0 + S1; s[ch][0] = 2.0f * y1 - s[ch][0];
            const float y2 = G * y1  + S2; s[ch][1] = 2.0f * y2 - s[ch][1];
            const float y3 = G * y2  + S3; s[ch][2] = 2.0f * y3 - s[ch][2];
            const float y4 = G * y3  + S4; s[ch][3] = 2.0f * y4 - s[ch][3];

            zfb[ch] = y4;
            return y4;
        }
    };

    //--------------------------------------------------------------------------
    //  Korg35 / MS-20-style Sallen-Key low-pass (Pirkle topology).
    //  Series LP1 -> resonant SK core (LP2 + HP1 feedback) with a tanh in the
    //  resonance path. R (the loop gain) approaches 1 for self-oscillation; the
    //  tanh turns runaway into a bounded, screaming limit cycle.
    struct Korg35
    {
        float sA[2] { 0.0f, 0.0f }; // LP1 (series)
        float sB[2] { 0.0f, 0.0f }; // LP2 (loop)
        float sC[2] { 0.0f, 0.0f }; // HP1 (loop)
        float g = 0.0f, G = 0.0f;
        float R = 0.0f;

        void reset() noexcept
        {
            for (int c = 0; c < 2; ++c) { sA[c] = sB[c] = sC[c] = 0.0f; }
        }

        void setCutoff (float fc, float fs) noexcept
        {
            g = std::tan (3.14159265358979f * fc / fs);
            G = g / (1.0f + g);
        }

        // resonance in [0,1] -> loop gain R. Capped below 1 and the saturated
        // feedback (Kc) is clamped low so the Korg35 screams without a Larsen
        // runaway. Self-oscillation is reachable but stays a bounded limit cycle.
        void setResonance (float r) noexcept { R = r * 0.92f; }

        float process (int ch, float x) noexcept
        {
            const float a = 1.0f / (1.0f + g);  // = 1 - G

            // Series LP1.
            const float yA = G * x + sA[ch] * a;
            sA[ch] = 2.0f * yA - sA[ch];

            // SK core: linear ZDF estimate of the loop output.
            const float betaB = sB[ch] * a;
            const float betaC = sC[ch] * a;
            const float alpha0 = 1.0f / (1.0f - R);
            const float yLin = alpha0 * (G * yA + betaB - R * sC[ch]);

            // Saturate the resonance (HP1) feedback, then resolve open-loop.
            const float hp = a * (yLin - sC[ch]);          // HP1 output estimate
            float Kc = R / (G * a);                          // = R*(1+g)^2/g
            if (Kc > 10.0f) Kc = 10.0f;                       // clamp feedback gain (anti-Larsen)
            const float u = yA + Kc * std::tanh (hp);

            const float y = G * u + betaB;                   // LP2 output
            sB[ch] = 2.0f * y - sB[ch];
            const float lpC = G * y + betaC;                 // HP1's LP twin
            sC[ch] = 2.0f * lpC - sC[ch];

            return y;
        }
    };
}
