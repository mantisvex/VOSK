#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Presets.h"

//==============================================================================
//  VOSK preset manager — factory presets (baked in) + user presets (.voskpreset
//  XML in the user documents folder), browsable by category. ChangeBroadcaster
//  so the GUI can follow external loads (prev/next, save).
//==============================================================================
namespace vosk
{
    class PresetManager : public juce::ChangeBroadcaster
    {
    public:
        explicit PresetManager (juce::AudioProcessorValueTreeState& s) : apvts (s)
        {
            userDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                          .getChildFile ("MantisVex").getChildFile ("VOSK").getChildFile ("Presets");
            userDir.createDirectory();
            rebuild();
        }

        //-- Browsing ---------------------------------------------------------
        juce::StringArray getCategories() const
        {
            juce::StringArray cats { "All" };
            // Preferred ordering; only include those actually present.
            for (auto& c : { "Bass", "Lead", "Pluck", "Keys", "Stab", "Pad", "Arp", "FX" })
                if (hasCategory (c)) cats.add (c);
            // Any other categories not in the preferred list (e.g. bank tags).
            for (auto& e : entries)
                if (! cats.contains (e.category) && e.category != "User")
                    cats.add (e.category);
            if (hasCategory ("User")) cats.add ("User");
            return cats;
        }

        juce::StringArray namesInCategory (const juce::String& cat) const
        {
            juce::StringArray names;
            for (auto& e : entries)
                if (cat == "All" || e.category == cat)
                    names.add (e.name);
            return names;
        }

        juce::String getCurrentName() const     { return currentName; }
        juce::String getCurrentCategory() const { return currentCategory; }

        //-- Loading ----------------------------------------------------------
        void load (const juce::String& cat, const juce::String& name)
        {
            for (auto& e : entries)
                if (e.name == name && (cat == "All" || e.category == cat))
                {
                    applyEntry (e);
                    currentName = e.name;
                    currentCategory = e.category;
                    sendChangeMessage();
                    return;
                }
        }

        // Step through a category's list (wrapping) and load the result.
        void step (int dir, const juce::String& cat)
        {
            auto names = namesInCategory (cat);
            if (names.isEmpty()) return;
            int idx = names.indexOf (currentName);
            if (idx < 0) idx = (dir > 0 ? -1 : 0);
            idx = (idx + dir + names.size()) % names.size();
            load (cat, names[idx]);
        }

        void loadInit()
        {
            resetToDefaults();
            currentName = "Init";
            currentCategory = "All";
            sendChangeMessage();
        }

