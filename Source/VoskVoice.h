#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <memory>
#include "Parameters.h"
#include "DspHelpers.h"
#include "Filters.h"
#include "ModMatrix.h"
#include "VoskSound.h"

//==============================================================================
//  Per-block modulation inputs shared from the processor (not APVTS params):
//  MIDI/host state and the shared free-running LFO phase used by Free trigger
//  mode so all voices' LFOs stay phase-aligned.
//==============================================================================
struct VoskModInputs
{
    float  modWheel   = 0.0f;     // CC1, 0..1
    float  aftertouch = 0.0f;     // channel pressure, 0..1
    double bpm        = 120.0;    // host tempo (120 when no playhead)
    float  lfoGlobalPhase[2] { 0.0f, 0.0f };
};

//==============================================================================
//  VoskVoice — one polyphonic voice.
//
//  Stage 1: three PolyBLEP oscillators, sub, noise, hard sync (osc2 -> osc1)
//  with anti-aliased reset, phase mod (osc3 -> osc1), amp ADSR, glide.
//
//  Stage 2: each of the three oscillators is now an independent Szabo supersaw
//  unison bank (up to 7 detuned PolyBLEP voices, own count/detune/spread/phase),
//  summed in stereo. Sync uses osc2's center pitch as master and resets ALL of
//  osc1's unison voices; PM uses osc3's summed output to modulate ALL of osc1's
//  unison voices. Output is stereo (unison spread).
//
//  The class is a juce::SynthesiserVoice (Poly mode), and also exposes direct
//  note control for the processor's Mono/Legato engine.
//==============================================================================
class VoskVoice : public juce::SynthesiserVoice
{
public:
    static constexpr int kMaxUni = 7;

    // Oversampling factor for the per-voice drive+filter section. Single tunable
    // constant: 2 (default) or 4 (heavier CPU, cleaner self-oscillation / drive).
    // 8 voices * this oversampled filter is the synth's CPU hotspot.
    static constexpr int kOversampleFactor = 2;

    VoskVoice() = default;

    void setParams (const vosk::ParamPtrs* p) noexcept { params = p; }
    void setModInputs (const VoskModInputs* m) noexcept { modIn = m; }

    // Allocate the oversampler / scratch buffers (needs the host's max block).
    void prepare (double sampleRate, int maxBlockSize);

    float getOversamplingLatency() const noexcept
    {
        return oversampler != nullptr ? (float) oversampler->getLatencyInSamples() : 0.0f;
    }

    //== SynthesiserVoice interface (used by Poly mode) =======================
    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<VoskSound*> (s) != nullptr;
    }

    void setCurrentPlaybackSampleRate (double newRate) override
    {
        juce::SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
        if (newRate > 0.0)
            adsr.setSampleRate (newRate);
    }

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int currentPitchWheelPosition) override
    {
        setPitchWheel (currentPitchWheelPosition);
        noteStarted (midiNoteNumber, velocity, /*retrigger*/ true, /*glide*/ false);
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        noteStopped (allowTailOff);
        if (! allowTailOff)
            clearCurrentNote();
    }

    void pitchWheelMoved (int newValue) override { setPitchWheel (newValue); }
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    //== Direct control (used by Mono/Legato engine) =========================
    void noteStarted (int midiNoteNumber, float velocity, bool retrigger, bool glide);
    void noteStopped (bool allowTailOff);
    void setPitchWheel (int wheelValue) noexcept
    {
        bendNorm = (float) (wheelValue - 8192) / 8192.0f; // -1 .. ~+1
    }

    bool  isActive() const noexcept           { return adsr.isActive(); }
    float getCurrentBaseFreq() const noexcept { return currentBaseFreq; }

    void resetState() noexcept;

