#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "ModMatrix.h"

//==============================================================================
//  VOSK — APVTS parameter IDs, layout, and a cached raw-pointer view.
//
//  Stage 1: oscillators + voicing + amp envelope only. The per-oscillator
//  unison IDs are DECLARED here so the parameter tree is stable across stages,
//  but they are inert (read by nothing in stage 1). Do not repurpose them.
//==============================================================================

namespace vosk::ids
{
    // Per-oscillator ID builder. n is 1-based (1, 2, 3).
    inline juce::String osc (int n, const char* suffix) { return "osc" + juce::String (n) + suffix; }
    // Per-LFO ID builder. n is 1-based (1, 2).
    inline juce::String lfo (int n, const char* suffix) { return "lfo" + juce::String (n) + suffix; }
    // Per-matrix-slot ID builder. n is 1-based (1..8).
    inline juce::String mod (int n, const char* suffix) { return "mod" + juce::String (n) + suffix; }

    // Per-oscillator suffixes.
    inline constexpr const char* kWave       = "wave";
    inline constexpr const char* kPW         = "pw";
    inline constexpr const char* kOct        = "oct";
    inline constexpr const char* kSemi       = "semi";
    inline constexpr const char* kFine       = "fine";
    inline constexpr const char* kLevel      = "level";
    inline constexpr const char* kPhaseMode  = "phasemode";
    // Reserved for stage 2 (unison) — declared but inert.
    inline constexpr const char* kUniVoices  = "univoices";
    inline constexpr const char* kUniDetune  = "unidetune";
    inline constexpr const char* kUniSpread  = "unispread";
    inline constexpr const char* kUniPhase   = "uniphase";

    // Globals.
    inline constexpr const char* kSyncOn        = "syncon";
    inline constexpr const char* kSyncDepth     = "syncdepth";
    inline constexpr const char* kPmAmount      = "pmamount";
    inline constexpr const char* kSubWave       = "subwave";
    inline constexpr const char* kSubOct        = "suboct";
    inline constexpr const char* kSubLevel      = "sublevel";
    inline constexpr const char* kNoiseLevel    = "noiselevel";
    inline constexpr const char* kNoiseColor    = "noisecolor";
    inline constexpr const char* kAmpAttack     = "ampattack";
    inline constexpr const char* kAmpDecay      = "ampdecay";
    inline constexpr const char* kAmpSustain    = "ampsustain";
    inline constexpr const char* kAmpRelease    = "amprelease";

    // Filter + pre-filter drive + filter envelope (Env2).
    inline constexpr const char* kFilterType    = "filtertype";
    inline constexpr const char* kCutoff        = "cutoff";
    inline constexpr const char* kResonance     = "resonance";
    inline constexpr const char* kDrive         = "drive";
    inline constexpr const char* kKeyTrack      = "keytrack";
    inline constexpr const char* kFilterEnvAmt  = "filterenvamount";
    inline constexpr const char* kFilterAttack  = "filterattack";
    inline constexpr const char* kFilterDecay   = "filterdecay";
    inline constexpr const char* kFilterSustain = "filtersustain";
    inline constexpr const char* kFilterRelease = "filterrelease";

    // Envelope velocity sensitivity (stage 4).
    inline constexpr const char* kAmpVelSens    = "ampvelsens";
    inline constexpr const char* kFilterVelSens = "filtervelsens";

    // LFO suffixes (per-LFO).
    inline constexpr const char* kLfoShape    = "shape";
    inline constexpr const char* kLfoSync     = "sync";
    inline constexpr const char* kLfoRate     = "rate";
    inline constexpr const char* kLfoDivision = "division";
    inline constexpr const char* kLfoTrigger  = "trigger";
    inline constexpr const char* kLfoPhase    = "phase";
    inline constexpr const char* kLfoFade     = "fade";

