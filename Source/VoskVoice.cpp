#include "VoskVoice.h"

using namespace vosk;

//==============================================================================
void VoskVoice::prepare (double sampleRate, int maxBlockSize)
{
    setCurrentPlaybackSampleRate (sampleRate);   // also sets adsr sample rate
    filterEnv.setSampleRate (sampleRate);

    // factor order: 2x -> 1, 4x -> 2.
    const int order = (kOversampleFactor >= 4) ? 2 : 1;
    oversampledRate = sampleRate * (double) (1 << order);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        2, order,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true);
    oversampler->initProcessing ((size_t) juce::jmax (1, maxBlockSize));

    scratch.setSize (2, juce::jmax (1, maxBlockSize), false, false, true);
    cutoffArr.assign    ((size_t) maxBlockSize, 0.0f);
    driveArr.assign     ((size_t) maxBlockSize, 0.0f);
    resonanceArr.assign ((size_t) maxBlockSize, 0.0f);
    panArr.assign       ((size_t) maxBlockSize, 0.0f);
    ampArr.assign       ((size_t) maxBlockSize, 0.0f);

    ladder.reset();
    korg35.reset();
}

// Effective cutoff = base * 2^(keyTrack*octavesFromRef + envAmount*8*env2).
// One place, reading every source per-sample, so stage 4's LFOs / matrix /
// sync-depth modulation just add more terms here without a refactor.
float VoskVoice::computeCutoff (float baseHz, float keyTrack, float noteFreq,
                                float envAmount, float env2, float modOctaves) noexcept
{
    constexpr float refFreq = 261.6255653f; // C4
    const float keyOct = keyTrack * std::log2 (juce::jmax (1.0e-6f, noteFreq) / refFreq);
    const float envOct = envAmount * 8.0f * env2;
    return baseHz * std::exp2 (keyOct + envOct + modOctaves);
}

//==============================================================================
void VoskVoice::resetState() noexcept
{
    for (int o = 0; o < 3; ++o)
        for (int k = 0; k < kMaxUni; ++k)
        {
            phaseA[o][k] = 0.0f;
            triA[o][k]   = 0.0f;
            drift[o][k]    = 0.0f;
            driftMul[o][k] = 1.0f;
        }

    for (int k = 0; k < kMaxUni; ++k)
    {
        blepNext1[k] = 0.0f;
        syncSkipRising[k] = false;
    }

    masterPhase = 0.0f;
    subPhase = 0.0f;
    noiseLP = 0.0f;
    lfoPhase[0] = lfoPhase[1] = 0.0f;
    lfoShVal[0] = lfoShVal[1] = 0.0f;
    noteOnSamples = 0;
    lastEnv1 = lastEnv2 = lastLfo1 = lastLfo2 = 0.0f;
    adsr.reset();
    filterEnv.reset();
    ladder.reset();
    korg35.reset();
}

//==============================================================================
//  Set the unison voice phases on note-on according to the two phase switches:
//   - uniPhase Random : reseed every unison voice with a random phase (smeared
//                       transient). Always reseeds, in every retrigger.
//   - uniPhase Fixed  : tight transient. Then the stage-1 phaseMode decides
//                       retrigger behaviour:
//                          phaseMode Fixed -> all voices to phase 0
//                          phaseMode Free  -> carry phases over (no reset)
void VoskVoice::applyPhaseModeOnNoteOn() noexcept
{
    if (params == nullptr)
        return;

    for (int o = 0; o < 3; ++o)
    {
        const bool freeMode  = params->oscPhaseMode[o]->load() < 0.5f; // {Free, Fixed}
        const bool uniRandom = params->oscUniPhase[o]->load()  < 0.5f; // {Random, Fixed}

        if (uniRandom)
        {
            for (int k = 0; k < kMaxUni; ++k)
            {
                phaseA[o][k] = rng.nextFloat();
                triA[o][k]   = 0.0f;
            }
        }
        else if (! freeMode) // uni Fixed + phaseMode Fixed -> hard reset to 0
        {
            for (int k = 0; k < kMaxUni; ++k)
            {
                phaseA[o][k] = 0.0f;
                triA[o][k]   = 0.0f;
            }
        }
        // uni Fixed + Free: carry phases over (do nothing).
    }

    // Sync scratch + master reset (master follows osc2's free/fixed choice).
    for (int k = 0; k < kMaxUni; ++k)
    {
        blepNext1[k] = 0.0f;
        syncSkipRising[k] = false;
    }
    if (params->oscPhaseMode[1]->load() >= 0.5f) // osc2 Fixed
        masterPhase = 0.0f;

    // Keep the sub locked to the note for consistent low end.
    subPhase = 0.0f;
}

