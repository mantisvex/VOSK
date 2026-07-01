#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <utility>

//==============================================================================
//  VOSK factory presets.
//
//  A preset is a sparse list of {paramID, value} overrides on top of the APVTS
//  defaults (the manager resets everything to default first, then applies these).
//  Values are in each parameter's native units:
//    wave: 0 Saw 1 Pulse 2 Tri 3 Sine | phasemode: 0 Free 1 Fixed
//    uniphase: 0 Random 1 Fixed | subwave: 0 Sine 1 Square | suboct: 0 = -1, 1 = -2
//    filtertype: 0 Ladder 1 Sallen-Key | voicemode: 0 Poly 1 Mono 2 Legato
//    mod source: 0 None 1 Env1 2 Env2 3 LFO1 4 LFO2 5 Vel 6 KeyTrk 7 Wheel 8 AT ...
//    mod dest:   0 None 1 PitchAll ... 11 SyncDepth 12 PMAmt ... 19 Cutoff 20 Reso 21 Drive
//
//  Default matrix slot 1 is Env2 -> SyncDepth (0.5): a preset that turns Hard
//  Sync on inherits the classic envelope-swept growl for free.
//
//  Gain-staging note: resonance is kept conservative (<= ~0.45) and Sallen-Key
//  patches lower still, so nothing self-oscillates into a Larsen runaway; the
//  processor's output stage soft-clips as a final guard. Heavy comes from drive,
//  sub weight and the filter envelope, not from cranked resonance.
//==============================================================================
namespace vosk
{
    // Param override. Templated ctor so both int and float literals in the
    // preset tables convert cleanly (no narrowing warnings).
    struct PV
    {
        juce::String id;
        float value;
        template <typename T> PV (const char* i, T v) : id (i), value ((float) v) {}
    };

    struct Preset
    {
        juce::String name;
        juce::String category;
        std::vector<PV> values;
    };