    // Matrix slot suffixes.
    inline constexpr const char* kModSource = "source";
    inline constexpr const char* kModDest   = "dest";
    inline constexpr const char* kModAmount = "amount";
    inline constexpr const char* kMasterVol     = "mastervol";
    inline constexpr const char* kVoiceMode     = "voicemode";
    inline constexpr const char* kGlideTime     = "glidetime";
    inline constexpr const char* kPitchBendRange = "pitchbendrange";
}

namespace vosk
{
    //==========================================================================
    //  Cached atomic pointers into the APVTS, so the audio thread never does
    //  a string lookup. Populated once via init().
    //==========================================================================
    struct ParamPtrs
    {
        // Per-oscillator (index 0..2 == osc 1..3).
        std::array<std::atomic<float>*, 3> oscWave      { {} };
        std::array<std::atomic<float>*, 3> oscPW        { {} };
        std::array<std::atomic<float>*, 3> oscOct       { {} };
        std::array<std::atomic<float>*, 3> oscSemi      { {} };
        std::array<std::atomic<float>*, 3> oscFine      { {} };
        std::array<std::atomic<float>*, 3> oscLevel     { {} };
        std::array<std::atomic<float>*, 3> oscPhaseMode { {} };
        std::array<std::atomic<float>*, 3> oscUniVoices { {} };
        std::array<std::atomic<float>*, 3> oscUniDetune { {} };
        std::array<std::atomic<float>*, 3> oscUniSpread { {} };
        std::array<std::atomic<float>*, 3> oscUniPhase  { {} };

        std::atomic<float>* syncOn        = nullptr;
        std::atomic<float>* syncDepth     = nullptr;
        std::atomic<float>* pmAmount      = nullptr;
        std::atomic<float>* subWave       = nullptr;
        std::atomic<float>* subOct        = nullptr;
        std::atomic<float>* subLevel      = nullptr;
        std::atomic<float>* noiseLevel    = nullptr;
        std::atomic<float>* noiseColor    = nullptr;
        std::atomic<float>* ampAttack     = nullptr;
        std::atomic<float>* ampDecay      = nullptr;
        std::atomic<float>* ampSustain    = nullptr;
        std::atomic<float>* ampRelease    = nullptr;
        std::atomic<float>* filterType    = nullptr;
        std::atomic<float>* cutoff        = nullptr;
        std::atomic<float>* resonance     = nullptr;
        std::atomic<float>* drive         = nullptr;
        std::atomic<float>* keyTrack      = nullptr;
        std::atomic<float>* filterEnvAmt  = nullptr;
        std::atomic<float>* filterAttack  = nullptr;
        std::atomic<float>* filterDecay   = nullptr;
        std::atomic<float>* filterSustain = nullptr;
        std::atomic<float>* filterRelease = nullptr;
        std::atomic<float>* ampVelSens    = nullptr;
        std::atomic<float>* filterVelSens = nullptr;

        // LFOs (index 0..1 == LFO 1..2).
        std::array<std::atomic<float>*, 2> lfoShape    { {} };
        std::array<std::atomic<float>*, 2> lfoSync     { {} };
        std::array<std::atomic<float>*, 2> lfoRate     { {} };
        std::array<std::atomic<float>*, 2> lfoDivision { {} };
        std::array<std::atomic<float>*, 2> lfoTrigger  { {} };
        std::array<std::atomic<float>*, 2> lfoPhase    { {} };
        std::array<std::atomic<float>*, 2> lfoFade     { {} };

        // Matrix (index 0..7 == slot 1..8).
        std::array<std::atomic<float>*, 8> modSource { {} };
        std::array<std::atomic<float>*, 8> modDest   { {} };
        std::array<std::atomic<float>*, 8> modAmount { {} };
        std::atomic<float>* masterVol     = nullptr;
        std::atomic<float>* voiceMode     = nullptr;
        std::atomic<float>* glideTime     = nullptr;
        std::atomic<float>* pitchBendRange = nullptr;