//==============================================================================
void VoskVoice::noteStarted (int midiNoteNumber, float velocity, bool retrigger, bool glide)
{
    // Latch velocity + key tracking as modulation sources (per note-on).
    velNorm = velocity;
    keyTrackNorm = juce::jlimit (-1.0f, 1.0f, (float) (midiNoteNumber - 48) / 48.0f); // C3 = 0
    targetBaseFreq = midiToHz ((float) midiNoteNumber);

    if (glide && currentBaseFreq > 0.0f)
        glideActive = true;
    else
    {
        currentBaseFreq = targetBaseFreq;
        glideActive = false;
    }

    if (retrigger)
    {
        applyPhaseModeOnNoteOn();
        if (params != nullptr)
        {
            adsrParams.attack  = params->ampAttack->load();
            adsrParams.decay   = params->ampDecay->load();
            adsrParams.sustain = params->ampSustain->load();
            adsrParams.release = params->ampRelease->load();
            adsr.setParameters (adsrParams);

            filterEnvParams.attack  = params->filterAttack->load();
            filterEnvParams.decay   = params->filterDecay->load();
            filterEnvParams.sustain = params->filterSustain->load();
            filterEnvParams.release = params->filterRelease->load();
            filterEnv.setParameters (filterEnvParams);
        }
        adsr.noteOn();
        filterEnv.noteOn();
        noteOnSamples = 0;

        // LFO retrigger: Key mode resets phase to the LFO's start-phase offset;
        // Free mode aligns to the shared global free-running phase so voices stay
        // phase-coherent.
        if (params != nullptr)
        {
            for (int n = 0; n < 2; ++n)
            {
                const bool keyTrig = params->lfoTrigger[n]->load() > 0.5f;
                const float startPhase = params->lfoPhase[n]->load();
                if (keyTrig)
                    lfoPhase[n] = startPhase;
                else if (modIn != nullptr)
                    lfoPhase[n] = vosk::wrap01 (modIn->lfoGlobalPhase[n]);
                lfoShVal[n] = rng.nextFloat() * 2.0f - 1.0f;
            }
        }

        // Clear filter state on a fresh (retriggered) note so a stolen poly
        // voice doesn't carry the previous note's resonance ring as a click.
        ladder.reset();
        korg35.reset();
        if (oversampler != nullptr)
            oversampler->reset();
    }
}

void VoskVoice::noteStopped (bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
        filterEnv.noteOff();
    }
    else
    {
        adsr.reset();
        filterEnv.reset();
    }
}