    inline std::vector<Preset> getFactoryPresets()
    {
        using V = std::vector<PV>;
        std::vector<Preset> p;

        //====================================================================
        //  BASS  — mono, glide, sub-loaded, driven, tight filter envelope.
        //====================================================================
        p.push_back ({ "Turbo Killer", "Bass", V {
            {"voicemode",1}, {"glidetime",0.02f}, {"pitchbendrange",2},
            {"osc1wave",0}, {"osc1level",0.9f}, {"osc1univoices",2}, {"osc1unidetune",0.09f}, {"osc1uniphase",1},
            {"osc2wave",1}, {"osc2pw",0.5f}, {"osc2level",0.55f}, {"osc2oct",0},
            {"subwave",1}, {"suboct",0}, {"sublevel",0.7f},
            {"filtertype",0}, {"cutoff",560.0f}, {"resonance",0.18f}, {"drive",0.55f}, {"filterenvamount",0.45f},
            {"filterattack",0.001f}, {"filterdecay",0.16f}, {"filtersustain",0.28f}, {"filterrelease",0.14f},
            {"ampattack",0.001f}, {"ampdecay",0.25f}, {"ampsustain",0.9f}, {"amprelease",0.1f}, {"ampvelsens",0.25f},
            {"charon",1}, {"charmode",1}, {"chardrive",0.4f}, {"chartone",0.7f},   // Diode bite
            {"mastervol",0.6f},
        }});

        p.push_back ({ "Hydra Growl", "Bass", V {
            {"voicemode",1}, {"glidetime",0.03f},
            {"osc1wave",0}, {"osc1level",0.9f}, {"osc1univoices",2}, {"osc1unidetune",0.12f}, {"osc1uniphase",1},
            {"osc2wave",0}, {"osc2level",0.7f},
            {"syncon",1}, {"syncdepth",4.0f},                 // Env2 -> SyncDepth (default 0.5) sweeps it
            {"subwave",1}, {"sublevel",0.7f},
            {"filtertype",0}, {"cutoff",520.0f}, {"resonance",0.22f}, {"drive",0.5f}, {"filterenvamount",0.5f},
            {"filterattack",0.001f}, {"filterdecay",0.22f}, {"filtersustain",0.2f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.12f},
            {"mastervol",0.6f},
        }});

        p.push_back ({ "Magonia Low", "Bass", V {
            {"voicemode",1}, {"glidetime",0.05f},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1univoices",3}, {"osc1unidetune",0.16f}, {"osc1uniphase",1},
            {"osc2wave",0}, {"osc2level",0.5f}, {"osc2oct",-1},
            {"subwave",0}, {"suboct",1}, {"sublevel",0.9f},
            {"filtertype",0}, {"cutoff",380.0f}, {"resonance",0.16f}, {"drive",0.5f}, {"filterenvamount",0.4f},
            {"filterattack",0.001f}, {"filterdecay",0.3f}, {"filtersustain",0.3f},
            {"ampattack",0.002f}, {"ampsustain",0.92f}, {"amprelease",0.18f},
            {"charon",1}, {"charmode",0}, {"chardrive",0.35f},                     // Tube warmth
            {"choruson",1}, {"chorusmix",0.2f}, {"mastervol",0.58f},
        }});

        p.push_back ({ "Doom Reese", "Bass", V {
            {"voicemode",1}, {"glidetime",0.04f},
            {"osc1wave",0}, {"osc1level",0.8f}, {"osc1univoices",3}, {"osc1unidetune",0.22f}, {"osc1unispread",0.25f},
            {"osc2wave",0}, {"osc2level",0.8f}, {"osc2fine",-9.0f},
            {"subwave",0}, {"sublevel",0.7f},
            {"filtertype",0}, {"cutoff",520.0f}, {"resonance",0.2f}, {"drive",0.45f}, {"filterenvamount",0.25f},
            {"filterdecay",0.4f}, {"filtersustain",0.45f},
            {"ampattack",0.003f}, {"ampsustain",0.92f}, {"amprelease",0.22f},
            {"lfo1shape",0}, {"lfo1rate",0.16f}, {"mod3source",3}, {"mod3dest",19}, {"mod3amount",0.12f},
            {"mastervol",0.58f},
        }});

        p.push_back ({ "Sewer Sub", "Bass", V {
            {"voicemode",1}, {"glidetime",0.06f},
            {"osc1wave",2}, {"osc1level",0.55f},
            {"osc2wave",0}, {"osc2level",0.45f},
            {"subwave",1}, {"suboct",1}, {"sublevel",0.95f},
            {"noiselevel",0.04f}, {"noisecolor",0.85f},
            {"filtertype",0}, {"cutoff",260.0f}, {"resonance",0.14f}, {"drive",0.42f}, {"filterenvamount",0.3f},
            {"filterdecay",0.25f}, {"filtersustain",0.25f},
            {"ampattack",0.001f}, {"ampsustain",0.95f}, {"amprelease",0.14f},
            {"mastervol",0.6f},
        }});

        p.push_back ({ "Acid Fang", "Bass", V {
            {"voicemode",1}, {"glidetime",0.05f},
            {"osc1wave",0}, {"osc1level",0.9f},
            {"subwave",1}, {"sublevel",0.45f},
            {"filtertype",1}, {"cutoff",420.0f}, {"resonance",0.4f}, {"drive",0.5f}, {"filterenvamount",0.55f},
            {"filterattack",0.001f}, {"filterdecay",0.2f}, {"filtersustain",0.08f}, {"filterrelease",0.12f},
            {"ampattack",0.001f}, {"ampsustain",0.85f}, {"amprelease",0.1f},
            {"delayon",1}, {"delaymix",0.14f}, {"delaydivision",9}, {"mastervol",0.58f},
        }});

        p.push_back ({ "Concrete", "Bass", V {
            {"voicemode",1}, {"glidetime",0.02f},
            {"osc1wave",1}, {"osc1pw",0.5f}, {"osc1level",0.8f},
            {"osc2wave",0}, {"osc2level",0.5f},
            {"subwave",1}, {"suboct",0}, {"sublevel",0.8f},
            {"filtertype",0}, {"cutoff",480.0f}, {"resonance",0.16f}, {"drive",0.55f}, {"filterenvamount",0.4f},
            {"filterdecay",0.18f}, {"filtersustain",0.25f},
            {"ampattack",0.001f}, {"ampsustain",0.92f}, {"amprelease",0.1f}, {"mastervol",0.6f},
        }});

        p.push_back ({ "Blood Machine", "Bass", V {
            {"voicemode",1}, {"glidetime",0.03f},
            {"osc1wave",0}, {"osc1level",0.9f},
            {"osc2wave",0}, {"osc2level",0.6f},
            {"syncon",1}, {"syncdepth",2.0f},                 // default Env2 -> SyncDepth growl
            {"subwave",1}, {"sublevel",0.65f},
            {"filtertype",0}, {"cutoff",560.0f}, {"resonance",0.24f}, {"drive",0.5f}, {"filterenvamount",0.4f},
            {"filterattack",0.001f}, {"filterdecay",0.26f}, {"filtersustain",0.15f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.12f},
            {"reverbon",1}, {"reverbmix",0.08f}, {"mastervol",0.58f},
        }});

        p.push_back ({ "Gravewalker", "Bass", V {
            {"voicemode",1}, {"glidetime",0.04f},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1oct",-1}, {"osc1univoices",3}, {"osc1unidetune",0.16f},
            {"subwave",1}, {"suboct",1}, {"sublevel",0.9f},
            {"filtertype",0}, {"cutoff",330.0f}, {"resonance",0.2f}, {"drive",0.55f}, {"filterenvamount",0.45f},
            {"filterdecay",0.3f}, {"filtersustain",0.15f},
            {"ampattack",0.001f}, {"ampsustain",0.92f}, {"amprelease",0.16f},
            {"charon",1}, {"charmode",1}, {"chardrive",0.45f}, {"chartone",0.65f},  // Diode grind
            {"mastervol",0.55f},
        }});

        p.push_back ({ "Night Driver", "Bass", V {
            {"voicemode",2}, {"glidetime",0.07f},
            {"osc1wave",0}, {"osc1level",0.8f}, {"osc1univoices",3}, {"osc1unidetune",0.18f}, {"osc1unispread",0.3f},
            {"osc2wave",0}, {"osc2level",0.6f},
            {"subwave",0}, {"sublevel",0.6f},
            {"filtertype",0}, {"cutoff",500.0f}, {"resonance",0.18f}, {"drive",0.4f}, {"filterenvamount",0.25f},
            {"filterdecay",0.5f}, {"filtersustain",0.5f},
            {"ampattack",0.004f}, {"ampsustain",0.95f}, {"amprelease",0.28f},
            {"choruson",1}, {"chorusmix",0.28f},
            {"tapeon",1}, {"tapewow",0.3f}, {"tapesat",0.3f}, {"tapehiss",0.12f}, {"tapetone",0.6f},
            {"mastervol",0.58f},
        }});

        p.push_back ({ "Crusher", "Bass", V {
            {"voicemode",1}, {"glidetime",0.02f},
            {"osc1wave",0}, {"osc1level",0.9f}, {"osc1univoices",2}, {"osc1unidetune",0.1f}, {"osc1uniphase",1},
            {"osc2wave",1}, {"osc2pw",0.3f}, {"osc2level",0.5f},
            {"subwave",1}, {"sublevel",0.65f},
            {"filtertype",1}, {"cutoff",620.0f}, {"resonance",0.28f}, {"drive",0.65f}, {"filterenvamount",0.3f},
            {"filterdecay",0.2f}, {"filtersustain",0.25f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.1f},
            {"charon",1}, {"charmode",1}, {"chardrive",0.6f}, {"chartone",0.6f},    // heavy Diode
            {"mastervol",0.5f},
        }});

        p.push_back ({ "Sub Bomb", "Bass", V {
            {"voicemode",1}, {"glidetime",0.03f},
            {"osc1wave",3}, {"osc1level",0.6f},
            {"subwave",0}, {"suboct",1}, {"sublevel",1.0f},
            {"filtertype",0}, {"cutoff",220.0f}, {"resonance",0.1f}, {"drive",0.4f}, {"filterenvamount",0.25f},
            {"filterdecay",0.25f}, {"filtersustain",0.35f},
            {"ampattack",0.001f}, {"ampsustain",0.95f}, {"amprelease",0.2f}, {"mastervol",0.6f},
        }});

        //====================================================================
        //  LEAD
        //====================================================================
        p.push_back ({ "Searing Lead", "Lead", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.7f}, {"osc1univoices",7}, {"osc1unidetune",0.28f}, {"osc1unispread",0.7f}, {"osc1uniphase",0},
            {"osc2wave",0}, {"osc2level",0.4f}, {"osc2oct",1},
            {"filtertype",0}, {"cutoff",3200.0f}, {"resonance",0.18f}, {"drive",0.35f}, {"keytrack",0.4f}, {"filterenvamount",0.25f},
            {"filterdecay",0.4f}, {"filtersustain",0.6f},
            {"ampattack",0.005f}, {"ampsustain",0.85f}, {"amprelease",0.3f},
            {"charon",1}, {"charmode",0}, {"chardrive",0.3f},                      // Tube glue
            {"choruson",1}, {"chorusmix",0.3f}, {"delayon",1}, {"delaymix",0.22f}, {"delaydivision",8}, {"delaypingpong",1},
            {"reverbon",1}, {"reverbmix",0.18f}, {"mastervol",0.48f},
        }});

        p.push_back ({ "Neon Blade", "Lead", V {
            {"voicemode",2}, {"glidetime",0.06f},
            {"osc1wave",0}, {"osc1level",0.8f}, {"osc1univoices",5}, {"osc1unidetune",0.2f}, {"osc1unispread",0.6f},
            {"osc2wave",1}, {"osc2pw",0.4f}, {"osc2level",0.4f},
            {"filtertype",0}, {"cutoff",2600.0f}, {"resonance",0.22f}, {"drive",0.4f}, {"keytrack",0.5f}, {"filterenvamount",0.3f},
            {"filterdecay",0.5f}, {"filtersustain",0.5f},
            {"ampattack",0.01f}, {"ampsustain",0.9f}, {"amprelease",0.32f},
            {"lfo1shape",0}, {"lfo1rate",5.0f}, {"lfo1fade",0.4f}, {"mod3source",3}, {"mod3dest",1}, {"mod3amount",0.03f},
            {"delayon",1}, {"delaymix",0.25f}, {"delaypingpong",1}, {"reverbon",1}, {"reverbmix",0.2f}, {"mastervol",0.5f},
        }});

        p.push_back ({ "Hero Lead", "Lead", V {
            {"voicemode",1}, {"glidetime",0.05f},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1univoices",3}, {"osc1unidetune",0.15f},
            {"osc2wave",0}, {"osc2level",0.6f}, {"osc2semi",7},
            {"filtertype",0}, {"cutoff",3800.0f}, {"resonance",0.2f}, {"drive",0.4f}, {"keytrack",0.6f}, {"filterenvamount",0.25f},
            {"filterdecay",0.4f}, {"filtersustain",0.7f},
            {"ampattack",0.005f}, {"ampsustain",0.9f}, {"amprelease",0.3f},
            {"reverbon",1}, {"reverbmix",0.22f}, {"delayon",1}, {"delaymix",0.18f}, {"mastervol",0.52f},
        }});

        p.push_back ({ "Ghost", "Lead", V {
            {"voicemode",0},
            {"osc1wave",3}, {"osc1level",0.9f},
            {"osc3wave",0}, {"osc3oct",1}, {"osc3level",0.0f},
            {"pmamount",0.45f},
            {"filtertype",0}, {"cutoff",4500.0f}, {"resonance",0.12f}, {"keytrack",0.5f}, {"filterenvamount",0.2f},
            {"ampattack",0.02f}, {"ampsustain",0.8f}, {"amprelease",0.4f},
            {"choruson",1}, {"chorusmix",0.35f}, {"reverbon",1}, {"reverbmix",0.3f}, {"reverbsize",0.7f},
            {"tapeon",1}, {"tapewow",0.4f}, {"tapesat",0.25f}, {"tapehiss",0.18f}, {"tapetone",0.55f},
            {"mastervol",0.55f},
        }});

        p.push_back ({ "Hunter", "Lead", V {
            {"voicemode",1}, {"glidetime",0.04f},
            {"osc1wave",0}, {"osc1level",0.85f},
            {"osc2wave",0}, {"osc2level",0.7f}, {"osc2fine",6.0f},
            {"syncon",1}, {"syncdepth",3.0f},
            {"filtertype",0}, {"cutoff",2800.0f}, {"resonance",0.22f}, {"drive",0.45f}, {"keytrack",0.5f}, {"filterenvamount",0.3f},
            {"filterdecay",0.3f}, {"filtersustain",0.5f},
            {"ampattack",0.005f}, {"ampsustain",0.9f}, {"amprelease",0.25f},
            {"delayon",1}, {"delaymix",0.22f}, {"delaypingpong",1}, {"mastervol",0.5f},
        }});

        //====================================================================
        //  PLUCK
        //====================================================================
        p.push_back ({ "Static Pluck", "Pluck", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1univoices",3}, {"osc1unidetune",0.18f},
            {"filtertype",0}, {"cutoff",2400.0f}, {"resonance",0.25f}, {"drive",0.35f}, {"keytrack",0.5f}, {"filterenvamount",0.6f},
            {"filterattack",0.001f}, {"filterdecay",0.18f}, {"filtersustain",0.0f},
            {"ampattack",0.001f}, {"ampdecay",0.4f}, {"ampsustain",0.0f}, {"amprelease",0.2f},
            {"delayon",1}, {"delaymix",0.28f}, {"delaydivision",8}, {"delaypingpong",1}, {"reverbon",1}, {"reverbmix",0.18f}, {"mastervol",0.55f},
        }});

        p.push_back ({ "Arc Pluck", "Pluck", V {
            {"voicemode",0},
            {"osc1wave",1}, {"osc1pw",0.4f}, {"osc1level",0.85f},
            {"osc2wave",0}, {"osc2level",0.4f}, {"osc2oct",1},
            {"subwave",0}, {"sublevel",0.4f},
            {"filtertype",0}, {"cutoff",1800.0f}, {"resonance",0.3f}, {"drive",0.4f}, {"filterenvamount",0.65f},
            {"filterattack",0.001f}, {"filterdecay",0.14f}, {"filtersustain",0.0f},
            {"ampattack",0.001f}, {"ampdecay",0.3f}, {"ampsustain",0.05f}, {"amprelease",0.15f},
            {"reverbon",1}, {"reverbmix",0.2f}, {"mastervol",0.55f},
        }});

        p.push_back ({ "Bit Pluck", "Pluck", V {
            {"voicemode",0},
            {"osc1wave",1}, {"osc1pw",0.25f}, {"osc1level",0.85f},
            {"filtertype",0}, {"cutoff",3000.0f}, {"resonance",0.28f}, {"drive",0.4f}, {"filterenvamount",0.55f},
            {"filterattack",0.001f}, {"filterdecay",0.1f}, {"filtersustain",0.0f},
            {"ampattack",0.001f}, {"ampdecay",0.18f}, {"ampsustain",0.0f}, {"amprelease",0.1f},
            {"delayon",1}, {"delaymix",0.26f}, {"delaydivision",10}, {"delaypingpong",1}, {"mastervol",0.55f},
        }});

        //====================================================================
        //  STAB
        //====================================================================
        p.push_back ({ "Brut Stab", "Stab", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.75f}, {"osc1univoices",5}, {"osc1unidetune",0.2f}, {"osc1unispread",0.6f},
            {"osc2wave",0}, {"osc2level",0.6f}, {"osc2oct",-1},
            {"filtertype",0}, {"cutoff",2200.0f}, {"resonance",0.25f}, {"drive",0.45f}, {"filterenvamount",0.5f},
            {"filterattack",0.001f}, {"filterdecay",0.22f}, {"filtersustain",0.1f},
            {"ampattack",0.001f}, {"ampdecay",0.5f}, {"ampsustain",0.3f}, {"amprelease",0.25f},
            {"charon",1}, {"charmode",1}, {"chardrive",0.4f},                      // Diode bite
            {"reverbon",1}, {"reverbmix",0.2f}, {"delayon",1}, {"delaymix",0.18f}, {"delaypingpong",1}, {"mastervol",0.48f},
        }});

        p.push_back ({ "Power Chord", "Stab", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.8f}, {"osc1univoices",4}, {"osc1unidetune",0.15f},
            {"osc2wave",0}, {"osc2level",0.7f}, {"osc2semi",7},
            {"subwave",1}, {"sublevel",0.4f},
            {"filtertype",1}, {"cutoff",2400.0f}, {"resonance",0.22f}, {"drive",0.55f}, {"filterenvamount",0.35f},
            {"filterattack",0.001f}, {"filterdecay",0.3f}, {"filtersustain",0.25f},
            {"ampattack",0.001f}, {"ampdecay",0.6f}, {"ampsustain",0.4f}, {"amprelease",0.25f},
            {"reverbon",1}, {"reverbmix",0.18f}, {"mastervol",0.5f},
        }});

        //====================================================================
        //  FX / DRONE
        //====================================================================
        p.push_back ({ "Dread Drone", "FX", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.6f}, {"osc1univoices",7}, {"osc1unidetune",0.4f}, {"osc1unispread",0.8f},
            {"osc2wave",0}, {"osc2level",0.5f}, {"osc2oct",-1}, {"osc2univoices",5}, {"osc2unidetune",0.4f},
            {"subwave",0}, {"sublevel",0.5f}, {"noiselevel",0.05f}, {"noisecolor",0.7f},
            {"filtertype",0}, {"cutoff",800.0f}, {"resonance",0.3f}, {"drive",0.4f},
            {"lfo1shape",0}, {"lfo1rate",0.1f}, {"mod3source",3}, {"mod3dest",19}, {"mod3amount",0.25f},
            {"ampattack",0.5f}, {"ampsustain",1.0f}, {"amprelease",0.8f},
            {"reverbon",1}, {"reverbmix",0.35f}, {"reverbsize",0.85f}, {"reverbdecay",0.8f}, {"mastervol",0.5f},
        }});

        p.push_back ({ "Siren Riser", "FX", V {
            {"voicemode",1}, {"glidetime",0.0f},
            {"osc1wave",0}, {"osc1level",0.7f}, {"osc1univoices",3}, {"osc1unidetune",0.25f},
            {"filtertype",1}, {"cutoff",1500.0f}, {"resonance",0.4f}, {"drive",0.4f},
            {"lfo1shape",0}, {"lfo1rate",4.0f}, {"mod3source",3}, {"mod3dest",1}, {"mod3amount",0.2f},
            {"lfo2shape",2}, {"lfo2sync",1}, {"lfo2division",4}, {"mod4source",4}, {"mod4dest",19}, {"mod4amount",0.35f},
            {"ampattack",0.05f}, {"ampsustain",1.0f}, {"amprelease",0.4f},
            {"delayon",1}, {"delaymix",0.28f}, {"delaypingpong",1}, {"reverbon",1}, {"reverbmix",0.28f}, {"mastervol",0.5f},
        }});

        //====================================================================
        //  BASS (expanded)
        //====================================================================
        p.push_back ({ "Perturbator", "Bass", V {
            {"voicemode",1}, {"glidetime",0.025f},
            {"osc1wave",0}, {"osc1level",0.9f}, {"osc1univoices",2}, {"osc1unidetune",0.1f}, {"osc1uniphase",1},
            {"osc2wave",0}, {"osc2level",0.6f}, {"osc2oct",-1},
            {"subwave",1}, {"sublevel",0.7f},
            {"filtertype",0}, {"cutoff",500.0f}, {"resonance",0.2f}, {"drive",0.55f}, {"filterenvamount",0.5f},
            {"filterattack",0.001f}, {"filterdecay",0.2f}, {"filtersustain",0.2f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.1f},
            {"charon",1}, {"charmode",1}, {"chardrive",0.4f}, {"chartone",0.65f},
            {"mastervol",0.58f},
        }});

        p.push_back ({ "Dusk Bass", "Bass", V {
            {"voicemode",2}, {"glidetime",0.06f},
            {"osc1wave",0}, {"osc1level",0.8f}, {"osc1univoices",2}, {"osc1unidetune",0.1f},
            {"subwave",0}, {"sublevel",0.8f},
            {"filtertype",0}, {"cutoff",420.0f}, {"resonance",0.15f}, {"drive",0.35f}, {"filterenvamount",0.3f},
            {"filterdecay",0.35f}, {"filtersustain",0.4f},
            {"ampattack",0.004f}, {"ampsustain",0.95f}, {"amprelease",0.22f},
            {"charon",1}, {"charmode",0}, {"chardrive",0.25f}, {"mastervol",0.6f},
        }});

        p.push_back ({ "Grinder", "Bass", V {
            {"voicemode",1}, {"glidetime",0.02f},
            {"osc1wave",1}, {"osc1pw",0.35f}, {"osc1level",0.85f}, {"osc1univoices",2}, {"osc1unidetune",0.09f},
            {"osc2wave",0}, {"osc2level",0.55f},
            {"subwave",1}, {"sublevel",0.7f},
            {"filtertype",1}, {"cutoff",560.0f}, {"resonance",0.3f}, {"drive",0.6f}, {"filterenvamount",0.4f},
            {"filterdecay",0.2f}, {"filtersustain",0.2f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.1f},
            {"charon",1}, {"charmode",1}, {"chardrive",0.5f}, {"chartone",0.6f}, {"mastervol",0.54f},
        }});

        p.push_back ({ "Wobbler", "Bass", V {
            {"voicemode",1}, {"glidetime",0.03f},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1univoices",3}, {"osc1unidetune",0.16f},
            {"subwave",0}, {"sublevel",0.7f},
            {"filtertype",0}, {"cutoff",500.0f}, {"resonance",0.32f}, {"drive",0.45f},
            {"lfo1shape",0}, {"lfo1sync",1}, {"lfo1division",7}, {"mod3source",3}, {"mod3dest",19}, {"mod3amount",0.4f},
            {"ampattack",0.002f}, {"ampsustain",0.92f}, {"amprelease",0.14f}, {"mastervol",0.56f},
        }});

        p.push_back ({ "Deep Diver", "Bass", V {
            {"voicemode",1}, {"glidetime",0.04f},
            {"osc1wave",3}, {"osc1level",0.7f},
            {"subwave",0}, {"suboct",1}, {"sublevel",0.95f},
            {"filtertype",0}, {"cutoff",240.0f}, {"resonance",0.12f}, {"drive",0.4f}, {"filterenvamount",0.25f},
            {"filterdecay",0.3f}, {"filtersustain",0.35f},
            {"ampattack",0.001f}, {"ampsustain",0.95f}, {"amprelease",0.2f}, {"mastervol",0.6f},
        }});

        p.push_back ({ "Overlord", "Bass", V {
            {"voicemode",1}, {"glidetime",0.02f},
            {"osc1wave",0}, {"osc1level",0.95f},
            {"osc2wave",0}, {"osc2level",0.6f},
            {"syncon",1}, {"syncdepth",3.0f},
            {"subwave",1}, {"sublevel",0.65f},
            {"filtertype",1}, {"cutoff",520.0f}, {"resonance",0.32f}, {"drive",0.6f}, {"filterenvamount",0.4f},
            {"filterdecay",0.24f}, {"filtersustain",0.15f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.1f},
            {"charon",1}, {"charmode",1}, {"chardrive",0.45f}, {"mastervol",0.52f},
        }});

        p.push_back ({ "Rust", "Bass", V {
            {"voicemode",1}, {"glidetime",0.03f},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1univoices",2}, {"osc1unidetune",0.1f},
            {"subwave",1}, {"sublevel",0.7f},
            {"filtertype",0}, {"cutoff",600.0f}, {"resonance",0.2f}, {"drive",0.45f}, {"filterenvamount",0.35f},
            {"filterdecay",0.22f}, {"filtersustain",0.2f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.12f},
            {"charon",1}, {"charmode",3}, {"chardrive",0.4f}, {"chartone",0.55f}, {"mastervol",0.56f},
        }});

        p.push_back ({ "Pulse Bass", "Bass", V {
            {"voicemode",1}, {"glidetime",0.02f},
            {"osc1wave",1}, {"osc1pw",0.5f}, {"osc1level",0.85f},
            {"subwave",0}, {"sublevel",0.7f},
            {"filtertype",0}, {"cutoff",620.0f}, {"resonance",0.22f}, {"drive",0.45f}, {"filterenvamount",0.4f},
            {"filterdecay",0.22f}, {"filtersustain",0.25f},
            {"lfo1shape",0}, {"lfo1rate",3.0f}, {"mod3source",3}, {"mod3dest",8}, {"mod3amount",0.3f}, // PWM
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.12f}, {"mastervol",0.57f},
        }});

        p.push_back ({ "FM Growl", "Bass", V {
            {"voicemode",1}, {"glidetime",0.03f},
            {"osc1wave",3}, {"osc1level",0.85f},
            {"osc3wave",0}, {"osc3oct",1}, {"osc3level",0.0f}, {"pmamount",0.5f},
            {"subwave",1}, {"sublevel",0.7f},
            {"filtertype",0}, {"cutoff",700.0f}, {"resonance",0.22f}, {"drive",0.5f}, {"filterenvamount",0.4f},
            {"filterdecay",0.2f}, {"filtersustain",0.2f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.12f}, {"mastervol",0.55f},
        }});

        p.push_back ({ "Octaver", "Bass", V {
            {"voicemode",1}, {"glidetime",0.03f},
            {"osc1wave",0}, {"osc1level",0.8f},
            {"osc2wave",0}, {"osc2level",0.5f}, {"osc2oct",1},
            {"subwave",1}, {"sublevel",0.8f},
            {"filtertype",0}, {"cutoff",520.0f}, {"resonance",0.18f}, {"drive",0.45f}, {"filterenvamount",0.4f},
            {"filterdecay",0.2f}, {"filtersustain",0.25f},
            {"ampattack",0.001f}, {"ampsustain",0.92f}, {"amprelease",0.12f}, {"mastervol",0.57f},
        }});

        //====================================================================
        //  LEAD (expanded)
        //====================================================================
        p.push_back ({ "Anthem Lead", "Lead", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.75f}, {"osc1univoices",7}, {"osc1unidetune",0.3f}, {"osc1unispread",0.75f},
            {"osc2wave",0}, {"osc2level",0.45f}, {"osc2oct",-1},
            {"filtertype",0}, {"cutoff",3600.0f}, {"resonance",0.18f}, {"drive",0.35f}, {"keytrack",0.4f}, {"filterenvamount",0.25f},
            {"filterdecay",0.4f}, {"filtersustain",0.65f},
            {"ampattack",0.006f}, {"ampsustain",0.85f}, {"amprelease",0.35f},
            {"choruson",1}, {"chorusmix",0.3f}, {"delayon",1}, {"delaymix",0.24f}, {"delaypingpong",1},
            {"reverbon",1}, {"reverbmix",0.22f}, {"charon",1}, {"charmode",0}, {"chardrive",0.3f}, {"mastervol",0.46f},
        }});

        p.push_back ({ "Chrome Lead", "Lead", V {
            {"voicemode",1}, {"glidetime",0.04f},
            {"osc1wave",3}, {"osc1level",0.85f},
            {"osc3wave",0}, {"osc3oct",1}, {"osc3level",0.0f}, {"pmamount",0.4f},
            {"filtertype",0}, {"cutoff",4500.0f}, {"resonance",0.15f}, {"keytrack",0.5f}, {"filterenvamount",0.2f},
            {"ampattack",0.006f}, {"ampsustain",0.85f}, {"amprelease",0.3f},
            {"delayon",1}, {"delaymix",0.24f}, {"delaypingpong",1}, {"reverbon",1}, {"reverbmix",0.22f}, {"mastervol",0.5f},
        }});

        p.push_back ({ "Screamer", "Lead", V {
            {"voicemode",1}, {"glidetime",0.05f},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1univoices",3}, {"osc1unidetune",0.16f},
            {"filtertype",1}, {"cutoff",2200.0f}, {"resonance",0.42f}, {"drive",0.55f}, {"keytrack",0.7f}, {"filterenvamount",0.35f},
            {"filterdecay",0.4f}, {"filtersustain",0.45f},
            {"ampattack",0.006f}, {"ampsustain",0.9f}, {"amprelease",0.28f},
            {"lfo1shape",0}, {"lfo1rate",6.0f}, {"lfo1fade",0.5f}, {"mod3source",3}, {"mod3dest",1}, {"mod3amount",0.04f},
            {"charon",1}, {"charmode",1}, {"chardrive",0.35f}, {"reverbon",1}, {"reverbmix",0.2f}, {"mastervol",0.46f},
        }});

        p.push_back ({ "Retro Lead", "Lead", V {
            {"voicemode",1}, {"glidetime",0.05f},
            {"osc1wave",1}, {"osc1pw",0.45f}, {"osc1level",0.85f}, {"osc1univoices",2}, {"osc1unidetune",0.12f},
            {"filtertype",0}, {"cutoff",3000.0f}, {"resonance",0.2f}, {"keytrack",0.5f}, {"filterenvamount",0.25f},
            {"filterdecay",0.4f}, {"filtersustain",0.6f},
            {"ampattack",0.008f}, {"ampsustain",0.88f}, {"amprelease",0.3f},
            {"choruson",1}, {"chorusmix",0.3f}, {"delayon",1}, {"delaymix",0.2f},
            {"tapeon",1}, {"tapewow",0.35f}, {"tapesat",0.25f}, {"tapehiss",0.15f}, {"mastervol",0.5f},
        }});

        p.push_back ({ "Wide Saw", "Lead", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.7f}, {"osc1univoices",7}, {"osc1unidetune",0.35f}, {"osc1unispread",0.85f},
            {"filtertype",0}, {"cutoff",4000.0f}, {"resonance",0.15f}, {"keytrack",0.4f}, {"filterenvamount",0.2f},
            {"ampattack",0.01f}, {"ampsustain",0.85f}, {"amprelease",0.35f},
            {"choruson",1}, {"chorusmix",0.35f}, {"reverbon",1}, {"reverbmix",0.25f}, {"mastervol",0.46f},
        }});

        p.push_back ({ "Duo Lead", "Lead", V {
            {"voicemode",1}, {"glidetime",0.04f},
            {"osc1wave",0}, {"osc1level",0.8f}, {"osc1univoices",3}, {"osc1unidetune",0.15f},
            {"osc2wave",0}, {"osc2level",0.7f}, {"osc2semi",7},
            {"filtertype",0}, {"cutoff",3400.0f}, {"resonance",0.2f}, {"keytrack",0.55f}, {"filterenvamount",0.25f},
            {"filterdecay",0.4f}, {"filtersustain",0.6f},
            {"ampattack",0.006f}, {"ampsustain",0.9f}, {"amprelease",0.3f},
            {"delayon",1}, {"delaymix",0.22f}, {"reverbon",1}, {"reverbmix",0.2f}, {"mastervol",0.48f},
        }});

        p.push_back ({ "Laser", "Lead", V {
            {"voicemode",1}, {"glidetime",0.02f},
            {"osc1wave",0}, {"osc1level",0.85f},
            {"syncon",1}, {"syncdepth",6.0f},
            {"filtertype",0}, {"cutoff",3800.0f}, {"resonance",0.25f}, {"drive",0.4f}, {"keytrack",0.6f}, {"filterenvamount",0.5f},
            {"filterattack",0.001f}, {"filterdecay",0.15f}, {"filtersustain",0.2f},
            {"ampattack",0.001f}, {"ampdecay",0.3f}, {"ampsustain",0.4f}, {"amprelease",0.15f},
            {"delayon",1}, {"delaymix",0.28f}, {"delaypingpong",1}, {"mastervol",0.5f},
        }});

        //====================================================================
        //  PLUCK (expanded)
        //====================================================================
        p.push_back ({ "Glass Pluck", "Pluck", V {
            {"voicemode",0},
            {"osc1wave",3}, {"osc1level",0.8f},
            {"osc2wave",0}, {"osc2level",0.4f}, {"osc2oct",1},
            {"filtertype",0}, {"cutoff",3200.0f}, {"resonance",0.25f}, {"keytrack",0.5f}, {"filterenvamount",0.5f},
            {"filterattack",0.001f}, {"filterdecay",0.16f}, {"filtersustain",0.0f},
            {"ampattack",0.001f}, {"ampdecay",0.35f}, {"ampsustain",0.0f}, {"amprelease",0.2f},
            {"delayon",1}, {"delaymix",0.28f}, {"delaypingpong",1}, {"reverbon",1}, {"reverbmix",0.25f}, {"mastervol",0.54f},
        }});

        p.push_back ({ "Dark Pluck", "Pluck", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1univoices",2}, {"osc1unidetune",0.14f},
            {"subwave",0}, {"sublevel",0.4f},
            {"filtertype",0}, {"cutoff",1600.0f}, {"resonance",0.3f}, {"drive",0.4f}, {"filterenvamount",0.6f},
            {"filterattack",0.001f}, {"filterdecay",0.16f}, {"filtersustain",0.0f},
            {"ampattack",0.001f}, {"ampdecay",0.3f}, {"ampsustain",0.0f}, {"amprelease",0.18f},
            {"reverbon",1}, {"reverbmix",0.22f}, {"mastervol",0.55f},
        }});

        p.push_back ({ "Mallet", "Pluck", V {
            {"voicemode",0},
            {"osc1wave",2}, {"osc1level",0.8f},
            {"osc2wave",3}, {"osc2level",0.4f}, {"osc2oct",1},
            {"filtertype",0}, {"cutoff",2600.0f}, {"resonance",0.15f}, {"keytrack",0.5f}, {"filterenvamount",0.5f},
            {"filterattack",0.001f}, {"filterdecay",0.12f}, {"filtersustain",0.0f},
            {"ampattack",0.001f}, {"ampdecay",0.25f}, {"ampsustain",0.0f}, {"amprelease",0.15f},
            {"reverbon",1}, {"reverbmix",0.2f}, {"mastervol",0.56f},
        }});

        p.push_back ({ "Blip", "Pluck", V {
            {"voicemode",0},
            {"osc1wave",1}, {"osc1pw",0.2f}, {"osc1level",0.85f},
            {"filtertype",1}, {"cutoff",3200.0f}, {"resonance",0.3f}, {"drive",0.4f}, {"filterenvamount",0.5f},
            {"filterattack",0.001f}, {"filterdecay",0.08f}, {"filtersustain",0.0f},
            {"ampattack",0.001f}, {"ampdecay",0.14f}, {"ampsustain",0.0f}, {"amprelease",0.1f},
            {"delayon",1}, {"delaymix",0.3f}, {"delaydivision",10}, {"delaypingpong",1}, {"mastervol",0.55f},
        }});

        //====================================================================
        //  KEYS
        //====================================================================
        p.push_back ({ "Synth Keys", "Keys", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.7f}, {"osc1univoices",2}, {"osc1unidetune",0.12f},
            {"osc2wave",1}, {"osc2pw",0.45f}, {"osc2level",0.5f},
            {"filtertype",0}, {"cutoff",2400.0f}, {"resonance",0.15f}, {"keytrack",0.4f}, {"filterenvamount",0.3f},
            {"filterdecay",0.5f}, {"filtersustain",0.4f},
            {"ampattack",0.005f}, {"ampdecay",0.8f}, {"ampsustain",0.4f}, {"amprelease",0.3f},
            {"choruson",1}, {"chorusmix",0.3f}, {"reverbon",1}, {"reverbmix",0.22f}, {"mastervol",0.52f},
        }});

        p.push_back ({ "Cold Keys", "Keys", V {
            {"voicemode",0},
            {"osc1wave",3}, {"osc1level",0.7f},
            {"osc2wave",0}, {"osc2level",0.45f}, {"osc2oct",1},
            {"filtertype",0}, {"cutoff",2000.0f}, {"resonance",0.12f}, {"keytrack",0.5f}, {"filterenvamount",0.25f},
            {"filterdecay",0.6f}, {"filtersustain",0.3f},
            {"ampattack",0.008f}, {"ampdecay",0.9f}, {"ampsustain",0.3f}, {"amprelease",0.4f},
            {"reverbon",1}, {"reverbmix",0.3f}, {"reverbsize",0.7f},
            {"tapeon",1}, {"tapewow",0.3f}, {"tapehiss",0.12f}, {"mastervol",0.54f},
        }});

        p.push_back ({ "Bell Keys", "Keys", V {
            {"voicemode",0},
            {"osc1wave",3}, {"osc1level",0.8f},
            {"osc3wave",3}, {"osc3oct",2}, {"osc3level",0.0f}, {"pmamount",0.3f},
            {"filtertype",0}, {"cutoff",4000.0f}, {"resonance",0.1f}, {"keytrack",0.5f},
            {"ampattack",0.002f}, {"ampdecay",0.7f}, {"ampsustain",0.2f}, {"amprelease",0.5f},
            {"choruson",1}, {"chorusmix",0.3f}, {"reverbon",1}, {"reverbmix",0.28f}, {"mastervol",0.54f},
        }});

        //====================================================================
        //  STAB (expanded)
        //====================================================================
        p.push_back ({ "Rave Stab", "Stab", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.8f}, {"osc1univoices",4}, {"osc1unidetune",0.18f},
            {"osc2wave",1}, {"osc2pw",0.4f}, {"osc2level",0.5f}, {"osc2oct",-1},
            {"filtertype",1}, {"cutoff",2000.0f}, {"resonance",0.3f}, {"drive",0.5f}, {"filterenvamount",0.5f},
            {"filterattack",0.001f}, {"filterdecay",0.18f}, {"filtersustain",0.1f},
            {"ampattack",0.001f}, {"ampdecay",0.4f}, {"ampsustain",0.2f}, {"amprelease",0.2f},
            {"delayon",1}, {"delaymix",0.24f}, {"delaypingpong",1}, {"reverbon",1}, {"reverbmix",0.2f}, {"mastervol",0.5f},
        }});

        p.push_back ({ "Hit Stab", "Stab", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.8f}, {"osc1univoices",5}, {"osc1unidetune",0.22f}, {"osc1unispread",0.6f},
            {"osc2wave",0}, {"osc2level",0.6f}, {"osc2oct",-1},
            {"noiselevel",0.05f},
            {"filtertype",0}, {"cutoff",2600.0f}, {"resonance",0.2f}, {"drive",0.45f}, {"filterenvamount",0.55f},
            {"filterattack",0.001f}, {"filterdecay",0.2f}, {"filtersustain",0.0f},
            {"ampattack",0.001f}, {"ampdecay",0.45f}, {"ampsustain",0.0f}, {"amprelease",0.25f},
            {"reverbon",1}, {"reverbmix",0.25f}, {"mastervol",0.5f},
        }});

        //====================================================================
        //  PAD
        //====================================================================
        p.push_back ({ "Warm Pad", "Pad", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.6f}, {"osc1univoices",5}, {"osc1unidetune",0.24f}, {"osc1unispread",0.7f},
            {"osc2wave",0}, {"osc2level",0.4f}, {"osc2oct",-1},
            {"filtertype",0}, {"cutoff",1600.0f}, {"resonance",0.14f}, {"filterenvamount",0.2f},
            {"filterattack",0.6f}, {"filterdecay",1.0f}, {"filtersustain",0.6f},
            {"ampattack",0.6f}, {"ampdecay",1.0f}, {"ampsustain",0.9f}, {"amprelease",1.2f},
            {"choruson",1}, {"chorusmix",0.35f}, {"reverbon",1}, {"reverbmix",0.35f}, {"reverbsize",0.8f}, {"mastervol",0.5f},
        }});

        p.push_back ({ "Dark Pad", "Pad", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.55f}, {"osc1univoices",7}, {"osc1unidetune",0.35f}, {"osc1unispread",0.8f},
            {"osc2wave",0}, {"osc2level",0.4f}, {"osc2oct",-2},
            {"noiselevel",0.04f}, {"noisecolor",0.7f},
            {"filtertype",0}, {"cutoff",1000.0f}, {"resonance",0.2f}, {"filterenvamount",0.2f},
            {"filterattack",1.0f}, {"filtersustain",0.5f},
            {"ampattack",0.8f}, {"ampsustain",0.9f}, {"amprelease",1.4f},
            {"lfo1shape",0}, {"lfo1rate",0.15f}, {"mod3source",3}, {"mod3dest",19}, {"mod3amount",0.12f},
            {"reverbon",1}, {"reverbmix",0.4f}, {"reverbsize",0.85f}, {"reverbdecay",0.8f}, {"mastervol",0.5f},
        }});

        p.push_back ({ "Void Pad", "Pad", V {
            {"voicemode",0},
            {"osc1wave",3}, {"osc1level",0.55f}, {"osc1univoices",3}, {"osc1unidetune",0.2f},
            {"osc3wave",0}, {"osc3oct",1}, {"osc3level",0.0f}, {"pmamount",0.25f},
            {"filtertype",0}, {"cutoff",1800.0f}, {"resonance",0.12f},
            {"ampattack",1.2f}, {"ampsustain",0.85f}, {"amprelease",1.6f},
            {"choruson",1}, {"chorusmix",0.4f}, {"reverbon",1}, {"reverbmix",0.42f}, {"reverbsize",0.9f}, {"mastervol",0.5f},
        }});

        //====================================================================
        //  ARP / SEQ (gated via LFO -> osc level)
        //====================================================================
        p.push_back ({ "Gated Seq", "Arp", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1univoices",3}, {"osc1unidetune",0.16f},
            {"subwave",0}, {"sublevel",0.4f},
            {"filtertype",0}, {"cutoff",2200.0f}, {"resonance",0.25f}, {"drive",0.4f}, {"keytrack",0.4f}, {"filterenvamount",0.3f},
            {"filterdecay",0.3f}, {"filtersustain",0.4f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.05f},
            {"lfo2shape",4}, {"lfo2sync",1}, {"lfo2division",10}, // square 1/16
            {"mod4source",4}, {"mod4dest",5}, {"mod4amount",1.0f}, // LFO2 -> Osc1 Level (gate)
            {"delayon",1}, {"delaymix",0.24f}, {"delaydivision",8}, {"delaypingpong",1},
            {"reverbon",1}, {"reverbmix",0.2f}, {"mastervol",0.5f},
        }});

        p.push_back ({ "Pulse Seq", "Arp", V {
            {"voicemode",1}, {"glidetime",0.0f},
            {"osc1wave",1}, {"osc1pw",0.4f}, {"osc1level",0.85f},
            {"subwave",1}, {"sublevel",0.6f},
            {"filtertype",1}, {"cutoff",1400.0f}, {"resonance",0.3f}, {"drive",0.45f}, {"filterenvamount",0.5f},
            {"filterattack",0.001f}, {"filterdecay",0.14f}, {"filtersustain",0.1f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.05f},
            {"lfo2shape",4}, {"lfo2sync",1}, {"lfo2division",10},
            {"mod4source",4}, {"mod4dest",5}, {"mod4amount",1.0f},
            {"charon",1}, {"charmode",1}, {"chardrive",0.35f}, {"mastervol",0.52f},
        }});

        //====================================================================
        //  FX (expanded)
        //====================================================================
        p.push_back ({ "Riser", "FX", V {
            {"voicemode",1}, {"glidetime",0.0f},
            {"osc1wave",0}, {"osc1level",0.7f}, {"osc1univoices",5}, {"osc1unidetune",0.3f},
            {"noiselevel",0.1f},
            {"filtertype",0}, {"cutoff",800.0f}, {"resonance",0.35f}, {"drive",0.4f},
            {"lfo1shape",2}, {"lfo1sync",1}, {"lfo1division",0}, // saw-up 1/1
            {"mod3source",3}, {"mod3dest",19}, {"mod3amount",0.6f}, // sweep cutoff up
            {"ampattack",0.02f}, {"ampsustain",1.0f}, {"amprelease",0.3f},
            {"reverbon",1}, {"reverbmix",0.3f}, {"delayon",1}, {"delaymix",0.25f}, {"mastervol",0.48f},
        }});

        p.push_back ({ "Impact", "FX", V {
            {"voicemode",1},
            {"osc1wave",0}, {"osc1level",0.7f},
            {"subwave",0}, {"suboct",1}, {"sublevel",0.9f},
            {"noiselevel",0.3f}, {"noisecolor",0.4f},
            {"filtertype",0}, {"cutoff",900.0f}, {"resonance",0.2f}, {"drive",0.5f}, {"filterenvamount",0.6f},
            {"filterattack",0.001f}, {"filterdecay",0.3f}, {"filtersustain",0.0f},
            {"ampattack",0.001f}, {"ampdecay",0.6f}, {"ampsustain",0.0f}, {"amprelease",0.4f},
            {"reverbon",1}, {"reverbmix",0.4f}, {"reverbsize",0.85f}, {"mastervol",0.5f},
        }});

        p.push_back ({ "Downlifter", "FX", V {
            {"voicemode",1}, {"glidetime",0.0f},
            {"osc1wave",0}, {"osc1level",0.7f}, {"osc1univoices",3}, {"osc1unidetune",0.25f},
            {"filtertype",1}, {"cutoff",2000.0f}, {"resonance",0.35f}, {"drive",0.4f},
            {"lfo1shape",3}, {"lfo1sync",1}, {"lfo1division",0}, // saw-down 1/1
            {"mod3source",3}, {"mod3dest",1}, {"mod3amount",0.5f}, // pitch drop
            {"mod4source",3}, {"mod4dest",19}, {"mod4amount",0.4f},
            {"ampattack",0.01f}, {"ampsustain",1.0f}, {"amprelease",0.3f},
            {"reverbon",1}, {"reverbmix",0.3f}, {"delayon",1}, {"delaymix",0.25f}, {"delaypingpong",1}, {"mastervol",0.48f},
        }});

        return p;
    }
}
