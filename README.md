# VOSK — Stages 1–5: Voice + Unison + Filter + Modulation + FX

Darksynth / cyberpunk subtractive synthesizer (MantisVex). Stages: **1** band-limited
oscillators + voicing, **2** Szabo supersaw unison, **3** pre-filter drive + two filter
topologies + filter envelope, **4** 2 envelopes + 2 LFOs + 8-slot mod matrix, **5** FX
chain (chorus → delay → reverb). Macros are the only remaining stage.

The GUI is a custom darksynth interface (`VoskLookAndFeel.h` + `GuiComponents.h`):
charcoal panels with neon accent arcs, sectioned into 3 oscillators, sub/noise/sync,
filter+drive, amp/filter envelopes, 2 LFOs, the 8-slot mod matrix and a global bar.
All controls attach to the APVTS.

## What works
- 3 symmetric **PolyBLEP** oscillators per voice (Saw / Pulse / Triangle / Sine),
  band-limited so they stay clean across the whole keyboard (no foldover on C6–C7).
- **Per-oscillator Szabo unison** (stage 2): each osc is an independent bank of up
  to 7 detuned PolyBLEP voices with the exact Szabo detune polynomial, offset
  table and center/side gain curves, symmetric voice-count subsets, constant-power
  stereo spread, and Random/Fixed phase. Poly auto-caps to 4 voices/osc; Mono/Legato
  get the full 7.
- **Hard sync** osc2 → osc1 with an **anti-aliased reset** (BLEP correction at the
  forced reset, not just the natural wrap). With unison: osc2's center pitch is the
  master and resets all of osc1's unison voices.
- **Phase mod** osc3 → osc1 (linear PM, `pmAmount`); osc3's summed output modulates
  all of osc1's unison voices.
- `syncDepth` and `pmAmount` are read **per sample** so they drop into the mod
  system in stage 4 without refactoring.
- Sub oscillator (sine/square, −1/−2 oct), noise (white → dark tilt).
- **Per-voice pre-filter drive → oversampled filter** (stage 3): asymmetric
  soft-clip drive (even+odd harmonics, makeup-compensated) feeding a selectable
  **Moog-style ZDF ladder** (24 dB, smooth scream) or **Korg35/MS-20 Sallen-Key**
  (nasty, screams at high res). Both self-oscillate. Drive→filter runs inside
  `juce::dsp::Oversampling` (factor = single constant `kOversampleFactor`, 2× or 4×).
- **Filter envelope (Env2)** per voice → cutoff (bipolar amount), plus key
  tracking; cutoff is computed in one per-sample function ready for stage-4 mod.
- Per-voice **amp ADSR**, **master volume**.
- **Modulation system** (stage 4): two generalized per-voice envelopes (Env1/Env2,
  now matrix sources with velocity sensitivity), two per-voice **tempo-syncable
  LFOs** (6 shapes incl. S&H, free Hz or beat divisions, Free/Key trigger, phase,
  fade-in), and an **8-slot fixed mod matrix** feeding the per-sample destinations
  built in stages 1–3. Ships with **Env2 → Sync Depth (+0.5)** wired by default so
  it growls out of the box; macro sources are reserved (inert) for stage 6.
- **FX chain** (stage 5, `FxChain.h`): global post-voice, fixed order, each bypassable —
  a Juno-style BBD **chorus** (anti-phase L/R, mode I/II), a tempo-synced stereo **delay**
  (tone filter in the feedback path, ping-pong), and a dark **reverb** (Freeverb topology,
  dark-biased damping, separate size/decay, pre-delay). All default OFF.
- **Poly** (8 voices, JUCE stealing) / **Mono** / **Legato** with exponential
  **glide** and **pitch bend**.

## Build

Requires CMake ≥ 3.22 and a C++17 compiler. JUCE 8.0.6 is used from a local
checkout at `E:/JUCE` if present, otherwise fetched from GitHub. Override the
location with the `JUCE_DIR` environment variable or `-DJUCE_DIR=...`.