//==============================================================================
//  Build each oscillator's Szabo bank for this block: symmetric voice subset,
//  per-voice detune multiplier, center/side gain (power-normalised so loudness
//  is roughly stable across detune and voice count), and constant-power pan.
void VoskVoice::updateBankConfigs (int voiceMode, const float* srcSnapshot) noexcept
{
    using namespace vosk::modsys;
    const float halfPi = juce::MathConstants<float>::halfPi;

    // Sum matrix contributions for one destination using the block-start source
    // snapshot. Unison detune/spread were built block-rate in stage 2, so we feed
    // that hook here (block rate) rather than per-sample — no refactor of stage 2.
    auto modFor = [this, srcSnapshot] (Dest dest)
    {
        float sum = 0.0f;
        for (int s = 0; s < 8; ++s)
            if (matSrc[s] != (int) Source::None && matDst[s] == (int) dest)
                sum += srcSnapshot[matSrc[s]] * matAmt[s];
        return sum * destDepth[(int) dest];
    };

    const Dest detuneDest[3] { Dest::Osc1UniDetune, Dest::Osc2UniDetune, Dest::Osc3UniDetune };
    const Dest spreadDest[3] { Dest::Osc1UniSpread, Dest::Osc2UniSpread, Dest::Osc3UniSpread };

    // Hero ("ROT") gesture widens unison detune on all oscillators.
    const float heroDetune = params->hero->load() * 0.5f;

    for (int o = 0; o < 3; ++o)
    {
        const int rawVoices = juce::jlimit (1, kMaxUni, (int) params->oscUniVoices[o]->load());

        // Poly CPU cap: poly -> max 4 per oscillator; mono/legato -> full 7.
        const int eff = (voiceMode == 0) ? juce::jmin (rawVoices, 4) : rawVoices;

        const float d   = juce::jlimit (0.0f, 1.0f, params->oscUniDetune[o]->load() + modFor (detuneDest[o]) + heroDetune);
        const float det = szabo::detuneCurve (d);
        const float spread = juce::jlimit (0.0f, 1.0f, params->oscUniSpread[o]->load() + modFor (spreadDest[o]));

        auto& c = cfg[o];
        c.count = eff;

        float sumSq = 0.0f;
        for (int k = 0; k < eff; ++k)
        {
            const int idx = szabo::selection[eff][k];
            c.idx[k] = idx;
            c.freqMul[k] = 1.0f + det * szabo::offsets[idx];

            const float g = (idx == 3) ? szabo::centerGain (d) : szabo::sideGain (d);
            c.gain[k] = g;
            sumSq += g * g;

            // Pan symmetrically by index across [-1, +1]; center (idx 3) stays 0.
            const float panPos = ((float) (idx - 3) / 3.0f) * spread;
            const float ang = (panPos + 1.0f) * halfPi * 0.5f; // (panPos+1)/2 * (pi/2)
            c.panL[k] = std::cos (ang);
            c.panR[k] = std::sin (ang);
        }

        const float norm = (sumSq > 0.0f) ? 1.0f / std::sqrt (sumSq) : 1.0f;
        for (int k = 0; k < eff; ++k)
            c.gain[k] *= norm;
    }
}

//==============================================================================
float VoskVoice::guardedBlep (float t, float dt, bool skipRising) noexcept
{
    if (t < dt)
    {
        if (skipRising)
            return 0.0f;
        const float x = t / dt;
        return x + x - x * x - 1.0f;
    }
    if (t > 1.0f - dt)
    {
        const float x = (t - 1.0f) / dt;
        return x * x + x + x + 1.0f;
    }
    return 0.0f;
}

float VoskVoice::renderOsc1Voice (int k, Wave w, float pw, float dt, float pmOffset) noexcept
{
    const float ph   = phaseA[0][k];
    const float look = wrap01 (ph + pmOffset);
    const bool  skip = syncSkipRising[k];

    float v = 0.0f;
    switch (w)
    {
        case Wave::Saw:
            v = (1.0f - 2.0f * look) + guardedBlep (ph, dt, skip);
            break;

        case Wave::Pulse:
        {
            v = (look < pw) ? 1.0f : -1.0f;
            v += guardedBlep (ph, dt, skip);
            float t2 = ph + (1.0f - pw);
            if (t2 >= 1.0f) t2 -= 1.0f;
            v -= polyBlep (t2, dt);
            break;
        }

        case Wave::Triangle:
        {
            float sq = (look < 0.5f) ? 1.0f : -1.0f;
            sq += guardedBlep (ph, dt, skip);
            float t2 = ph + 0.5f;
            if (t2 >= 1.0f) t2 -= 1.0f;
            sq -= polyBlep (t2, dt);
            triA[0][k] = dt * sq + (1.0f - dt) * triA[0][k];
            v = 4.0f * triA[0][k];
            break;
        }

        case Wave::Sine:
            v = std::sin (kTwoPi * look);
            break;
    }

    v += blepNext1[k];
    blepNext1[k] = 0.0f;
    syncSkipRising[k] = false;
    return v;
}

