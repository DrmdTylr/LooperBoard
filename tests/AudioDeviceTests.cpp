#include <juce_audio_devices/juce_audio_devices.h>
#include "../src/AudioSettings.h"
#include <iostream>

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiser;
    juce::BigInteger channels;
    channels.setBit(0);
    if (setAudioChannel(channels, 0, 4, false) || !channels[0]
        || !setAudioChannel(channels, 1, 4, true) || channels.countNumberOfSetBits() != 2
        || !setAudioChannel(channels, 3, 4, true) || channels[0] || !channels[1] || !channels[3]
        || setAudioChannel(channels, 4, 4, true))
    {
        std::cerr << "Audio channel selection failed\n";
        return 1;
    }
    juce::AudioDeviceManager manager;
    juce::TemporaryFile settingsFile(".xml");
    std::unique_ptr<juce::XmlElement> saved;
    if (readAudioSettings(settingsFile.getFile(), saved).failed() || saved != nullptr
        || saveAudioSettings(settingsFile.getFile(), manager, 2).failed()
        || readAudioSettings(settingsFile.getFile(), saved).failed() || saved == nullptr
        || saved->getIntAttribute("inputMode") != 2)
    {
        std::cerr << "Audio settings round trip failed\n";
        return 1;
    }
    juce::XmlElement unavailable("DEVICESETUP");
    unavailable.setAttribute("deviceType", "ASIO");
    unavailable.setAttribute("audioOutputDeviceName", "Disconnected interface");
    unavailable.setAttribute("audioDeviceRate", 96000);
    if (saveAudioSettings(settingsFile.getFile(), manager, 1, &unavailable).failed()
        || readAudioSettings(settingsFile.getFile(), saved).failed()
        || saved->getChildByName("DEVICESETUP") == nullptr
        || saved->getChildByName("DEVICESETUP")->getStringAttribute("audioOutputDeviceName") != "Disconnected interface"
        || saved->getChildByName("DEVICESETUP")->getIntAttribute("audioDeviceRate") != 96000)
    {
        std::cerr << "Unavailable audio device preference was lost\n";
        return 1;
    }
    if (!settingsFile.getFile().replaceWithText("<LOOPER_AUDIO inputMode=\"99\"/>")
        || readAudioSettings(settingsFile.getFile(), saved).wasOk() || saved != nullptr)
    {
        std::cerr << "Invalid audio settings were accepted\n";
        return 1;
    }
    bool foundAsio = false;
    for (auto* type : manager.getAvailableDeviceTypes())
    {
        std::cout << "Backend: " << type->getTypeName() << '\n';
        if (type->getTypeName() != "ASIO")
            continue;

        foundAsio = true;
        type->scanForDevices();
        for (const auto& name : type->getDeviceNames())
            std::cout << "ASIO driver: " << name << '\n';
    }
    if (!foundAsio)
    {
        std::cerr << "ASIO backend missing from this Windows build\n";
        return 1;
    }
    return 0;
}