private:
    //== Per-oscillator unison bank ==========================================
    // Per-block layout derived from the unison params (Szabo selection/detune/
    // gain/pan). Recomputed once per block in updateBankConfigs().
    struct BankConfig
    {
        int   count = 1;                 // effective voice count (after poly cap)
        int   idx     [kMaxUni] { 3 };   // offset-table index per active voice
        float freqMul [kMaxUni] { 1.0f };// 1 + detune*offset
        float gain    [kMaxUni] { 1.0f };// center/side gain (power-normalised)
        float panL    [kMaxUni] { 0.70710678f };
        float panR    [kMaxUni] { 0.70710678f };
    };

    void updateBankConfigs (int voiceMode, const float* srcSnapshot) noexcept;
    void applyPhaseModeOnNoteOn() noexcept;

    // Advance/read LFO n (0..1); applies fade-in. S&H uses the voice RNG.
    float renderLfo (int n, float incr, int shape) noexcept;

    // osc1 unison voice render: PolyBLEP + PM read-offset + carried sync BLEP.
    float renderOsc1Voice (int k, vosk::Wave w, float pw, float dt, float pmOffset) noexcept;
    static float guardedBlep (float t, float dt, bool skipRising) noexcept;

    const vosk::ParamPtrs* params = nullptr;
    const VoskModInputs*   modIn = nullptr;

    BankConfig cfg[3];

    // Per-oscillator, per-unison-voice state.
    float phaseA[3][kMaxUni] { { 0.0f } };
    float triA  [3][kMaxUni] { { 0.0f } }; // triangle leaky-integrator states

    // osc1 hard-sync reset BLEP state (per unison voice).
    float blepNext1     [kMaxUni] { 0.0f };
    bool  syncSkipRising[kMaxUni] { false };

    // osc2 center pitch = the sync master (separate accumulator so even voice
    // counts, which have no center voice, still have a well-defined master).
    float masterPhase = 0.0f;

    // Sub oscillator (single, not unison).
    float subPhase = 0.0f;

    // Pitch / glide.
    float currentBaseFreq = 0.0f;
    float targetBaseFreq  = 440.0f;
    bool  glideActive     = false;

    float bendNorm = 0.0f;

    // Modulation sources latched at note-on.
    float velNorm      = 1.0f;        // velocity 0..1
    float keyTrackNorm = 0.0f;        // bipolar around C3, -1..1

    // Per-voice LFO state.
    float lfoPhase[2] { 0.0f, 0.0f };
    float lfoShVal[2] { 0.0f, 0.0f }; // sample & hold held value
    int   noteOnSamples = 0;          // for LFO fade-in

    // Last source values, for block-rate unison-detune/spread modulation
    // (those destinations were built block-rate in stage 2; we feed that hook).
    float lastEnv1 = 0.0f, lastEnv2 = 0.0f, lastLfo1 = 0.0f, lastLfo2 = 0.0f;

    // Matrix snapshot (read once per block).
    int   matSrc[8] { 0 };
    int   matDst[8] { 0 };
    float matAmt[8] { 0.0f };

    juce::ADSR adsr;                  // Env1 -> amplitude (stage 1)
    juce::ADSR::Parameters adsrParams;

    juce::ADSR filterEnv;             // Env2 -> cutoff (stage 3)
    juce::ADSR::Parameters filterEnvParams;

    juce::Random rng;
    float noiseLP = 0.0f;

    //== Drive + oversampled filter (per voice, stereo) =======================
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    double oversampledRate = 44100.0;

    vosk::LadderZDF ladder;
    vosk::Korg35    korg35;

    // Scratch + per-base-sample control signals (filled before the oversampled
    // drive/filter pass so cutoff/drive/env modulate per sample).
    juce::AudioBuffer<float> scratch;       // 2 ch, clean osc mix for the sub-block
    std::vector<float> cutoffArr;
    std::vector<float> driveArr;
    std::vector<float> resonanceArr;
    std::vector<float> panArr;
    std::vector<float> ampArr;

    static float computeCutoff (float baseHz, float keyTrack, float noteFreq,
                                float envAmount, float env2, float modOctaves) noexcept;
};
