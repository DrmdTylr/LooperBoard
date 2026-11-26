#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

struct PedalPreset
{
    juce::PluginDescription plugin;
    juce::String templateId;
    juce::StringArray mappings;
    juce::MemoryBlock state;
    bool bypassed = false;
};

inline bool upgradePedalMappings(juce::StringArray& mappings, int count, int legacyCount)
{
    if (mappings.size() != count && mappings.size() != legacyCount) return false;
    while (mappings.size() < count) mappings.add({});
    return true;
}

inline juce::Result writePedalPresetXml(juce::XmlElement& xml, const PedalPreset& preset)
{
    if (preset.state.getSize() > 16 * 1024 * 1024)
        return juce::Result::fail("This plugin state is too large to save as a pedal preset.");
    xml.setAttribute("template", preset.templateId);
    xml.setAttribute("bypassed", preset.bypassed);
    xml.addChildElement(preset.plugin.createXml().release());
    xml.createNewChildElement("STATE")->addTextElement(juce::Base64::toBase64(preset.state.getData(), preset.state.getSize()));
    auto* mappings = xml.createNewChildElement("MAPPINGS");
    for (const auto& mapping : preset.mappings)
        mappings->createNewChildElement("KNOB")->setAttribute("parameter", mapping);
    return juce::Result::ok();
}

inline juce::Result readPedalPresetXml(const juce::XmlElement& xml, PedalPreset& destination)
{
    if (!xml.hasAttribute("template") || !xml.hasAttribute("bypassed")
        || (xml.getStringAttribute("bypassed") != "0" && xml.getStringAttribute("bypassed") != "1"))
        return juce::Result::fail("Invalid or unsupported pedal preset.");
    const auto* plugin = xml.getChildByName("PLUGIN");
    const auto* state = xml.getChildByName("STATE");
    const auto* mappings = xml.getChildByName("MAPPINGS");
    PedalPreset preset;
    if (plugin == nullptr || state == nullptr || mappings == nullptr
        || !preset.plugin.loadFromXml(*plugin) || preset.plugin.pluginFormatName != "VST3"
        || preset.plugin.name.isEmpty() || preset.plugin.fileOrIdentifier.isEmpty() || preset.plugin.isInstrument)
        return juce::Result::fail("Invalid pedal preset plugin or state.");
    const auto encoded = state->getAllSubText();
    {
        juce::MemoryOutputStream decoded(preset.state, false);
        if (encoded.length() > 22369624 || (encoded.isNotEmpty() && !juce::Base64::convertFromBase64(decoded, encoded)))
            return juce::Result::fail("Invalid pedal preset plugin or state.");
    }
    if (preset.state.getSize() > 16 * 1024 * 1024
        || juce::Base64::toBase64(preset.state.getData(), preset.state.getSize()) != encoded)
        return juce::Result::fail("Invalid pedal preset plugin or state.");
    preset.templateId = xml.getStringAttribute("template");
    preset.bypassed = xml.getBoolAttribute("bypassed");
    for (const auto* knob : mappings->getChildIterator())
    {
        if (!knob->hasTagName("KNOB") || !knob->hasAttribute("parameter") || preset.mappings.size() >= 8)
            return juce::Result::fail("Invalid pedal knob assignments.");
        preset.mappings.add(knob->getStringAttribute("parameter"));
    }
    destination = std::move(preset);
    return juce::Result::ok();
}

inline juce::Result savePedalPreset(const juce::File& file, const PedalPreset& preset)
{
    juce::XmlElement xml("LOOPER_PEDAL");
    xml.setAttribute("version", 1);
    const auto result = writePedalPresetXml(xml, preset);
    if (result.failed()) return result;
    return file.replaceWithText(xml.toString())
        ? juce::Result::ok() : juce::Result::fail("Could not save the pedal preset.");
}

inline juce::Result loadPedalPreset(const juce::File& file, PedalPreset& destination)
{
    if (!file.existsAsFile() || file.getSize() <= 0 || file.getSize() > 24 * 1024 * 1024)
        return juce::Result::fail("Pedal preset is missing or too large.");
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr || !xml->hasTagName("LOOPER_PEDAL") || xml->getStringAttribute("version") != "1")
        return juce::Result::fail("Invalid or unsupported pedal preset.");
    return readPedalPresetXml(*xml, destination);
}

inline juce::AudioProcessorParameter* findPedalParameter(juce::AudioProcessor& plugin, const juce::String& id)
{
    if (id.isEmpty()) return nullptr;
    for (auto* parameter : plugin.getParameters())
        if (auto* hosted = dynamic_cast<juce::HostedAudioProcessorParameter*>(parameter))
            if (hosted->getParameterID() == id && parameter->isAutomatable()) return parameter;
    return nullptr;
}
