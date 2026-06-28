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
            for (auto& c : { "Bass", "Lead", "Pluck", "Stab", "FX" })
                if (hasCategory (c)) cats.add (c);
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
