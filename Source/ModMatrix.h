#pragma once

#include <juce_core/juce_core.h>

//==============================================================================
//  VOSK modulation matrix — sources, destinations, and per-destination depth.
//
//  IMPORTANT (scope wall): destinations are synth/DSP parameters ONLY. There is
//  deliberately NO destination for LFO rate, envelope times, or matrix amounts,
//  so no modulation-of-modulation path can exist. Sources never feed sources.
//
//  The 5 macro sources are RESERVED for stage 6: present in the enum so the
//  matrix layout/automation IDs never change, but they output 0 until then.
//==============================================================================

namespace vosk::modsys
{
    enum class Source
    {
        None = 0,
        Env1, Env2, LFO1, LFO2,
        Velocity, KeyTrack, ModWheel, Aftertouch,
        Macro1, Macro2, Macro3, Macro4, HeroMacro,
        Count
    };

    enum class Dest
    {
        None = 0,
        PitchAll, Osc1Pitch, Osc2Pitch, Osc3Pitch,
        Osc1Level, Osc2Level, Osc3Level,
        Osc1PW, Osc2PW, Osc3PW,
        SyncDepth, PMAmount,
        Osc1UniDetune, Osc2UniDetune, Osc3UniDetune,
        Osc1UniSpread, Osc2UniSpread, Osc3UniSpread,
        Cutoff, Resonance, Drive,
        VoicePan,
        Count
    };

    inline constexpr int NumSources = (int) Source::Count;
    inline constexpr int NumDests   = (int) Dest::Count;
    inline constexpr int NumSlots   = 8;

    // Normalised matrix contribution (source*amount, ~[-1,1]) is multiplied by
    // this per-destination depth to reach the destination's native units:
    //   pitch dests / sync depth : semitones
    //   cutoff                   : octaves
    //   levels / pw / detune / spread / res / drive / pan : the param's own units
    inline constexpr float destDepth[NumDests] =
    {
        0.0f,                 // None
        24.0f,                // PitchAll  (semitones, ±2 oct)
        24.0f, 24.0f, 24.0f,  // OscNPitch (semitones)
        1.0f, 1.0f, 1.0f,     // OscNLevel
        0.45f, 0.45f, 0.45f,  // OscNPW
        36.0f,                // SyncDepth (semitones, full 0..36 reach)
        1.0f,                 // PMAmount
        1.0f, 1.0f, 1.0f,     // OscNUniDetune
        1.0f, 1.0f, 1.0f,     // OscNUniSpread
        8.0f,                 // Cutoff (octaves)
        1.0f,                 // Resonance
        1.0f,                 // Drive
        1.0f                  // VoicePan
    };

    inline juce::StringArray sourceNames()
    {
        return { "None", "Env 1", "Env 2", "LFO 1", "LFO 2",
                 "Velocity", "Key Track", "Mod Wheel", "Aftertouch",
                 "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Hero Macro" };
    }

    inline juce::StringArray destNames()
    {
        return { "None", "Pitch (All)", "Osc1 Pitch", "Osc2 Pitch", "Osc3 Pitch",
                 "Osc1 Level", "Osc2 Level", "Osc3 Level",
                 "Osc1 PW", "Osc2 PW", "Osc3 PW",
                 "Sync Depth", "PM Amount",
                 "Osc1 Uni Detune", "Osc2 Uni Detune", "Osc3 Uni Detune",
                 "Osc1 Uni Spread", "Osc2 Uni Spread", "Osc3 Uni Spread",
                 "Cutoff", "Resonance", "Drive", "Voice Pan" };
    }

    //==========================================================================
    //  LFO shapes + tempo divisions.
    //==========================================================================
    enum class LfoShape { Sine = 0, Triangle, SawUp, SawDown, Square, SampleHold, Count };

    inline juce::StringArray lfoShapeNames()
    {
        return { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S & H" };
    }

    inline juce::StringArray lfoTriggerNames() { return { "Free", "Key" }; }

    inline juce::StringArray lfoDivisionNames()
    {
        return { "1/1", "1/2", "1/2.", "1/2T", "1/4", "1/4.", "1/4T",
                 "1/8", "1/8.", "1/8T", "1/16", "1/16.", "1/16T", "1/32" };
    }

    // Length of one LFO cycle in beats (quarter notes) for each division index.
    inline float lfoDivisionBeats (int idx) noexcept
    {
        static const float beats[] =
        {
            4.0f,          // 1/1
            2.0f, 3.0f, 4.0f / 3.0f,   // 1/2, dotted, triplet
            1.0f, 1.5f, 2.0f / 3.0f,   // 1/4, dotted, triplet
            0.5f, 0.75f, 1.0f / 3.0f,  // 1/8, dotted, triplet
            0.25f, 0.375f, 1.0f / 6.0f,// 1/16, dotted, triplet
            0.125f                     // 1/32
        };
        const int n = (int) (sizeof (beats) / sizeof (float));
        return beats[juce::jlimit (0, n - 1, idx)];
    }

    // Naive bipolar LFO shape value at phase ph in [0,1). S&H is handled by the
    // caller (it needs persistent state + an RNG). LFOs are sub-audio, so no
    // band-limiting is required.
    inline float lfoShapeValue (int shape, float ph) noexcept
    {
        switch ((LfoShape) shape)
        {
            case LfoShape::Sine:     return std::sin (6.28318530718f * ph);
            case LfoShape::Triangle: return 1.0f - 4.0f * std::abs (ph - 0.5f);
            case LfoShape::SawUp:    return 2.0f * ph - 1.0f;
            case LfoShape::SawDown:  return 1.0f - 2.0f * ph;
            case LfoShape::Square:   return ph < 0.5f ? 1.0f : -1.0f;
            default:                 return 0.0f; // SampleHold / Count handled by caller
        }
    }
}