        void init (juce::AudioProcessorValueTreeState& s)
        {
            using namespace ids;
            for (int i = 0; i < 3; ++i)
            {
                const int n = i + 1;
                oscWave[i]      = s.getRawParameterValue (osc (n, kWave));
                oscPW[i]        = s.getRawParameterValue (osc (n, kPW));
                oscOct[i]       = s.getRawParameterValue (osc (n, kOct));
                oscSemi[i]      = s.getRawParameterValue (osc (n, kSemi));
                oscFine[i]      = s.getRawParameterValue (osc (n, kFine));
                oscLevel[i]     = s.getRawParameterValue (osc (n, kLevel));
                oscPhaseMode[i] = s.getRawParameterValue (osc (n, kPhaseMode));
                oscUniVoices[i] = s.getRawParameterValue (osc (n, kUniVoices));
                oscUniDetune[i] = s.getRawParameterValue (osc (n, kUniDetune));
                oscUniSpread[i] = s.getRawParameterValue (osc (n, kUniSpread));
                oscUniPhase[i]  = s.getRawParameterValue (osc (n, kUniPhase));
            }
            syncOn         = s.getRawParameterValue (kSyncOn);
            syncDepth      = s.getRawParameterValue (kSyncDepth);
            pmAmount       = s.getRawParameterValue (kPmAmount);
            subWave        = s.getRawParameterValue (kSubWave);
            subOct         = s.getRawParameterValue (kSubOct);
            subLevel       = s.getRawParameterValue (kSubLevel);
            noiseLevel     = s.getRawParameterValue (kNoiseLevel);
            noiseColor     = s.getRawParameterValue (kNoiseColor);
            ampAttack      = s.getRawParameterValue (kAmpAttack);
            ampDecay       = s.getRawParameterValue (kAmpDecay);
            ampSustain     = s.getRawParameterValue (kAmpSustain);
            ampRelease     = s.getRawParameterValue (kAmpRelease);
            filterType     = s.getRawParameterValue (kFilterType);
            cutoff         = s.getRawParameterValue (kCutoff);
            resonance      = s.getRawParameterValue (kResonance);
            drive          = s.getRawParameterValue (kDrive);
            keyTrack       = s.getRawParameterValue (kKeyTrack);
            filterEnvAmt   = s.getRawParameterValue (kFilterEnvAmt);
            filterAttack   = s.getRawParameterValue (kFilterAttack);
            filterDecay    = s.getRawParameterValue (kFilterDecay);
            filterSustain  = s.getRawParameterValue (kFilterSustain);
            filterRelease  = s.getRawParameterValue (kFilterRelease);
            ampVelSens     = s.getRawParameterValue (kAmpVelSens);
            filterVelSens  = s.getRawParameterValue (kFilterVelSens);

            for (int i = 0; i < 2; ++i)
            {
                const int n = i + 1;
                lfoShape[i]    = s.getRawParameterValue (lfo (n, kLfoShape));
                lfoSync[i]     = s.getRawParameterValue (lfo (n, kLfoSync));
                lfoRate[i]     = s.getRawParameterValue (lfo (n, kLfoRate));
                lfoDivision[i] = s.getRawParameterValue (lfo (n, kLfoDivision));
                lfoTrigger[i]  = s.getRawParameterValue (lfo (n, kLfoTrigger));
                lfoPhase[i]    = s.getRawParameterValue (lfo (n, kLfoPhase));
                lfoFade[i]     = s.getRawParameterValue (lfo (n, kLfoFade));
            }
            for (int i = 0; i < 8; ++i)
            {
                const int n = i + 1;
                modSource[i] = s.getRawParameterValue (mod (n, kModSource));
                modDest[i]   = s.getRawParameterValue (mod (n, kModDest));
                modAmount[i] = s.getRawParameterValue (mod (n, kModAmount));
            }
            masterVol      = s.getRawParameterValue (kMasterVol);
            voiceMode      = s.getRawParameterValue (kVoiceMode);
            glideTime      = s.getRawParameterValue (kGlideTime);
            pitchBendRange = s.getRawParameterValue (kPitchBendRange);
        }
    };

