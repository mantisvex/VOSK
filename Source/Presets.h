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
//  Default matrix slot 1 is Env2 -> SyncDepth (0.5), so any preset that turns
//  Hard Sync on inherits the classic envelope-swept growl for free.
//
//  Voice is leads + bass first (Carpenter Brut / Perturbator / darksynth):
//  heavy, driven, ready to play.
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
        //  BASS
        //====================================================================
        p.push_back ({ "Hydra Bass", "Bass", V {
            {"voicemode",1}, {"glidetime",0.03f},
            {"osc1wave",0}, {"osc1level",0.95f}, {"osc1univoices",3}, {"osc1unidetune",0.16f}, {"osc1uniphase",1},
            {"osc2wave",0}, {"osc2level",0.7f},
            {"syncon",1}, {"syncdepth",8.0f},
            {"subwave",1}, {"suboct",0}, {"sublevel",0.8f},
            {"filtertype",0}, {"cutoff",520.0f}, {"resonance",0.28f}, {"drive",0.55f}, {"filterenvamount",0.6f},
            {"filterattack",0.001f}, {"filterdecay",0.20f}, {"filtersustain",0.18f}, {"filterrelease",0.18f},
            {"ampattack",0.001f}, {"ampdecay",0.3f}, {"ampsustain",0.85f}, {"amprelease",0.14f},
            {"reverbon",1}, {"reverbmix",0.10f}, {"reverbsize",0.4f},
        }});

        p.push_back ({ "Turbo Killer", "Bass", V {
            {"voicemode",1}, {"glidetime",0.02f},
            {"osc1wave",0}, {"osc1level",1.0f}, {"osc1univoices",2}, {"osc1unidetune",0.1f}, {"osc1uniphase",1},
            {"osc2wave",0}, {"osc2level",0.85f}, {"osc2semi",0},
            {"syncon",1}, {"syncdepth",2.0f},
            {"subwave",1}, {"sublevel",0.7f},
            {"filtertype",1}, {"cutoff",430.0f}, {"resonance",0.35f}, {"drive",0.78f}, {"filterenvamount",0.5f},
            {"filterattack",0.001f}, {"filterdecay",0.16f}, {"filtersustain",0.15f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.1f},
            {"mod1amount",0.8f}, // Env2 -> SyncDepth, heavier growl
        }});

        p.push_back ({ "Magonia Low", "Bass", V {
            {"voicemode",1}, {"glidetime",0.05f},
            {"osc1wave",0}, {"osc1level",0.9f}, {"osc1univoices",5}, {"osc1unidetune",0.28f}, {"osc1unispread",0.4f}, {"osc1uniphase",1},
            {"osc2wave",0}, {"osc2level",0.6f}, {"osc2oct",-1},
            {"subwave",0}, {"suboct",1}, {"sublevel",0.9f},
            {"filtertype",0}, {"cutoff",340.0f}, {"resonance",0.2f}, {"drive",0.6f}, {"filterenvamount",0.45f},
            {"filterattack",0.001f}, {"filterdecay",0.35f}, {"filtersustain",0.25f},
            {"ampattack",0.002f}, {"ampsustain",0.9f}, {"amprelease",0.2f},
            {"choruson",1}, {"chorusmix",0.25f}, {"reverbon",1}, {"reverbmix",0.12f},
        }});

        p.push_back ({ "Doom Reese", "Bass", V {
            {"voicemode",1}, {"glidetime",0.04f},
            {"osc1wave",0}, {"osc1level",0.9f}, {"osc1univoices",4}, {"osc1unidetune",0.35f}, {"osc1unispread",0.3f},
            {"osc2wave",0}, {"osc2level",0.9f}, {"osc2fine",-12.0f}, {"osc2univoices",3}, {"osc2unidetune",0.3f},
            {"subwave",0}, {"sublevel",0.7f},
            {"filtertype",0}, {"cutoff",600.0f}, {"resonance",0.22f}, {"drive",0.5f}, {"filterenvamount",0.3f},
            {"filterdecay",0.4f}, {"filtersustain",0.4f},
            {"ampattack",0.003f}, {"ampsustain",0.95f}, {"amprelease",0.25f},
            {"lfo1shape",0}, {"lfo1rate",0.18f}, {"mod3source",3}, {"mod3dest",19}, {"mod3amount",0.15f}, // LFO1 -> Cutoff slow movement
            {"reverbon",1}, {"reverbmix",0.1f},
        }});

        p.push_back ({ "Sewer Sub", "Bass", V {
            {"voicemode",1}, {"glidetime",0.06f},
            {"osc1wave",2}, {"osc1level",0.6f},
            {"osc2wave",0}, {"osc2level",0.5f}, {"osc2oct",0},
            {"subwave",1}, {"suboct",1}, {"sublevel",1.0f},
            {"noiselevel",0.06f}, {"noisecolor",0.8f},
            {"filtertype",0}, {"cutoff",260.0f}, {"resonance",0.15f}, {"drive",0.45f}, {"filterenvamount",0.35f},
            {"filterdecay",0.25f}, {"filtersustain",0.2f},
            {"ampattack",0.001f}, {"ampsustain",0.95f}, {"amprelease",0.15f},
        }});

        p.push_back ({ "Acid Fang", "Bass", V {
            {"voicemode",1}, {"glidetime",0.05f},
            {"osc1wave",0}, {"osc1level",0.95f},
            {"subwave",1}, {"sublevel",0.5f},
            {"filtertype",1}, {"cutoff",380.0f}, {"resonance",0.72f}, {"drive",0.6f}, {"filterenvamount",0.7f},
            {"filterattack",0.001f}, {"filterdecay",0.18f}, {"filtersustain",0.05f}, {"filterrelease",0.12f},
            {"ampattack",0.001f}, {"ampsustain",0.85f}, {"amprelease",0.1f},
            {"delayon",1}, {"delaymix",0.16f}, {"delaydivision",9}, // 1/8T
        }});

        p.push_back ({ "Concrete Sub", "Bass", V {
            {"voicemode",1}, {"glidetime",0.025f},
            {"osc1wave",1}, {"osc1pw",0.5f}, {"osc1level",0.85f},
            {"osc2wave",0}, {"osc2level",0.6f},
            {"subwave",1}, {"suboct",0}, {"sublevel",0.85f},
            {"filtertype",0}, {"cutoff",480.0f}, {"resonance",0.18f}, {"drive",0.65f}, {"filterenvamount",0.5f},
            {"filterdecay",0.18f}, {"filtersustain",0.2f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.12f},
            {"mod4source",3}, {"mod4dest",9}, {"mod4amount",0.2f}, {"lfo1shape",0}, {"lfo1rate",4.0f}, // PWM movement on osc1 (dest Osc2PW? use osc1 PW = dest 8)
        }});

        p.push_back ({ "Blood Machine", "Bass", V {
            {"voicemode",1}, {"glidetime",0.03f},
            {"osc1wave",0}, {"osc1level",1.0f},
            {"osc2wave",0}, {"osc2level",0.7f},
            {"syncon",1}, {"syncdepth",0.0f},
            {"subwave",1}, {"sublevel",0.7f},
            {"filtertype",1}, {"cutoff",500.0f}, {"resonance",0.4f}, {"drive",0.7f}, {"filterenvamount",0.4f},
            {"filterattack",0.001f}, {"filterdecay",0.28f}, {"filtersustain",0.1f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.12f},
            {"mod1amount",0.95f}, // Env2 -> SyncDepth full screaming sweep
            {"reverbon",1}, {"reverbmix",0.1f},
        }});

        p.push_back ({ "Vendetta PM", "Bass", V {
            {"voicemode",1}, {"glidetime",0.03f},
            {"osc1wave",3}, {"osc1level",0.9f},
            {"osc3wave",0}, {"osc3level",0.0f}, {"osc3oct",1},
            {"pmamount",0.4f},
            {"subwave",1}, {"sublevel",0.7f},
            {"filtertype",0}, {"cutoff",700.0f}, {"resonance",0.25f}, {"drive",0.55f}, {"filterenvamount",0.45f},
            {"filterdecay",0.2f}, {"filtersustain",0.2f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.12f},
        }});

        p.push_back ({ "Roller", "Bass", V {
            {"voicemode",2}, {"glidetime",0.08f},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1univoices",3}, {"osc1unidetune",0.4f}, {"osc1unispread",0.5f},
            {"osc2wave",0}, {"osc2level",0.85f}, {"osc2fine",13.0f},
            {"subwave",0}, {"sublevel",0.6f},
            {"filtertype",0}, {"cutoff",520.0f}, {"resonance",0.3f}, {"drive",0.45f}, {"filterenvamount",0.25f},
            {"filterdecay",0.5f}, {"filtersustain",0.5f},
            {"ampattack",0.004f}, {"ampsustain",0.95f}, {"amprelease",0.3f},
            {"choruson",1}, {"chorusmix",0.3f},
        }});

        p.push_back ({ "Nightcrawler", "Bass", V {
            {"voicemode",1}, {"glidetime",0.05f},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1univoices",2}, {"osc1unidetune",0.12f},
            {"osc2wave",2}, {"osc2level",0.4f}, {"osc2oct",-1},
            {"subwave",0}, {"sublevel",0.8f},
            {"filtertype",0}, {"cutoff",420.0f}, {"resonance",0.2f}, {"drive",0.4f}, {"filterenvamount",0.5f},
            {"filterdecay",0.3f}, {"filtersustain",0.15f},
            {"ampattack",0.002f}, {"ampsustain",0.85f}, {"amprelease",0.2f},
            {"reverbon",1}, {"reverbmix",0.14f}, {"reverbsize",0.5f},
        }});

        p.push_back ({ "Gravewalker", "Bass", V {
            {"voicemode",1}, {"glidetime",0.04f},
            {"osc1wave",0}, {"osc1level",0.9f}, {"osc1oct",-1}, {"osc1univoices",4}, {"osc1unidetune",0.22f},
            {"subwave",1}, {"suboct",1}, {"sublevel",0.9f},
            {"filtertype",1}, {"cutoff",300.0f}, {"resonance",0.45f}, {"drive",0.7f}, {"filterenvamount",0.5f},
            {"filterdecay",0.3f}, {"filtersustain",0.1f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.18f},
            {"hero",0.25f},
        }});

        p.push_back ({ "Distortion Pedal", "Bass", V {
            {"voicemode",1}, {"glidetime",0.02f},
            {"osc1wave",0}, {"osc1level",1.0f}, {"osc1univoices",3}, {"osc1unidetune",0.14f}, {"osc1uniphase",1},
            {"osc2wave",1}, {"osc2pw",0.35f}, {"osc2level",0.6f},
            {"subwave",1}, {"sublevel",0.7f},
            {"filtertype",1}, {"cutoff",650.0f}, {"resonance",0.3f}, {"drive",0.9f}, {"filterenvamount",0.35f},
            {"filterdecay",0.2f}, {"filtersustain",0.25f},
            {"ampattack",0.001f}, {"ampsustain",0.9f}, {"amprelease",0.1f},
        }});

        p.push_back ({ "Sub Bomb", "Bass", V {
            {"voicemode",1}, {"glidetime",0.03f},
            {"osc1wave",3}, {"osc1level",0.7f},
            {"subwave",0}, {"suboct",1}, {"sublevel",1.0f},
            {"filtertype",0}, {"cutoff",220.0f}, {"resonance",0.1f}, {"drive",0.5f}, {"filterenvamount",0.3f},
            {"filterdecay",0.25f}, {"filtersustain",0.3f},
            {"ampattack",0.001f}, {"ampsustain",0.95f}, {"amprelease",0.2f},
        }});

        //====================================================================
        //  LEAD
        //====================================================================
        p.push_back ({ "Searing Lead", "Lead", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.8f}, {"osc1univoices",7}, {"osc1unidetune",0.3f}, {"osc1unispread",0.7f}, {"osc1uniphase",0},
            {"osc2wave",0}, {"osc2level",0.5f}, {"osc2oct",1},
            {"filtertype",0}, {"cutoff",3500.0f}, {"resonance",0.2f}, {"drive",0.4f}, {"keytrack",0.4f}, {"filterenvamount",0.3f},
            {"filterdecay",0.4f}, {"filtersustain",0.6f},
            {"ampattack",0.005f}, {"ampsustain",0.85f}, {"amprelease",0.3f},
            {"choruson",1}, {"chorusmix",0.35f}, {"delayon",1}, {"delaymix",0.25f}, {"delaydivision",8}, {"delaypingpong",1},
            {"reverbon",1}, {"reverbmix",0.2f},
        }});

        p.push_back ({ "Neon Blade", "Lead", V {
            {"voicemode",2}, {"glidetime",0.06f},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1univoices",5}, {"osc1unidetune",0.22f}, {"osc1unispread",0.6f},
            {"osc2wave",1}, {"osc2pw",0.4f}, {"osc2level",0.5f},
            {"filtertype",0}, {"cutoff",2600.0f}, {"resonance",0.3f}, {"drive",0.5f}, {"keytrack",0.5f}, {"filterenvamount",0.35f},
            {"filterdecay",0.5f}, {"filtersustain",0.5f},
            {"ampattack",0.01f}, {"ampsustain",0.9f}, {"amprelease",0.35f},
            {"lfo1shape",0}, {"lfo1rate",5.0f}, {"lfo1fade",0.4f}, {"mod3source",3}, {"mod3dest",1}, {"mod3amount",0.04f}, // vibrato
            {"delayon",1}, {"delaymix",0.28f}, {"delaypingpong",1}, {"reverbon",1}, {"reverbmix",0.22f},
        }});

        p.push_back ({ "Hero Lead", "Lead", V {
            {"voicemode",1}, {"glidetime",0.05f},
            {"osc1wave",0}, {"osc1level",0.9f}, {"osc1univoices",3}, {"osc1unidetune",0.18f},
            {"osc2wave",0}, {"osc2level",0.7f}, {"osc2semi",7},
            {"filtertype",0}, {"cutoff",4000.0f}, {"resonance",0.25f}, {"drive",0.45f}, {"keytrack",0.6f}, {"filterenvamount",0.3f},
            {"filterdecay",0.4f}, {"filtersustain",0.7f},
            {"ampattack",0.005f}, {"ampsustain",0.9f}, {"amprelease",0.3f},
            {"reverbon",1}, {"reverbmix",0.25f}, {"delayon",1}, {"delaymix",0.2f},
        }});

        p.push_back ({ "Ghost", "Lead", V {
            {"voicemode",0},
            {"osc1wave",3}, {"osc1level",0.9f},
            {"osc3wave",0}, {"osc3oct",1}, {"osc3level",0.0f},
            {"pmamount",0.55f},
            {"filtertype",0}, {"cutoff",5000.0f}, {"resonance",0.15f}, {"keytrack",0.5f}, {"filterenvamount",0.2f},
            {"ampattack",0.02f}, {"ampsustain",0.8f}, {"amprelease",0.4f},
            {"choruson",1}, {"chorusmix",0.4f}, {"reverbon",1}, {"reverbmix",0.3f}, {"reverbsize",0.7f},
        }});

        p.push_back ({ "Hunter Lead", "Lead", V {
            {"voicemode",1}, {"glidetime",0.04f},
            {"osc1wave",0}, {"osc1level",0.9f},
            {"osc2wave",0}, {"osc2level",0.8f}, {"osc2fine",8.0f},
            {"syncon",1}, {"syncdepth",5.0f},
            {"filtertype",1}, {"cutoff",2800.0f}, {"resonance",0.3f}, {"drive",0.55f}, {"keytrack",0.5f}, {"filterenvamount",0.3f},
            {"filterdecay",0.3f}, {"filtersustain",0.5f},
            {"ampattack",0.005f}, {"ampsustain",0.9f}, {"amprelease",0.25f},
            {"delayon",1}, {"delaymix",0.25f}, {"delaypingpong",1},
        }});

        p.push_back ({ "Cyber Howl", "Lead", V {
            {"voicemode",1}, {"glidetime",0.07f},
            {"osc1wave",0}, {"osc1level",0.9f}, {"osc1univoices",5}, {"osc1unidetune",0.25f},
            {"filtertype",1}, {"cutoff",1800.0f}, {"resonance",0.55f}, {"drive",0.6f}, {"keytrack",0.7f}, {"filterenvamount",0.4f},
            {"filterdecay",0.5f}, {"filtersustain",0.4f},
            {"ampattack",0.01f}, {"ampsustain",0.9f}, {"amprelease",0.3f},
            {"lfo1shape",0}, {"lfo1rate",6.0f}, {"lfo1fade",0.5f}, {"mod3source",3}, {"mod3dest",1}, {"mod3amount",0.05f},
            {"reverbon",1}, {"reverbmix",0.25f},
        }});

        //====================================================================
        //  PLUCK
        //====================================================================
        p.push_back ({ "Static Pluck", "Pluck", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.9f}, {"osc1univoices",3}, {"osc1unidetune",0.2f},
            {"filtertype",0}, {"cutoff",2500.0f}, {"resonance",0.3f}, {"drive",0.4f}, {"keytrack",0.5f}, {"filterenvamount",0.7f},
            {"filterattack",0.001f}, {"filterdecay",0.18f}, {"filtersustain",0.0f},
            {"ampattack",0.001f}, {"ampdecay",0.4f}, {"ampsustain",0.0f}, {"amprelease",0.2f},
            {"delayon",1}, {"delaymix",0.3f}, {"delaydivision",8}, {"delaypingpong",1}, {"reverbon",1}, {"reverbmix",0.2f},
        }});

        p.push_back ({ "Arc Pluck", "Pluck", V {
            {"voicemode",0},
            {"osc1wave",1}, {"osc1pw",0.4f}, {"osc1level",0.9f},
            {"osc2wave",0}, {"osc2level",0.5f}, {"osc2oct",1},
            {"subwave",0}, {"sublevel",0.4f},
            {"filtertype",0}, {"cutoff",1800.0f}, {"resonance",0.4f}, {"drive",0.45f}, {"filterenvamount",0.75f},
            {"filterattack",0.001f}, {"filterdecay",0.14f}, {"filtersustain",0.0f},
            {"ampattack",0.001f}, {"ampdecay",0.3f}, {"ampsustain",0.05f}, {"amprelease",0.15f},
            {"reverbon",1}, {"reverbmix",0.22f},
        }});

        p.push_back ({ "Bit Pluck", "Pluck", V {
            {"voicemode",0},
            {"osc1wave",1}, {"osc1pw",0.25f}, {"osc1level",0.9f},
            {"filtertype",1}, {"cutoff",3000.0f}, {"resonance",0.35f}, {"drive",0.5f}, {"filterenvamount",0.6f},
            {"filterattack",0.001f}, {"filterdecay",0.1f}, {"filtersustain",0.0f},
            {"ampattack",0.001f}, {"ampdecay",0.18f}, {"ampsustain",0.0f}, {"amprelease",0.1f},
            {"delayon",1}, {"delaymix",0.28f}, {"delaydivision",10}, {"delaypingpong",1},
        }});

        //====================================================================
        //  STAB / CHORD
        //====================================================================
        p.push_back ({ "Brut Stab", "Stab", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.85f}, {"osc1univoices",5}, {"osc1unidetune",0.25f}, {"osc1unispread",0.6f},
            {"osc2wave",0}, {"osc2level",0.7f}, {"osc2oct",-1},
            {"filtertype",0}, {"cutoff",2200.0f}, {"resonance",0.3f}, {"drive",0.55f}, {"filterenvamount",0.6f},
            {"filterattack",0.001f}, {"filterdecay",0.22f}, {"filtersustain",0.1f},
            {"ampattack",0.001f}, {"ampdecay",0.5f}, {"ampsustain",0.3f}, {"amprelease",0.25f},
            {"reverbon",1}, {"reverbmix",0.22f}, {"delayon",1}, {"delaymix",0.2f}, {"delaypingpong",1},
        }});

        p.push_back ({ "Midnight Stab", "Stab", V {
            {"voicemode",0},
            {"osc1wave",1}, {"osc1pw",0.45f}, {"osc1level",0.8f}, {"osc1univoices",3}, {"osc1unidetune",0.2f},
            {"osc2wave",0}, {"osc2level",0.6f},
            {"filtertype",0}, {"cutoff",1800.0f}, {"resonance",0.35f}, {"drive",0.4f}, {"filterenvamount",0.5f},
            {"filterattack",0.001f}, {"filterdecay",0.3f}, {"filtersustain",0.2f},
            {"ampattack",0.002f}, {"ampdecay",0.6f}, {"ampsustain",0.35f}, {"amprelease",0.3f},
            {"choruson",1}, {"chorusmix",0.3f}, {"reverbon",1}, {"reverbmix",0.28f},
        }});

        p.push_back ({ "Power Chord", "Stab", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.9f}, {"osc1univoices",4}, {"osc1unidetune",0.18f},
            {"osc2wave",0}, {"osc2level",0.8f}, {"osc2semi",7},
            {"subwave",1}, {"sublevel",0.4f},
            {"filtertype",1}, {"cutoff",2400.0f}, {"resonance",0.25f}, {"drive",0.7f}, {"filterenvamount",0.4f},
            {"filterattack",0.001f}, {"filterdecay",0.3f}, {"filtersustain",0.25f},
            {"ampattack",0.001f}, {"ampdecay",0.6f}, {"ampsustain",0.4f}, {"amprelease",0.25f},
            {"reverbon",1}, {"reverbmix",0.2f},
        }});

        //====================================================================
        //  FX / DRONE
        //====================================================================
        p.push_back ({ "Dread Drone", "FX", V {
            {"voicemode",0},
            {"osc1wave",0}, {"osc1level",0.7f}, {"osc1univoices",7}, {"osc1unidetune",0.5f}, {"osc1unispread",0.8f},
            {"osc2wave",0}, {"osc2level",0.6f}, {"osc2oct",-1}, {"osc2univoices",5}, {"osc2unidetune",0.5f},
            {"subwave",0}, {"sublevel",0.6f}, {"noiselevel",0.08f}, {"noisecolor",0.6f},
            {"filtertype",0}, {"cutoff",900.0f}, {"resonance",0.4f}, {"drive",0.5f},
            {"lfo1shape",0}, {"lfo1rate",0.12f}, {"mod3source",3}, {"mod3dest",19}, {"mod3amount",0.3f},
            {"ampattack",0.5f}, {"ampsustain",1.0f}, {"amprelease",0.8f},
            {"reverbon",1}, {"reverbmix",0.4f}, {"reverbsize",0.85f}, {"reverbdecay",0.8f},
        }});

        p.push_back ({ "Siren Riser", "FX", V {
            {"voicemode",1}, {"glidetime",0.0f},
            {"osc1wave",0}, {"osc1level",0.8f}, {"osc1univoices",3}, {"osc1unidetune",0.3f},
            {"filtertype",1}, {"cutoff",1500.0f}, {"resonance",0.6f}, {"drive",0.5f},
            {"lfo1shape",0}, {"lfo1rate",4.0f}, {"mod3source",3}, {"mod3dest",1}, {"mod3amount",0.25f},
            {"lfo2shape",2}, {"lfo2sync",1}, {"lfo2division",4}, {"mod4source",4}, {"mod4dest",19}, {"mod4amount",0.4f},
            {"ampattack",0.05f}, {"ampsustain",1.0f}, {"amprelease",0.4f},
            {"delayon",1}, {"delaymix",0.3f}, {"delaypingpong",1}, {"reverbon",1}, {"reverbmix",0.3f},
        }});

        return p;
    }
}
