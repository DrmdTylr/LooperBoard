#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

inline juce::Result saveAudioSettings(const juce::File& file, const juce::AudioDeviceManager& devices, int inputMode,
                                      const juce::XmlElement* unavailableDevice = nullptr)
{
    juce::XmlElement settings("LOOPER_AUDIO");
    settings.setAttribute("inputMode", inputMode);
    auto state = devices.createStateXml();
    if (unavailableDevice != nullptr)
    {
        if (state == nullptr) state = std::make_unique<juce::XmlElement>(*unavailableDevice);
        else
        {
            for (int i = state->getNumAttributes(); --i >= 0;)
            {
                const auto name = state->getAttributeName(i);
                if (name == "deviceType" || name.startsWith("audio")) state->removeAttribute(name);
            }
            for (int i = 0; i < unavailableDevice->getNumAttributes(); ++i)
            {
                const auto name = unavailableDevice->getAttributeName(i);
                if (name == "deviceType" || name.startsWith("audio"))
                    state->setAttribute(name, unavailableDevice->getAttributeValue(i));
            }
        }
    }
    if (state != nullptr) settings.addChildElement(state.release());
    const auto directory = file.getParentDirectory().createDirectory();
    if (directory.failed()) return directory;
    return file.replaceWithText(settings.toString())
        ? juce::Result::ok() : juce::Result::fail("Could not save audio settings.");
}

inline juce::Result readAudioSettings(const juce::File& file, std::unique_ptr<juce::XmlElement>& settings)
{
    settings.reset();
    if (!file.existsAsFile()) return juce::Result::ok();
    if (file.getSize() <= 0 || file.getSize() > 1024 * 1024)
        return juce::Result::fail("Could not read saved audio settings. Choose your audio device again.");
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr || !xml->hasTagName("LOOPER_AUDIO")
        || !xml->hasAttribute("inputMode") || xml->getStringAttribute("inputMode").length() != 1
        || !juce::String("012").contains(xml->getStringAttribute("inputMode")))
        return juce::Result::fail("Could not read saved audio settings. Choose your audio device again.");
    for (const auto* child : xml->getChildIterator())
        if (!child->hasTagName("DEVICESETUP"))
            return juce::Result::fail("Could not read saved audio settings. Choose your audio device again.");
    settings = std::move(xml);
    return juce::Result::ok();
}

inline bool setAudioChannel(juce::BigInteger& channels, int index, int channelCount, bool enabled)
{
    if (!juce::isPositiveAndBelow(index, channelCount))
        return false;
    if (channels[index] == enabled)
        return true;
    const auto count = channels.countNumberOfSetBits();
    if (!enabled && count <= 1)
        return false;
    if (enabled && count >= 2)
    {
        const auto first = channels.findNextSetBit(0);
        channels.clearBit(index > first ? first : channels.getHighestBit());
    }
    channels.setBit(index, enabled);
    return true;
}
