# VOSK — Design Lock

**Name:** VOSK
**Type:** Darksynth / cyberpunk subtractive synthesizer
**Brand:** MantisVex
**Engine approach:** Analog-style subtractive (PolyBLEP), no wavetables.
**North star:** Insane darksynth basses + searing leads. Every contested decision bends toward bass violence.

This document is the design source of truth. It is *not* the Claude Code build prompt — that's the next artifact, where PolyBLEP oscillators, both filter topologies, and the Szabo unison engine get pinned down at implementation level so nothing is hand-waved.

---

## 0. Purpose / scope priority

Built for **leads + bass first**, with the envelope speed and mono/glide to nail arps and plucks. Pads are the throwaway tier — supported, not chased. No lush evolving-pad ambitions (that road rebuilds Serum).

The reason this genre is buildable where a Serum clone isn't: darksynth wants grit, saturation, and filth. The hardest part of a clean wavetable synth (anti-aliasing) is downgraded from blocker to non-issue. We lean into dirt deliberately.

---

## 1. Voice architecture

- **8-voice true polyphony.**
- **Mono / Legato modes** with **glide (portamento)** — this is the core lead/bass sound, non-negotiable.
- **Pitch bend range** global, default ±2.
- **Unison voice cap by mode (mandatory, not optional):**
  - Mono: full unison (up to 7 per osc).
  - Poly: auto-capped to 3–4 per osc to stay under CPU ceiling.
- CPU ceiling set consciously now: 3 osc × 7 unison × 8 voices = 168 oscillators worst case. Viable *only* because PolyBLEP is cheap. The poly auto-cap exists to never actually hit 168.

---

## 2. Oscillators

**Three symmetric oscillators.** Identical capability, each with full waveform choice. No "special" oscillator — roles come from routing, not from baked-in personality.

Per oscillator:
| Param | Range / values |
|---|---|
| Waveform | Saw / Pulse / Triangle / Sine |
| Pulse Width | active on Pulse; PWM-modulatable |
| Octave | −2 … +2 |
| Semitone | −12 … +12 |
| Fine | −50 … +50 cents |
| Level | 0 … 1 |
| Phase | Free / Fixed-retrigger |
| Unison voices | 1 … 7 |
| Unison detune | 0 … 1 |
| Unison spread | 0 … 1 (stereo width) |
| Unison phase mode | Random (leads) / Fixed (bass) |

Plus:
- **Sub oscillator:** sine/square, −1/−2 oct, level. Dedicated low-end weight independent of the three main osc.
- **Noise:** level + color (tilt white→filtered).

### Sync — the bass growl engine
- **Osc2 → Osc1 hard sync**, on/off.
- **Sync depth is a mod destination.** This is the line that makes it vicious instead of just "having sync." The signature bass sound is a fast envelope sweeping sync depth, morphing clean→screaming across the note. If this isn't modulatable it's worthless.

### Phase mod — secondary dirt
- **Osc3 → Osc1 phase mod**, single knob, modulatable. Metallic/aggressive bite without a full FM engine. Alternate flavor of filth for when sync isn't the right texture.

---

## 3. Unison engine (the soul)

Implementation is pinned now so it doesn't degrade into a flat cheap detune:

- **Reference: Adam Szabo, "How to Emulate the Super Saw."**
- **Non-linear detune spread** — side voices are not evenly spaced.
- **Detune-dependent gain curve** — side voices sit quieter than the center voice.
- Variable voice count 1–7 (not fixed).
- **Phase mode switchable:** random per note-on = lush/smeared (leads); fixed = punchy/tight (bass).

---

## 4. Filter

**Both topologies, selectable.** The one place flexibility is justified — patches genuinely want different teeth.

- **Moog ladder 24 dB** — smooth scream, lead-friendly, versatile.
- **Sallen-Key (MS-20 style)** — dirtier, nastier, the bass-violence option.
- Both must reach **musical self-oscillation** at high resonance.

Signal placement (where the character actually lives):
- **Drive is PRE-filter.** This is non-negotiable for the sound. Synced/dirty osc → drive mangles harmonics → resonant filter focuses the scream. The three stack; a synced osc into a clean filter sounds thin.