```sh
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Targets: `VOSK_Standalone`, `VOSK_VST3`, `VOSK_All`. On this machine the VST3 is
auto-copied to `E:/Audio plugins` after build.

Artifacts land in `build/VOSK_artefacts/Release/`.

## Source layout
| File | Role |
|---|---|
| `Source/DspHelpers.h` | PolyBLEP + band-limited saw/pulse/triangle/sine, naive-value helper for sync-step measurement. |
| `Source/Parameters.h` | APVTS layout, all param IDs (incl. reserved stage-2 unison IDs), cached atomic-pointer view. |
| `Source/VoskVoice.{h,cpp}` | The voice: 3 osc, sync + reset BLEP, PM, sub, noise, ADSR, glide. |
| `Source/VoskSound.h` | One sound covering the whole keyboard. |
| `Source/PluginProcessor.{h,cpp}` | Poly via `juce::Synthesiser`; Mono/Legato via a manual last-note-priority engine. |
| `Source/PluginEditor.{h,cpp}` | Generic-editor stub. |

## DSP notes

**PolyBLEP.** Naive saw/square steps alias because their corners carry energy
above Nyquist. `polyBlep(t, dt)` (in `DspHelpers.h`) is a 2-sample polynomial
band-limited-step residual, active only within `dt` of a phase wrap. Saw =
downward ramp + blep; pulse = dual-edge blep (both rising and falling edges, so
PWM stays clean); triangle = leaky-integrate the band-limited square (the
integrator turns alias-free steps into alias-free slope changes); sine is exact.

**Anti-aliased hard sync.** A forced phase reset is itself a step discontinuity
that the natural-wrap blep never sees. When osc2 (master) wraps at sub-sample
position `tau`, we measure osc1's true step `h = value_after − value_before` and
add a blep scaled by `h`, using the same parabolic kernel as the natural saw:
`corrNow = 0.5·h·(1−tau)²`, `corrNext = −0.5·h·tau²` (carried to the next
sample). A one-sample guard (`syncSkipRising`) stops the small post-reset phase
from tripping a second, spurious natural-wrap correction.

**Phase mod.** osc1's waveform is read at `phase + pmAmount·osc3` (k = 1 cycle at
full amount) while the blep timing stays on the base phase — exact linear PM for
the sine carrier, gritty/metallic on the others.

**Modulation (stage 4).** Per sample, per voice: all source values are computed
(Env1/Env2 post-velocity, LFO1/2, Velocity, KeyTrack, ModWheel, Aftertouch; macros
reserved = 0), then each of the 8 matrix slots adds `source × amount` into its
destination accumulator (`md[]`), which is scaled by a per-destination depth
(`ModMatrix.h` `destDepth` — semitones for pitch/sync, octaves for cutoff, native
units otherwise), added to the base parameter and clamped. The stage 1–3 hooks were
all already per-sample, so nothing was refactored — the matrix just feeds them. The
one exception is unison detune/spread, which stage 2 built at block rate; those are
modulated at block rate from a one-block-latent source snapshot (`updateBankConfigs`).
**No modulation-of-modulation exists**: destinations are DSP params only — there is
no destination for LFO rate, envelope times, or matrix amounts, and sources are
never destinations. Free-trigger LFOs read a processor-owned shared phase so all
voices stay aligned; Key-trigger LFOs reset per note-on.

**Filter + drive (stage 3).** Per-voice signal flow is
`oscMix → drive → [oversample] → filter → [downsample] → amp env → out`; the drive
is **pre-filter** by design (do not reorder — synced/dirty osc into drive into a
resonant filter is where the character lives). Both filters are ZDF/TPT designs
(`Filters.h`, Zavalishin): the ladder is a 4-pole with a tanh-saturated resonance
feedback (self-oscillates at k≈4 independent of cutoff); the Korg35 is the
Pirkle Sallen-Key topology (series one-pole LP → resonant LP/HP core with a
saturator in the resonance path) that deliberately breaks up at high resonance.
Cutoff is computed per-sample from base + key tracking + Env2 in one function so
stage-4 modulation drops in without refactoring. The oversampling factor is the
single constant `VoskVoice::kOversampleFactor` (2× default; set 4× for
heavy-drive/self-osc patches — 8 voices × oversampled filter is the CPU hotspot).

Note: the filter path is run in **stereo** (one filter object per voice, 2-channel
state) so stage 2's unison stereo spread survives the filter. The stage brief said
"mono"; this keeps the prior stage's stereo intact at the cost of 2 channels of
filter state per voice. Collapsing to literal mono is a one-line change
(`kFilterChannels`-style) if CPU ever demands it.

**Szabo unison (stage 2).** Constants are reproduced verbatim from Adam Szabo,
*"How to Emulate the Super Saw"* (BTH thesis, 2010), in `DspHelpers.h`
(`vosk::szabo`): the 11th-order detune polynomial, the 7-wide relative offset
table (`freq_i = base·(1 + detune(x)·offset_i)`), and the detune-dependent
`centerGain`/`sideGain` curves. Voice counts below 7 pick a **symmetric subset**
around the center so the detune stays balanced. Gains are power-normalised
(`1/√Σg²`) so patch loudness barely moves with the detune knob or voice count
while the center-vs-side balance is preserved. Spread pans voices symmetrically
by index across [−1,+1] with a constant-power law (center stays centered).
