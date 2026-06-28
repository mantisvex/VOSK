#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VoskAudioProcessor::VoskAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VOSK", vosk::createParameterLayout())
{
    params.init (apvts);

    // Poly engine.
    for (int i = 0; i < kNumPolyVoices; ++i)
    {
        auto* v = new VoskVoice();
        v->setParams (&params);
        v->setModInputs (&modInputs);
        synth.addVoice (v);
    }
    synth.addSound (new VoskSound());
    synth.setNoteStealingEnabled (true);

    // Mono / legato engine.
    monoVoice.setParams (&params);
    monoVoice.setModInputs (&modInputs);

    fxChain.setParams (&params);

    noteStack.reserve (128);
}

//==============================================================================
void VoskAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<VoskVoice*> (synth.getVoice (i)))
            v->prepare (sampleRate, samplesPerBlock);

    monoVoice.prepare (sampleRate, samplesPerBlock);
    monoVoice.resetState();

    fxChain.prepare (sampleRate);
    fxChain.reset();

    dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0.0f;

    // Per-voice oversampling adds a fixed latency; report it (all voices match).
    setLatencySamples (juce::roundToInt (monoVoice.getOversamplingLatency()));

    noteStack.clear();
    lastVoiceMode = -1;
}

bool VoskAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Synth: stereo (or mono) output, no input.
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return layouts.getMainInputChannelSet().isDisabled();
}

//==============================================================================
void VoskAudioProcessor::panic()
{
    synth.allNotesOff (0, false);
    monoVoice.noteStopped (false);
    monoVoice.resetState();
    noteStack.clear();
}

void VoskAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();
    const int numSamples = buffer.getNumSamples();

    updateModInputs (midiMessages, numSamples);

    const int mode = (int) params.voiceMode->load(); // 0 Poly, 1 Mono, 2 Legato

    if (mode != lastVoiceMode)
    {
        panic();                 // avoid stuck notes when switching engines
        lastVoiceMode = mode;
    }

    if (mode == 0)
    {
        synth.renderNextBlock (buffer, midiMessages, 0, numSamples);
    }
    else
    {
        renderMono (buffer, midiMessages, numSamples, /*legato*/ mode == 2);
    }

    // FX chain (post-voice, pre master trim): chorus -> delay -> reverb.
    fxChain.process (buffer, modInputs.bpm);

    buffer.applyGain (params.masterVol->load());

    outputStage (buffer);
}

//==============================================================================
//  Output safety + final glue: kill NaN/inf, block DC (asymmetric drive can
//  leave an offset that triggers host auto-mute), and gently soft-clip so
//  runaway resonance / self-oscillation can never exceed 0 dBFS. Below ~-0.7 dB
//  the signal is left untouched, so clean patches stay clean; pushed patches get
//  a touch of output grit (which suits the genre).
void VoskAudioProcessor::outputStage (juce::AudioBuffer<float>& buffer)
{
    const int numCh = juce::jmin (2, buffer.getNumChannels());
    const float th = 0.92f;
    const float inv = 1.0f - th;

    for (int ch = 0; ch < numCh; ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        float x1 = dcX[ch], y1 = dcY[ch];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = d[i];
            if (! std::isfinite (x)) x = 0.0f;

            // DC blocker (one-pole high-pass at a few Hz).
            const float y = x - x1 + 0.9995f * y1;
            x1 = x; y1 = y;

            // Soft clip above threshold only.
            float o = y;
            if (o >  th) o =  th + inv * std::tanh ((o - th) / inv);
            else if (o < -th) o = -th + inv * std::tanh ((o + th) / inv);
            d[i] = o;
        }

        dcX[ch] = x1; dcY[ch] = y1;
    }
}

