#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include "Parameters.h"
#include "VoskVoice.h"
#include "VoskSound.h"
#include "FxChain.h"
#include "Character.h"
#include "Tape.h"
#include "Scope.h"

//==============================================================================
//  VOSK — Stage 1 processor.
//
//  Poly mode uses a juce::Synthesiser (8 voices, JUCE voice stealing).
//  Mono / Legato are handled by a dedicated single voice driven directly from
//  a last-note-priority stack, so we get true legato (no envelope retrigger on
//  overlap) and exponential glide — neither of which JUCE's voice stealing
//  gives us for free.
//==============================================================================
class VoskAudioProcessor : public juce::AudioProcessor
{
public:
    VoskAudioProcessor();
    ~VoskAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VOSK"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Visualiser feeds (written on the audio thread, read by the editor).
    vosk::ScopeBuffer scopeBuffer;
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f };

    // Shared free-running LFO phase, for the editor's live LFO indicators.
    float getLfoPhase (int n) const noexcept
    {
        return (n >= 0 && n < 2) ? modInputs.lfoGlobalPhase[n] : 0.0f;
    }

    // On-screen keyboard (editor) -> synth.
    juce::MidiKeyboardState keyboardState;

private:
    void renderMono (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                     int numSamples, bool legato);
    void outputStage (juce::AudioBuffer<float>& buffer);
    void handleMonoMidi (const juce::MidiMessage& m, bool legato);
    void panic();

    static constexpr int kNumPolyVoices = 8;

    vosk::ParamPtrs params;
    VoskModInputs   modInputs;      // MIDI/host mod sources + shared LFO phase

    void updateModInputs (juce::MidiBuffer& midi, int numSamples);

    vosk::CharacterStage characterStage; // output drive/saturation (post-voice, pre-FX)
    vosk::fx::FxChain fxChain;      // chorus -> delay -> reverb (post-voice)
    vosk::TapeStage tapeStage;     // tape/VHS final color (post-FX)

    juce::Synthesiser synth;        // Poly
    VoskVoice monoVoice;            // Mono / Legato
    std::vector<int> noteStack;     // last-note-priority order
    float monoVelocity = 0.8f;

    int lastVoiceMode = -1;

    // Output safety: DC blocker state per channel.
    float dcX[2] { 0.0f, 0.0f };
    float dcY[2] { 0.0f, 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoskAudioProcessor)
};
