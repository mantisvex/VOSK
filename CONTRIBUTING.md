# Contributing to VOSK

Thanks for your interest in hacking on VOSK. This document covers how to build it, how
the code is laid out, and the DSP/GUI architecture so you can find your way around.

By contributing you agree that your contributions are licensed under the project's
**GPLv3** (see [`LICENSE`](LICENSE)).

## Building

Requires **CMake ≥ 3.22** and a **C++17** compiler. JUCE 8.x is pulled in automatically
via CMake `FetchContent`, so a clean checkout builds with no extra setup:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release      # or omit for a debug build
cmake --build build --config Release
```

Useful targets: `VOSK_Standalone`, `VOSK_VST3`, `VOSK_AU` (macOS), `VOSK_All`.
Artefacts land in `build/VOSK_artefacts/`.

If you keep a local JUCE checkout, point `-DJUCE_DIR=/path/to/JUCE` (or set the
`JUCE_DIR` env var) to skip the download.

Almost all parameters live in a `juce::AudioProcessorValueTreeState` (APVTS), so any new
control is: add the parameter in `Parameters.h`, read its cached atomic pointer where the
DSP needs it, and attach a widget in `GuiComponents.h`.

## Repository layout

| Path | Role |
|---|---|
| `Source/DspHelpers.h` | PolyBLEP band-limited oscillators; Szabo super-saw constants. |
| `Source/Filters.h` | ZDF ladder + Korg35 Sallen-Key filters, pre-filter drive shaper. |
| `Source/Character.h` | Output saturation stage (Tube/Diode/Fold/Crush). |
| `Source/Tape.h` | Tape/VHS stage (wow & flutter, saturation, hiss). |
| `Source/FxChain.h` | Chorus → delay → reverb. |
| `Source/ModMatrix.h` | Mod source/dest enums, per-destination depths, LFO shapes. |
| `Source/Scope.h` | Lock-free ring buffer feeding the oscilloscope. |
| `Source/Parameters.h` | APVTS layout + cached atomic-pointer view of every param. |
| `Source/VoskVoice.{h,cpp}` | The voice: 3 oscillator banks, sync, PM, sub, noise, filter, envelopes, glide. |
| `Source/PluginProcessor.{h,cpp}` | Voice allocation, FX/tape, output safety, on-screen keyboard. |
| `Source/Presets.h` / `PresetManager.h` | Factory preset data + preset/bank file handling. |
| `Source/VoskLookAndFeel.h` / `GuiComponents.h` | Custom look & feel + reusable widgets/panels. |
| `Source/PluginEditor.{h,cpp}` | Editor: scalable canvas, tabs, live displays, keyboard. |
| `.github/workflows/release.yml` | Cross-platform release CI. |

## Signal flow

Per voice:

```
oscillators (+unison) + sub + noise
        │
      MIX ─► pre-filter DRIVE ─► [oversample] ─► FILTER ─► [downsample] ─► AMP ENV ─► voice out
```

Then the summed voice bus:

```
Σ voices ─► CHARACTER (saturation) ─► chorus ─► delay ─► reverb ─► TAPE ─► master vol ─► output safety ─► out
```

The **output safety** stage (in the processor) blocks DC, kills NaN/Inf, and soft-clips
below 0 dBFS — nothing downstream can produce a runaway level.

## DSP notes

**Band-limiting (PolyBLEP).** Naive saw/pulse steps alias; `polyBlep(t, dt)` is a
2-sample band-limited-step residual applied at discontinuities. Saw = downward ramp +
blep; pulse = dual-edge blep (clean PWM); triangle = leaky-integrated band-limited
square; sine is exact.

**Anti-aliased hard sync.** A forced phase reset is a step discontinuity the natural
blep never sees. On the master wrap (sub-sample position `tau`) the slave's true step
`h = after − before` is corrected with the same parabolic kernel as the saw
(`corrNow = 0.5·h·(1−tau)²`, `corrNext = −0.5·h·tau²`), with a one-sample guard so the
small post-reset phase doesn't trip a second, spurious correction.

**Super-saw unison.** Detune polynomial, offset table and center/side gain curves are
from Adam Szabo's *"How to Emulate the Super Saw"* (`vosk::szabo`). Voice counts below 7
use a symmetric subset around center; gains are power-normalised so loudness barely moves
with detune/voice count; spread uses a constant-power pan law.

**Filters.** Both are zero-delay-feedback / TPT designs (Zavalishin). The ladder is a
4-pole with tanh-saturated resonance feedback; the Korg35 is Pirkle's Sallen-Key model
that breaks up at high resonance. The drive→filter block runs inside
`juce::dsp::Oversampling` — the factor is a single constant (`VoskVoice::kOversampleFactor`).
8 voices × the oversampled filter is the CPU hot spot; that constant is the quality/CPU knob.

**Modulation.** Per sample, per voice: compute all sources (envelopes post-velocity,
LFOs, velocity, key track, mod wheel, aftertouch, macros), then each of the 8 matrix
slots adds `source × amount` into a per-destination accumulator scaled by a
per-destination depth (`ModMatrix.h` `destDepth`), added to the base parameter and
clamped. **There is no modulation-of-modulation** — destinations are DSP params only,
and sources are never destinations. Unison detune/spread are modulated at block rate
(they rebuild the unison bank); everything else is per-sample.

## GUI notes

All content lives on a fixed-size `Canvas` (see `PluginEditor`) that is drawn with an
`AffineTransform::scale` for the UI-scale control, so fonts and graphics scale crisply.
The lower section is tabbed (Modulation / FX & Output). Live displays (envelope curves,
filter response, LFO playheads, scope/meters, matrix highlights) are cheap timers that
repaint on change and read APVTS atomics or processor-owned visualiser feeds.

The UI font (Rajdhani) is embedded as binary data so the interface renders identically
everywhere.

## Presets

Factory presets are data in `getFactoryPresets()` (`Presets.h`): each is a **sparse list
of `{paramID, value}` overrides** applied on top of the defaults. Value conventions
(wave/filter/voice-mode indices, mod source/dest enums) are documented at the top of that
file. When adding presets:

- keep `mastervol` roughly in the 0.45–0.6 range so browsing stays level-matched;
- keep resonance moderate (the output safety stage is a backstop, not a license to blast);
- give it a `name` and a `category` (categories appear automatically).

User presets are `.voskpreset` files (APVTS state as XML); banks are `.voskbank` bundles.

## Testing

Please run [pluginval](https://github.com/Tracktion/pluginval) on the built VST3 before
opening a PR touching DSP, parameters, state or the editor:

```sh
pluginval --strictness-level 10 --validate build/VOSK_artefacts/**/VST3/VOSK.vst3
```

It should pass at strictness 10 (including the fuzz-parameters test).

## Pull requests

- Keep changes focused; match the surrounding code style (JUCE conventions, 4-space
  indent, `//====` section banners).
- Don't commit build output — `build/`, `tools/` and `installer/Output/` are gitignored.
- Describe what you changed and how you verified it (a build + pluginval run is ideal).

## Releases

Releases are cut by pushing a tag:

```sh
git tag v0.2.0
git push origin v0.2.0
```

The `Release` workflow then builds on Windows + macOS, zips the binaries and attaches
them to the GitHub release. macOS builds are currently unsigned.