        // Genre-constrained randomiser ("MUTATE"): always lands on usable
        // darksynth territory (saws/pulses, sub weight, low-mid resonant filter,
        // drive + character on), never chaos. Output safety covers any extremes.
        void randomise()
        {
            resetToDefaults();
            juce::Random r;
            auto set = [&] (const char* id, float v) { if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
            auto rf  = [&] (float lo, float hi) { return lo + r.nextFloat() * (hi - lo); };
            auto ri  = [&] (int lo, int hi) { return (float) (lo + r.nextInt (hi - lo + 1)); };
            const bool bass = r.nextFloat() < 0.6f;

            set ("voicemode", bass ? ri (1, 2) : 0.0f);
            if (bass) set ("glidetime", rf (0.0f, 0.06f));

            set ("osc1wave", r.nextFloat() < 0.7f ? 0.0f : 1.0f);
            set ("osc1level", rf (0.8f, 1.0f));
            set ("osc1univoices", ri (1, 5));
            set ("osc1unidetune", rf (0.05f, 0.32f));
            set ("osc1unispread", rf (0.0f, 0.6f));
            set ("osc1uniphase", bass ? 1.0f : 0.0f);

            if (r.nextFloat() < 0.7f)
            {
                set ("osc2wave", (float) r.nextInt (2));
                set ("osc2level", rf (0.3f, 0.8f));
                set ("osc2semi", r.nextInt (2) ? 7.0f : 0.0f);
                set ("osc2fine", rf (-9.0f, 9.0f));
            }

            set ("subwave", (float) r.nextInt (2));
            set ("suboct", (float) r.nextInt (2));
            set ("sublevel", bass ? rf (0.5f, 0.95f) : rf (0.0f, 0.4f));

            if (r.nextFloat() < 0.4f) { set ("syncon", 1.0f); set ("syncdepth", rf (0.0f, 8.0f)); }

            set ("filtertype", (float) r.nextInt (2));
            set ("cutoff", bass ? rf (250.0f, 900.0f) : rf (1200.0f, 5000.0f));
            set ("resonance", rf (0.1f, 0.4f));
            set ("drive", rf (0.3f, 0.65f));
            set ("keytrack", bass ? 0.0f : rf (0.3f, 0.6f));
            set ("filterenvamount", rf (0.25f, 0.65f));
            set ("filterattack", 0.001f);
            set ("filterdecay", rf (0.12f, 0.4f));
            set ("filtersustain", rf (0.1f, 0.4f));

            set ("ampattack", bass ? 0.001f : rf (0.001f, 0.02f));
            set ("ampsustain", rf (0.8f, 0.95f));
            set ("amprelease", rf (0.1f, 0.3f));

            set ("charon", 1.0f);
            set ("charmode", (float) r.nextInt (4));
            set ("chardrive", rf (0.2f, 0.55f));
            set ("chartone", rf (0.5f, 0.85f));

            if (r.nextFloat() < 0.5f) { set ("choruson", 1.0f); set ("chorusmix", rf (0.2f, 0.35f)); }
            if (r.nextFloat() < 0.5f) { set ("delayon", 1.0f); set ("delaymix", rf (0.15f, 0.3f)); set ("delaypingpong", (float) r.nextInt (2)); set ("delaydivision", r.nextInt (2) ? 8.0f : 9.0f); }
            if (r.nextFloat() < 0.6f) { set ("reverbon", 1.0f); set ("reverbmix", rf (0.12f, 0.28f)); set ("reverbsize", rf (0.4f, 0.7f)); }
            if (r.nextFloat() < 0.5f) { set ("tapeon", 1.0f); set ("tapewow", rf (0.1f, 0.5f)); set ("tapesat", rf (0.2f, 0.5f)); set ("tapehiss", rf (0.0f, 0.3f)); set ("tapetone", rf (0.45f, 0.8f)); }

            set ("mastervol", 0.55f);

            currentName = "Mutation";
            currentCategory = "All";
            sendChangeMessage();
        }

        //-- Saving (user) ----------------------------------------------------
        void saveUser (const juce::String& name)
        {
            if (name.isEmpty()) return;
            auto file = userDir.getChildFile (juce::File::createLegalFileName (name) + ".voskpreset");

            if (auto xml = apvts.copyState().createXml())
            {
                xml->setAttribute ("voskName", name);
                xml->setAttribute ("voskCategory", "User");
                xml->writeTo (file);
            }
            rebuild();
            currentName = name;
            currentCategory = "User";
            sendChangeMessage();
        }

        juce::File getUserDir() const { return userDir; }

        //-- Import / export --------------------------------------------------
        // Write the current state to a .voskpreset file anywhere.
        bool exportPreset (const juce::File& dest, const juce::String& name) const
        {
            if (auto xml = apvts.copyState().createXml())
            {
                xml->setAttribute ("voskName", name.isNotEmpty() ? name : dest.getFileNameWithoutExtension());
                xml->setAttribute ("voskCategory", "User");
                return xml->writeTo (dest);
            }
            return false;
        }

        // Copy an external .voskpreset into the user library and load it.
        void importPreset (const juce::File& src)
        {
            if (! src.existsAsFile()) return;
            auto dest = userDir.getChildFile (src.getFileName());
            if (dest != src) src.copyFileTo (dest);
            rebuild();
            juce::String nm = dest.getFileNameWithoutExtension();
            if (auto xml = juce::XmlDocument::parse (dest)) nm = xml->getStringAttribute ("voskName", nm);
            currentName = nm; currentCategory = "User";
            sendChangeMessage();
        }

        // Bundle every user preset into a single shareable .voskbank file.
        bool exportBank (const juce::File& dest) const
        {
            juce::XmlElement bank ("VoskBank");
            for (auto& e : entries)
            {
                if (e.isFactory) continue;
                if (auto x = juce::XmlDocument::parse (e.file))
                    bank.addChildElement (x.release());
            }
            return bank.getNumChildElements() > 0 && bank.writeTo (dest);
        }

        // Unpack a .voskbank into the user library.
        void importBank (const juce::File& src)
        {
            auto bank = juce::XmlDocument::parse (src);
            if (bank == nullptr || ! bank->hasTagName ("VoskBank")) return;
            for (auto* child : bank->getChildIterator())
            {
                const juce::String nm = child->getStringAttribute ("voskName", "Imported");
                auto dest = userDir.getChildFile (juce::File::createLegalFileName (nm) + ".voskpreset");
                child->writeTo (dest);
            }
            rebuild();
            currentCategory = "User";
            sendChangeMessage();
        }

    private:
        struct Entry
        {
            juce::String name, category;
            bool isFactory = true;
            Preset preset;     // factory only
            juce::File file;   // user only
        };

        bool hasCategory (const juce::String& c) const
        {
            for (auto& e : entries) if (e.category == c) return true;
            return false;
        }

        void rebuild()
        {
            entries.clear();
            for (auto& fp : getFactoryPresets())
                entries.push_back ({ fp.name, fp.category, true, fp, {} });

            for (auto& f : userDir.findChildFiles (juce::File::findFiles, false, "*.voskpreset"))
            {
                juce::String nm = f.getFileNameWithoutExtension();
                if (auto xml = juce::XmlDocument::parse (f))
                    nm = xml->getStringAttribute ("voskName", nm);
                entries.push_back ({ nm, "User", false, {}, f });
            }
        }

        void applyEntry (const Entry& e)
        {
            if (e.isFactory)
            {
                resetToDefaults();
                for (auto& kv : e.preset.values)
                    if (auto* p = apvts.getParameter (kv.id))
                        p->setValueNotifyingHost (p->convertTo0to1 (kv.value));
            }
            else if (auto xml = juce::XmlDocument::parse (e.file))
            {
                apvts.replaceState (juce::ValueTree::fromXml (*xml));
            }
        }

        void resetToDefaults()
        {
            for (auto* p : apvts.processor.getParameters())
                p->setValueNotifyingHost (p->getDefaultValue());
        }

        juce::AudioProcessorValueTreeState& apvts;
        juce::File userDir;
        std::vector<Entry> entries;
        juce::String currentName, currentCategory { "All" };
    };
}