Params: Cutoff, Resonance, Drive (pre), Key tracking, Env amount, Filter type select.

---

## 5. Modulation

- **2 envelopes** (ADSR + velocity sensitivity).
  - **Env1 → Amp, hardwired.**
  - Env2 default → filter cutoff + sync depth (both routable).
- **2 LFOs**, both **tempo-syncable** (the genre is rhythmic — non-negotiable). Shapes: sine/tri/saw/square/S&H. Free Hz or tempo division; phase/retrig control.
- **Fixed mod matrix, 8 slots.** Each slot: source → destination, bipolar amount. No drag-drop-anywhere (that's Serum-bloat).

**Sources:** Env1, Env2, LFO1, LFO2, Velocity, Key Track, Mod Wheel, Aftertouch, Macro1–4, Hero macro.

**Key destinations:** osc pitch (per osc), osc level, pulse width, **sync depth**, **PM amount**, unison detune, cutoff, resonance, drive, LFO rate, FX params.

**Explicitly out: drawable LFOs.** The single biggest scope monster; the genre doesn't need it.

---

## 6. FX chain

Fixed order, each bypassable. Reorderable rack = scope creep, skipped — the order never changes for this music.

1. **Juno-style BBD chorus** — that exact shimmer is half of synthwave. Rate, depth, mix, mode (I/II).
2. **Tempo-synced delay** — **dotted-eighth default**. Time (synced divisions), feedback, mix, feedback-path tone filter, ping-pong on/off.
3. **Dark reverb** — size, decay, damping (dark-biased), pre-delay, mix.

> Note on drive: the **primary drive is pre-filter** (§4), where the character lives. No second output-stage drive in v1 — avoids double-drive confusion. Output saturation is a deliberate v2 candidate, not a v1 feature.

---

## 7. Macros

- **Hero macro** (BITROT-"ROT" lineage) — name TBD. **Defined multi-target gesture**, not just a knob: one move that
  - drives gain up,
  - pulls filter cutoff down,
  - widens unison detune.
  → "dirties" the whole patch in a single gesture. Exact curve/targets are a real spec line, set here.
- **4 assignable macros** routed through the mod matrix.

---

## 8. Signal flow

```
[Osc1]──┐  (Osc2→Osc1 hard sync, depth modulatable)
[Osc2]──┤  (Osc3→Osc1 phase mod)
[Osc3]──┼──► MIXER ──► DRIVE (pre) ──► FILTER ──► AMP ──► FX ──► OUT
[Sub ]──┤                              (Ladder /        (Env1)   chorus
[Noise]─┘                               Sallen-Key)               → delay
                                                                  → reverb
        Unison (Szabo) applied per main osc
        Mod: 2 Env + 2 LFO → 8-slot matrix
        Env1→Amp hardwired · Env2→cutoff+sync default
```

---

## 9. Global controls

Master volume · Mono/Poly/Legato mode · Glide time · Pitch bend range.

---

## 10. Scope wall — DO NOT BUILD (v1)

Consciously excluded so they can't sneak in mid-build:

- Wavetables
- FFT / resynthesis
- Drawable LFOs
- Reorderable FX rack
- MPE
- Microtuning
- Preset morphing
- Modulation of modulation
- Output-stage saturation (v2 candidate)

Every one is a week that doesn't make the tracks better.

---

## 11. Open decisions (still yours to call)

1. **Hero macro name** — reuse "ROT" for brand continuity, or new name for VOSK's own identity?
2. **Aftertouch as mod source** — confirmed in matrix; drop if not used in your workflow.

---

## 12. What's locked vs what's next

**Locked:** everything above except §11.

**Next artifact (the build prompt):** module architecture for Claude Code, with PolyBLEP oscillator implementation, both filter topologies (ladder + Sallen-Key ZDF), and the Szabo unison engine pinned at code level — plus parameter IDs, ranges, and APVTS layout — so the build goes clean instead of turning to mush. The DSP that gets hand-waved is exactly the DSP that decides whether it sounds like Perturbator or like a demo.