    //==========================================================================
    //  Build the full APVTS layout.
    //==========================================================================
    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace ids;
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        auto pid = [] (const juce::String& s) { return juce::ParameterID { s, 1 }; };

        const juce::StringArray waveChoices  { "Saw", "Pulse", "Triangle", "Sine" };
        const juce::StringArray phaseChoices { "Free", "Fixed" };
        const juce::StringArray uniPhaseChoices { "Random", "Fixed" };

        const juce::NormalisableRange<float> fineRange { -50.0f, 50.0f, 0.1f };
        const juce::NormalisableRange<float> pwRange   { 0.05f, 0.95f, 0.001f };

        for (int n = 1; n <= 3; ++n)
        {
            const juce::String pre = "Osc " + juce::String (n) + " ";

            layout.add (std::make_unique<juce::AudioParameterChoice> (
                pid (osc (n, kWave)), pre + "Wave", waveChoices, 0));

            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid (osc (n, kPW)), pre + "Pulse Width", pwRange, 0.5f));

            layout.add (std::make_unique<juce::AudioParameterInt> (
                pid (osc (n, kOct)), pre + "Octave", -2, 2, 0));

            layout.add (std::make_unique<juce::AudioParameterInt> (
                pid (osc (n, kSemi)), pre + "Semitone", -12, 12, 0));

            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid (osc (n, kFine)), pre + "Fine", fineRange, 0.0f));

            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid (osc (n, kLevel)), pre + "Level",
                juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
                n == 1 ? 0.8f : 0.0f));

            layout.add (std::make_unique<juce::AudioParameterChoice> (
                pid (osc (n, kPhaseMode)), pre + "Phase", phaseChoices, 0));

            // ---- Reserved for stage 2 (unison): declared, inert. ----
            layout.add (std::make_unique<juce::AudioParameterInt> (
                pid (osc (n, kUniVoices)), pre + "Uni Voices", 1, 7, 1));
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid (osc (n, kUniDetune)), pre + "Uni Detune",
                juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid (osc (n, kUniSpread)), pre + "Uni Spread",
                juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                pid (osc (n, kUniPhase)), pre + "Uni Phase", uniPhaseChoices, 0));
        }

        // ---- Sync (osc2 -> osc1) ----
        layout.add (std::make_unique<juce::AudioParameterBool> (
            pid (kSyncOn), "Hard Sync", false));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kSyncDepth), "Sync Depth",
            juce::NormalisableRange<float> { 0.0f, 36.0f, 0.01f }, 0.0f));

        // ---- Phase mod (osc3 -> osc1) ----
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kPmAmount), "PM Amount",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));

        // ---- Sub ----
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            pid (kSubWave), "Sub Wave", juce::StringArray { "Sine", "Square" }, 0));
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            pid (kSubOct), "Sub Octave", juce::StringArray { "-1", "-2" }, 0));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kSubLevel), "Sub Level",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));

        // ---- Noise ----
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kNoiseLevel), "Noise Level",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kNoiseColor), "Noise Color",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));

        // ---- Amp envelope ----
        const juce::NormalisableRange<float> adRange { 0.001f, 5.0f, 0.0001f, 0.3f };
        const juce::NormalisableRange<float> relRange { 0.001f, 8.0f, 0.0001f, 0.3f };
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kAmpAttack),  "Amp Attack",  adRange, 0.005f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kAmpDecay),   "Amp Decay",   adRange, 0.20f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kAmpSustain), "Amp Sustain",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.8f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kAmpRelease), "Amp Release", relRange, 0.30f));

        // ---- Filter + pre-filter drive ----
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            pid (kFilterType), "Filter Type",
            juce::StringArray { "Ladder", "Sallen-Key" }, 0));

        juce::NormalisableRange<float> cutoffRange { 20.0f, 20000.0f };
        cutoffRange.setSkewForCentre (1000.0f);
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kCutoff), "Cutoff", cutoffRange, 12000.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kResonance), "Resonance",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.1f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kDrive), "Drive",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kKeyTrack), "Key Tracking",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kFilterEnvAmt), "Filter Env Amount",
            juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f }, 0.0f));

        // ---- Filter envelope (Env2) ----
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kFilterAttack),  "Filter Attack",  adRange, 0.005f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kFilterDecay),   "Filter Decay",   adRange, 0.20f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kFilterSustain), "Filter Sustain",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kFilterRelease), "Filter Release", relRange, 0.30f));

        // ---- Envelope velocity sensitivity ----
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kAmpVelSens), "Amp Vel Sens",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kFilterVelSens), "Filter Vel Sens",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));

        // ---- LFOs ----
        const juce::StringArray lfoShapes   = modsys::lfoShapeNames();
        const juce::StringArray lfoTrigs    = modsys::lfoTriggerNames();
        const juce::StringArray lfoDivs     = modsys::lfoDivisionNames();
        juce::NormalisableRange<float> rateRange { 0.01f, 40.0f, 0.0f, 0.3f };
        for (int n = 1; n <= 2; ++n)
        {
            const juce::String pre = "LFO " + juce::String (n) + " ";
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                pid (lfo (n, kLfoShape)), pre + "Shape", lfoShapes, 0));
            layout.add (std::make_unique<juce::AudioParameterBool> (
                pid (lfo (n, kLfoSync)), pre + "Sync", false));
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid (lfo (n, kLfoRate)), pre + "Rate", rateRange, 2.0f));
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                pid (lfo (n, kLfoDivision)), pre + "Division", lfoDivs, 4)); // 1/4
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                pid (lfo (n, kLfoTrigger)), pre + "Trigger", lfoTrigs, 0));  // Free
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid (lfo (n, kLfoPhase)), pre + "Phase",
                juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid (lfo (n, kLfoFade)), pre + "Fade",
                juce::NormalisableRange<float> { 0.0f, 5.0f, 0.0f, 0.4f }, 0.0f));
        }

        // ---- Mod matrix (8 fixed slots) ----
        // Factory defaults wire the bass growl out of the box:
        //   Slot 1: Env2 -> Sync Depth, +0.5
        //   Slot 2: LFO1 -> Pitch (All), 0  (vibrato, ready to dial in)
        const juce::StringArray srcNames = modsys::sourceNames();
        const juce::StringArray dstNames = modsys::destNames();
        for (int n = 1; n <= 8; ++n)
        {
            int defSrc = (int) modsys::Source::None;
            int defDst = (int) modsys::Dest::None;
            float defAmt = 0.0f;
            if (n == 1) { defSrc = (int) modsys::Source::Env2; defDst = (int) modsys::Dest::SyncDepth; defAmt = 0.5f; }
            if (n == 2) { defSrc = (int) modsys::Source::LFO1; defDst = (int) modsys::Dest::PitchAll; defAmt = 0.0f; }

            const juce::String pre = "Mod " + juce::String (n) + " ";
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                pid (mod (n, kModSource)), pre + "Source", srcNames, defSrc));
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                pid (mod (n, kModDest)), pre + "Dest", dstNames, defDst));
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid (mod (n, kModAmount)), pre + "Amount",
                juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f }, defAmt));
        }

        // ---- Global ----
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kMasterVol), "Master Volume",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.7f));
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            pid (kVoiceMode), "Voice Mode",
            juce::StringArray { "Poly", "Mono", "Legato" }, 0));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            pid (kGlideTime), "Glide Time",
            juce::NormalisableRange<float> { 0.0f, 2.0f, 0.0001f, 0.4f }, 0.0f));
        layout.add (std::make_unique<juce::AudioParameterInt> (
            pid (kPitchBendRange), "Pitch Bend Range", 0, 24, 2));

        return layout;
    }
}