//==============================================================================
//  Refresh the per-block modulation inputs shared with every voice: host tempo,
//  mod wheel (CC1) and channel aftertouch, plus advance the shared free-running
//  LFO phase so Free-trigger LFOs stay phase-aligned across voices/notes.
void VoskAudioProcessor::updateModInputs (juce::MidiBuffer& midi, int numSamples)
{
    // Host tempo (default 120 with no playhead, e.g. standalone).
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto t = pos->getBpm())
                bpm = *t;
    modInputs.bpm = bpm;

    // Mod wheel + aftertouch: take the last value seen this block (per-block).
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isController() && m.getControllerNumber() == 1)
            modInputs.modWheel = (float) m.getControllerValue() / 127.0f;
        else if (m.isChannelPressure())
            modInputs.aftertouch = (float) m.getChannelPressureValue() / 127.0f;
        else if (m.isAftertouch())
            modInputs.aftertouch = (float) m.getAfterTouchValue() / 127.0f;
    }

    // Advance the shared free-running LFO phase for next block. Mirrors the
    // voice's rate calculation so global and per-voice phases agree.
    const double sr = getSampleRate();
    if (sr > 0.0)
    {
        for (int n = 0; n < 2; ++n)
        {
            float hz;
            if (params.lfoSync[n]->load() > 0.5f)
            {
                const float beats = vosk::modsys::lfoDivisionBeats ((int) params.lfoDivision[n]->load());
                hz = (float) (bpm / 60.0) / juce::jmax (1.0e-4f, beats);
            }
            else
            {
                hz = params.lfoRate[n]->load();
            }
            const float incr = juce::jlimit (0.0f, 0.5f, hz / (float) sr);
            float p = modInputs.lfoGlobalPhase[n] + incr * (float) numSamples;
            p -= std::floor (p);
            modInputs.lfoGlobalPhase[n] = p;
        }
    }
}

//==============================================================================
//  Mono / Legato: split the block at MIDI event boundaries, drive the single
//  voice between events. Note logic = last-note priority.
void VoskAudioProcessor::renderMono (juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midi, int numSamples, bool legato)
{
    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, numSamples, meta.samplePosition);
        if (t > pos)
        {
            monoVoice.renderNextBlock (buffer, pos, t - pos);
            pos = t;
        }
        handleMonoMidi (meta.getMessage(), legato);
    }
    if (pos < numSamples)
        monoVoice.renderNextBlock (buffer, pos, numSamples - pos);
}

void VoskAudioProcessor::handleMonoMidi (const juce::MidiMessage& m, bool legato)
{
    if (m.isNoteOn())
    {
        const bool wasActive = ! noteStack.empty();
        noteStack.push_back (m.getNoteNumber());
        monoVelocity = m.getFloatVelocity();

        const bool glide = wasActive && params.glideTime->load() > 0.00001f;
        // Mono retriggers the envelope on every new note; Legato only when
        // nothing was already held (i.e. it's not an overlap).
        const bool retrigger = legato ? ! wasActive : true;

        monoVoice.noteStarted (m.getNoteNumber(), monoVelocity, retrigger, glide);
    }
    else if (m.isNoteOff())
    {
        const int n = m.getNoteNumber();
        noteStack.erase (std::remove (noteStack.begin(), noteStack.end(), n), noteStack.end());

        if (noteStack.empty())
        {
            monoVoice.noteStopped (true);
        }
        else
        {
            // Return to the most recently held note still down: glide, no retrigger.
            const bool glide = params.glideTime->load() > 0.00001f;
            monoVoice.noteStarted (noteStack.back(), monoVelocity, false, glide);
        }
    }
    else if (m.isPitchWheel())
    {
        monoVoice.setPitchWheel (m.getPitchWheelValue());
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        noteStack.clear();
        monoVoice.noteStopped (m.isAllSoundOff() ? false : true);
    }
}

//==============================================================================
juce::AudioProcessorEditor* VoskAudioProcessor::createEditor()
{
    return new VoskAudioProcessorEditor (*this);
}

void VoskAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VoskAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoskAudioProcessor();
}