//==============================================================================
//  Render one LFO sample (bipolar -1..1), advance its phase, apply fade-in.
//  Depth is left entirely to the matrix slot amount (no separate LFO depth).
float VoskVoice::renderLfo (int n, float incr, int shape) noexcept
{
    using namespace vosk::modsys;

    const float ph = wrap01 (lfoPhase[n] + params->lfoPhase[n]->load());
    const float value = ((LfoShape) shape == LfoShape::SampleHold)
                          ? lfoShVal[n]
                          : lfoShapeValue (shape, ph);

    lfoPhase[n] += incr;
    if (lfoPhase[n] >= 1.0f)
    {
        lfoPhase[n] -= 1.0f;
        lfoShVal[n] = rng.nextFloat() * 2.0f - 1.0f; // new S&H value each cycle
    }

    // Fade-in from note-on (per-LFO fade time).
    float fade = 1.0f;
    const float fadeTime = params->lfoFade[n]->load();
    if (fadeTime > 1.0e-4f)
    {
        const float t = (float) noteOnSamples / (float) getSampleRate();
        fade = juce::jlimit (0.0f, 1.0f, t / fadeTime);
    }

    return value * fade;
}

//==============================================================================
void VoskVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                 int startSample, int numSamples)
{
    if (params == nullptr || ! adsr.isActive())
        return;

    const double sr = getSampleRate();
    if (sr <= 0.0)
        return;
    const float sampleRate = (float) sr;

    adsrParams.attack  = params->ampAttack->load();
    adsrParams.decay   = params->ampDecay->load();
    adsrParams.sustain = params->ampSustain->load();
    adsrParams.release = params->ampRelease->load();
    adsr.setParameters (adsrParams);

    const int voiceMode = (int) params->voiceMode->load();

    // ---- Modulation matrix snapshot (read once per block) ------------------
    for (int i = 0; i < 8; ++i)
    {
        matSrc[i] = (int) params->modSource[i]->load();
        matDst[i] = (int) params->modDest[i]->load();
        matAmt[i] = params->modAmount[i]->load();
    }

    // Per-block source values (MIDI / host / macros) shared by all destinations.
    const float modWheel   = modIn != nullptr ? modIn->modWheel   : 0.0f;
    const float aftertouch = modIn != nullptr ? modIn->aftertouch : 0.0f;
    const double bpm       = modIn != nullptr ? modIn->bpm        : 120.0;
    const float macro1 = params->macro[0]->load();
    const float macro2 = params->macro[1]->load();
    const float macro3 = params->macro[2]->load();
    const float macro4 = params->macro[3]->load();
    const float hero   = params->hero->load();

    // Source snapshot for the block-rate unison detune/spread modulation. Uses
    // the previous block's last env/LFO values (one block latent, inaudible).
    float srcSnapshot[vosk::modsys::NumSources] = { 0.0f };
    {
        using namespace vosk::modsys;
        srcSnapshot[(int) Source::Env1]       = lastEnv1;
        srcSnapshot[(int) Source::Env2]       = lastEnv2;
        srcSnapshot[(int) Source::LFO1]       = lastLfo1;
        srcSnapshot[(int) Source::LFO2]       = lastLfo2;
        srcSnapshot[(int) Source::Velocity]   = velNorm;
        srcSnapshot[(int) Source::KeyTrack]   = keyTrackNorm;
        srcSnapshot[(int) Source::ModWheel]   = modWheel;
        srcSnapshot[(int) Source::Aftertouch] = aftertouch;
        srcSnapshot[(int) Source::Macro1]     = macro1;
        srcSnapshot[(int) Source::Macro2]     = macro2;
        srcSnapshot[(int) Source::Macro3]     = macro3;
        srcSnapshot[(int) Source::Macro4]     = macro4;
        srcSnapshot[(int) Source::HeroMacro]  = hero;
    }

    updateBankConfigs (voiceMode, srcSnapshot);

    // Analog drift: a slow bounded random walk per active osc-voice (±5 cents)
    // so unison/oscillators breathe instead of sitting perfectly static.
    for (int o = 0; o < 3; ++o)
        for (int k = 0; k < cfg[o].count; ++k)
        {
            drift[o][k] = juce::jlimit (-5.0f, 5.0f,
                                        drift[o][k] + (rng.nextFloat() * 2.0f - 1.0f) * 0.7f);
            driftMul[o][k] = std::exp2 (drift[o][k] / 1200.0f);
        }

    // ---- Per-block parameter snapshot --------------------------------------
    const Wave  w1 = (Wave) (int) params->oscWave[0]->load();
    const Wave  w2 = (Wave) (int) params->oscWave[1]->load();
    const Wave  w3 = (Wave) (int) params->oscWave[2]->load();
    const float pw1 = params->oscPW[0]->load();
    const float pw2 = params->oscPW[1]->load();
    const float pw3 = params->oscPW[2]->load();
    const float lvl1 = params->oscLevel[0]->load();
    const float lvl2 = params->oscLevel[1]->load();
    const float lvl3 = params->oscLevel[2]->load();

    auto ratioFor = [this] (int i)
    {
        const float oct  = params->oscOct[i]->load();
        const float semi = params->oscSemi[i]->load();
        const float fine = params->oscFine[i]->load();
        return std::exp2 (oct + semi / 12.0f + fine / 1200.0f);
    };
    const float ratio1 = ratioFor (0);
    const float ratio2 = ratioFor (1);
    const float ratio3 = ratioFor (2);

    const bool  subSine = params->subWave->load() < 0.5f;
    const float subOctSel = params->subOct->load() < 0.5f ? -1.0f : -2.0f;
    const float subRatio = std::exp2 (subOctSel);
    const float subLvl = params->subLevel->load();

    const float noiseLvl = params->noiseLevel->load();
    const float noiseColor = params->noiseColor->load();
    const float noiseFc = juce::jmap (noiseColor, 0.0f, 1.0f, 18000.0f, 300.0f);
    const float noiseCoeff = 1.0f - std::exp (-kTwoPi * noiseFc / sampleRate);
    const float noiseMakeup = 1.0f + 3.0f * noiseColor;

    const bool  syncOn = params->syncOn->load() > 0.5f;
    const int   bendRange = (int) params->pitchBendRange->load();
    const float bendFactor = std::exp2 (bendNorm * (float) bendRange / 12.0f);

    const float glideTime = params->glideTime->load();
    const float glideCoeff = (glideTime > 0.00001f)
                               ? std::exp (-1.0f / (glideTime * sampleRate))
                               : 0.0f;

    auto limitDt = [sampleRate] (float f) { return juce::jlimit (0.0f, 0.49f, f / sampleRate); };

    // ---- Filter / drive / Env2 per-block snapshot --------------------------
    filterEnvParams.attack  = params->filterAttack->load();
    filterEnvParams.decay   = params->filterDecay->load();
    filterEnvParams.sustain = params->filterSustain->load();
    filterEnvParams.release = params->filterRelease->load();
    filterEnv.setParameters (filterEnvParams);

    const float baseCutoff = params->cutoff->load();
    const float keyTrack   = params->keyTrack->load();
    const float filterEnvAmt = params->filterEnvAmt->load();
    const float baseResonance = params->resonance->load();
    const float baseDrive  = params->drive->load();
    const bool  useKorg35  = params->filterType->load() > 0.5f;

    // ---- Modulation per-block setup ----------------------------------------
    const float baseSyncDepth = params->syncDepth->load();
    const float basePmAmount  = params->pmAmount->load();

    const float ampVelSens    = params->ampVelSens->load();
    const float filterVelSens = params->filterVelSens->load();
    const float velFactorAmp  = (1.0f - ampVelSens)    + ampVelSens    * velNorm;
    const float velFactorFilt = (1.0f - filterVelSens) + filterVelSens * velNorm;

    // LFO rate (cycles/sample) + shape per LFO. Sync mode derives Hz from BPM.
    float lfoIncr[2] = { 0.0f, 0.0f };
    int   lfoShapeIdx[2] = { 0, 0 };
    for (int n = 0; n < 2; ++n)
    {
        lfoShapeIdx[n] = (int) params->lfoShape[n]->load();
        float hz;
        if (params->lfoSync[n]->load() > 0.5f)
        {
            const float beats = vosk::modsys::lfoDivisionBeats ((int) params->lfoDivision[n]->load());
            hz = (float) (bpm / 60.0) / juce::jmax (1.0e-4f, beats);
        }
        else
        {
            hz = params->lfoRate[n]->load();
        }
        lfoIncr[n] = juce::jlimit (0.0f, 0.5f, hz / sampleRate);
    }

    const float kSqrtHalf = 0.70710678f; // center-pan for mono content (sub/noise)

    // ========================================================================
    //  PHASE A: render the clean stereo oscillator mix into the scratch buffer
    //  and capture the per-sample control signals (amp env, cutoff, drive).
    // ========================================================================
    float* sL = scratch.getWritePointer (0);
    float* sR = scratch.getWritePointer (1);

    for (int s = 0; s < numSamples; ++s)
    {
        // -------- Glide (exponential, log-frequency domain) ------------------
        if (glideActive)
        {
            const float lc = std::log2 (currentBaseFreq);
            const float lt = std::log2 (targetBaseFreq);
            const float ln = lt + (lc - lt) * glideCoeff;
            currentBaseFreq = std::exp2 (ln);
            if (std::abs (ln - lt) < 1.0e-4f)
            {
                currentBaseFreq = targetBaseFreq;
                glideActive = false;
            }
        }

        const float noteFreq = currentBaseFreq * bendFactor;

        // -------- Modulation sources (per sample) ----------------------------
        using namespace vosk::modsys;
        const float env1  = adsr.getNextSample();              // Env1 (advance once)
        const float env2  = filterEnv.getNextSample();         // Env2
        const float env1s = env1 * velFactorAmp;               // post velocity scaling
        const float env2s = env2 * velFactorFilt;
        const float lfo1  = renderLfo (0, lfoIncr[0], lfoShapeIdx[0]);
        const float lfo2  = renderLfo (1, lfoIncr[1], lfoShapeIdx[1]);
        ++noteOnSamples;

        float src[NumSources] = { 0.0f };
        src[(int) Source::Env1]       = env1s;
        src[(int) Source::Env2]       = env2s;
        src[(int) Source::LFO1]       = lfo1;
        src[(int) Source::LFO2]       = lfo2;
        src[(int) Source::Velocity]   = velNorm;
        src[(int) Source::KeyTrack]   = keyTrackNorm;
        src[(int) Source::ModWheel]   = modWheel;
        src[(int) Source::Aftertouch] = aftertouch;
        src[(int) Source::Macro1]     = macro1;
        src[(int) Source::Macro2]     = macro2;
        src[(int) Source::Macro3]     = macro3;
        src[(int) Source::Macro4]     = macro4;
        src[(int) Source::HeroMacro]  = hero;

        // Sum matrix contributions per destination. (No modulation-of-modulation:
        // destinations are DSP params only, and sources are never destinations.)
        float md[NumDests] = { 0.0f };
        for (int slot = 0; slot < 8; ++slot)
            if (matSrc[slot] != (int) Source::None && matDst[slot] != (int) Dest::None)
                md[matDst[slot]] += src[matSrc[slot]] * matAmt[slot];

        lastEnv1 = env1s; lastEnv2 = env2s; lastLfo1 = lfo1; lastLfo2 = lfo2;

        auto modV = [&md] (Dest d) { return md[(int) d] * destDepth[(int) d]; };

        // -------- Apply modulation to the per-sample destinations ------------
        const float syncDepth = juce::jlimit (0.0f, 36.0f, baseSyncDepth + modV (Dest::SyncDepth));
        const float pmAmount  = juce::jlimit (0.0f, 1.0f,  basePmAmount  + modV (Dest::PMAmount));

        const float pitchAllSemi = modV (Dest::PitchAll);
        const float pMul1 = std::exp2 ((pitchAllSemi + modV (Dest::Osc1Pitch)) / 12.0f);
        const float pMul2 = std::exp2 ((pitchAllSemi + modV (Dest::Osc2Pitch)) / 12.0f);
        const float pMul3 = std::exp2 ((pitchAllSemi + modV (Dest::Osc3Pitch)) / 12.0f);
        const float pMulAll = std::exp2 (pitchAllSemi / 12.0f); // sub follows global vibrato

        const float lvl1e = juce::jlimit (0.0f, 1.0f, lvl1 + modV (Dest::Osc1Level));
        const float lvl2e = juce::jlimit (0.0f, 1.0f, lvl2 + modV (Dest::Osc2Level));
        const float lvl3e = juce::jlimit (0.0f, 1.0f, lvl3 + modV (Dest::Osc3Level));

        const float pw1e = juce::jlimit (0.05f, 0.95f, pw1 + modV (Dest::Osc1PW));
        const float pw2e = juce::jlimit (0.05f, 0.95f, pw2 + modV (Dest::Osc2PW));
        const float pw3e = juce::jlimit (0.05f, 0.95f, pw3 + modV (Dest::Osc3PW));

        // ===== Osc3 bank (PM source + audible) ===============================
        float o3mono = 0.0f, o3L = 0.0f, o3R = 0.0f;
        for (int k = 0; k < cfg[2].count; ++k)
        {
            const float dt = limitDt (noteFreq * ratio3 * pMul3 * cfg[2].freqMul[k] * driftMul[2][k]);
            const float val = renderValue (w3, phaseA[2][k], pw3e, dt, triA[2][k]);
            phaseA[2][k] += dt;
            if (phaseA[2][k] >= 1.0f) phaseA[2][k] -= 1.0f;

            const float g = cfg[2].gain[k];
            o3mono += val * g;                     // summed output drives PM
            o3L    += val * g * cfg[2].panL[k];
            o3R    += val * g * cfg[2].panR[k];
        }

        // ===== Sync master = osc2 center pitch ===============================
        const float dtMaster = limitDt (noteFreq * ratio2 * pMul2);
        masterPhase += dtMaster;
        bool syncReset = false;
        float tau = 0.0f;
        if (masterPhase >= 1.0f)
        {
            masterPhase -= 1.0f;
            if (syncOn)
            {
                syncReset = true;
                tau = 1.0f - masterPhase / dtMaster;
            }
        }

        // ===== Osc2 bank (audible) ===========================================
        float o2L = 0.0f, o2R = 0.0f;
        for (int k = 0; k < cfg[1].count; ++k)
        {
            const float dt = limitDt (noteFreq * ratio2 * pMul2 * cfg[1].freqMul[k] * driftMul[1][k]);
            const float val = renderValue (w2, phaseA[1][k], pw2e, dt, triA[1][k]);
            phaseA[1][k] += dt;
            if (phaseA[1][k] >= 1.0f) phaseA[1][k] -= 1.0f;

            const float g = cfg[1].gain[k];
            o2L += val * g * cfg[1].panL[k];
            o2R += val * g * cfg[1].panR[k];
        }

        // ===== Osc1 bank (sync slave + PM) ===================================
        const float pmOffset = pmAmount * 1.0f * o3mono; // k = 1 cycle at full amount
        const float syncFactor = syncOn ? std::exp2 (syncDepth / 12.0f) : 1.0f;

        float o1L = 0.0f, o1R = 0.0f;
        for (int k = 0; k < cfg[0].count; ++k)
        {
            const float dt = limitDt (noteFreq * ratio1 * pMul1 * cfg[0].freqMul[k] * syncFactor * driftMul[0][k]);
            const float prevPhase = phaseA[0][k];

            float val = renderOsc1Voice (k, w1, pw1e, dt, pmOffset);

            phaseA[0][k] += dt;
            if (phaseA[0][k] >= 1.0f) phaseA[0][k] -= 1.0f;

            if (syncReset)
            {
                // Anti-alias the forced reset (same kernel as the natural wrap,
                // scaled by the true step height h = value_after - value_before).
                const float phAtReset = wrap01 (prevPhase + tau * dt);
                const float before = naiveValue (w1, wrap01 (phAtReset + pmOffset), pw1e);
                const float after  = naiveValue (w1, wrap01 (0.0f      + pmOffset), pw1e);
                const float h = after - before;

                val          += 0.5f * h * (1.0f - tau) * (1.0f - tau);
                blepNext1[k] += -0.5f * h * tau * tau;

                phaseA[0][k] = (1.0f - tau) * dt;
                if (phaseA[0][k] >= 1.0f) phaseA[0][k] -= 1.0f;
                syncSkipRising[k] = true;
            }

            const float g = cfg[0].gain[k];
            o1L += val * g * cfg[0].panL[k];
            o1R += val * g * cfg[0].panR[k];
        }

        // ===== Sub + noise (mono, centered) ==================================
        const float dtsub = limitDt (noteFreq * subRatio * pMulAll);
        const float sub = subSine ? std::sin (kTwoPi * subPhase)
                                  : pulseValue (subPhase, subPhase, 0.5f, dtsub);
        subPhase += dtsub;
        if (subPhase >= 1.0f) subPhase -= 1.0f;

        const float white = rng.nextFloat() * 2.0f - 1.0f;
        noiseLP += noiseCoeff * (white - noiseLP);
        const float noise = noiseLP * noiseMakeup;

        const float monoExtra = (sub * subLvl + noise * noiseLvl) * kSqrtHalf;

        // ===== Clean stereo voice mix (pre drive/filter) ====================
        sL[s] = o1L * lvl1e + o2L * lvl2e + o3L * lvl3e + monoExtra;
        sR[s] = o1R * lvl1e + o2R * lvl2e + o3R * lvl3e + monoExtra;

        // ===== Per-sample control signals ===================================
        // Hero ("ROT") gesture, hardwired: drive up, cutoff down (octaves),
        // unison detune wider (the detune part is applied in updateBankConfigs).
        const float heroDrive  = hero * 0.8f;
        const float heroCutOct = -hero * 4.0f;

        ampArr[(size_t) s]       = env1s;                      // Env1 -> amp (hardwired)
        driveArr[(size_t) s]     = juce::jlimit (0.0f, 1.0f, baseDrive + modV (Dest::Drive) + heroDrive);
        resonanceArr[(size_t) s] = juce::jlimit (0.0f, 1.0f, baseResonance + modV (Dest::Resonance));
        panArr[(size_t) s]       = juce::jlimit (-1.0f, 1.0f, modV (Dest::VoicePan));
        cutoffArr[(size_t) s]    = computeCutoff (baseCutoff, keyTrack, noteFreq,
                                                  filterEnvAmt, env2s, modV (Dest::Cutoff) + heroCutOct);
    }

    // ========================================================================
    //  PHASE B/C: oversampled pre-filter drive -> filter. Drive (waveshaper),
    //  filter feedback nonlinearity and self-oscillation all alias at base rate,
    //  so the whole block runs inside juce::dsp::Oversampling.
    // ========================================================================
    juce::dsp::AudioBlock<float> block (scratch);
    auto inBlock = block.getSubBlock (0, (size_t) numSamples);
    auto osBlock = oversampler->processSamplesUp (inBlock);

    const int factor = (int) (osBlock.getNumSamples() / (size_t) numSamples);
    const float osRate = (float) oversampledRate;
    const float nyq = 0.45f * osRate;

    for (int i = 0; i < numSamples; ++i)
    {
        const float fc = juce::jlimit (20.0f, nyq, cutoffArr[(size_t) i]);
        const float drv = driveArr[(size_t) i];
        const float res = resonanceArr[(size_t) i];

        if (useKorg35) { korg35.setCutoff (fc, osRate); korg35.setResonance (res); }
        else           { ladder.setCutoff (fc, osRate); ladder.setResonance (res); }

        for (int j = 0; j < factor; ++j)
        {
            const size_t idx = (size_t) (i * factor + j);
            for (int ch = 0; ch < 2; ++ch)
            {
                float x = osBlock.getSample (ch, (int) idx);
                x = driveShape (x, drv);
                x = useKorg35 ? korg35.process (ch, x) : ladder.process (ch, x);
                osBlock.setSample (ch, (int) idx, x);
            }
        }
    }

    oversampler->processSamplesDown (inBlock);

    // ========================================================================
    //  PHASE D: amp envelope (Env1) + output. Master volume is applied by the
    //  processor across the summed buffer.
    // ========================================================================
    const int numCh = outputBuffer.getNumChannels();
    float* chL = outputBuffer.getWritePointer (0, startSample);
    float* chR = numCh > 1 ? outputBuffer.getWritePointer (1, startSample) : nullptr;

    const float halfPi = juce::MathConstants<float>::halfPi;
    for (int s = 0; s < numSamples; ++s)
    {
        const float amp = ampArr[(size_t) s] * 0.5f; // Env1 already velocity-scaled
        float L = sL[s] * amp;
        float R = sR[s] * amp;

        // Voice pan (constant power; centered = unity on both channels).
        const float ang = (panArr[(size_t) s] + 1.0f) * halfPi * 0.5f;
        L *= std::cos (ang) * 1.41421356f;
        R *= std::sin (ang) * 1.41421356f;

        if (chR != nullptr) { chL[s] += L; chR[s] += R; }
        else                { chL[s] += 0.5f * (L + R); }
    }

    if (! adsr.isActive())
        clearCurrentNote();
}
