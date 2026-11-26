#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "BoardUtilities.h"
#include "LooperEngine.h"

class PedalHost
{
public:
    static constexpr int maxEffects = 16;
    struct Effect
    {
        ~Effect()
        {
            editor.reset();
            if (prepared) instance->releaseResources();
        }
        std::unique_ptr<juce::AudioPluginInstance> instance;
        std::atomic<bool> bypassed { false };
        bool prepared = false;
        int channels = 2;
        std::unique_ptr<juce::DocumentWindow> editor;
    };

    PedalHost()
    {
        formats.addFormat(new juce::VST3PluginFormat());
    }

    juce::StringArray discover(const juce::FileSearchPath& remembered = {})
    {
        auto* format = formats.getFormat(0);
        auto locations = format->getDefaultLocationsToSearch();
        locations.addPath(remembered);
        locations.removeRedundantPaths();
        auto paths = format->searchPathsForPlugins(locations, true, false);
        paths.sort(true);
        return paths;
    }

    static bool isPlugin(const juce::File& file) { return file.hasFileExtension("vst3") && file.exists(); }

    std::unique_ptr<Effect> load(const juce::PluginDescription& description, juce::String& error)
    {
        auto effect = std::make_unique<Effect>();
        effect->instance = formats.createPluginInstance(description, rate.load(), blockSize.load(), error);
        if (effect->instance == nullptr) return {};
        auto& plugin = *effect->instance;
        auto layout = plugin.getBusesLayout();
        for (int i = 1; i < layout.inputBuses.size(); ++i) layout.inputBuses.set(i, juce::AudioChannelSet::disabled());
        for (int i = 1; i < layout.outputBuses.size(); ++i) layout.outputBuses.set(i, juce::AudioChannelSet::disabled());
        if (!description.isInstrument && layout.inputBuses.size() > 0 && layout.outputBuses.size() > 0)
        {
            for (const int channels : { 2, 1 })
            {
                const auto set = channels == 2 ? juce::AudioChannelSet::stereo() : juce::AudioChannelSet::mono();
                layout.inputBuses.set(0, set);
                layout.outputBuses.set(0, set);
                if (!plugin.setBusesLayout(layout)) continue;
                effect->channels = channels;
                return effect;
            }
        }
        error = "This host requires a mono-in/mono-out or stereo-in/stereo-out audio effect.";
        return {};
    }

    juce::OwnedArray<juce::PluginDescription> describe(const juce::File& file)
    {
        juce::OwnedArray<juce::PluginDescription> descriptions;
        if (isPlugin(file))
            formats.getFormat(0)->findAllTypesForFile(descriptions, file.getFullPathName());
        return descriptions;
    }

    void setOrder(const std::vector<Effect*>& order, int split)
    {
        const juce::ScopedLock lifeGuard(lifecycle);
        if (running)
            for (auto* effect : order)
                if (!effect->prepared) prepareEffect(*effect);
        const juce::SpinLock::ScopedLockType guard(lock);
        count = static_cast<int>(order.size());
        beforeCount = split;
        std::copy(order.begin(), order.end(), effects.begin());
    }

    void prepare(double newRate, int maxBlock)
    {
        scratch.setSize(3, std::max(1, maxBlock));
        midi.ensureSize(256);
        utilities.prepare(newRate);
        const juce::ScopedLock lifeGuard(lifecycle);
        rate = newRate;
        blockSize = scratch.getNumSamples();
        running = true;
        for (int i = 0; i < count; ++i) prepareEffect(*effects[static_cast<std::size_t>(i)]);
    }

    void stop()
    {
        const juce::ScopedLock lifeGuard(lifecycle);
        running = false;
        for (int i = 0; i < count; ++i)
        {
            auto& effect = *effects[static_cast<std::size_t>(i)];
            if (effect.prepared) effect.instance->releaseResources();
            effect.prepared = false;
        }
    }

    void process(LooperEngine& engine, const float* const* inputs, int inputChannels,
                 float* const* outputs, int outputChannels, int frames)
    {
        const juce::ScopedNoDenormals noDenormals;
        float* stereo[] { scratch.getWritePointer(0), scratch.getWritePointer(1) };
        auto* click = scratch.getWritePointer(2);
        const float* engineInputs[] { stereo[0], stereo[1] };
        const auto mode = engine.inputMode.load();
        const juce::SpinLock::ScopedTryLockType guard(lock);
        for (int start = 0; start < frames;)
        {
            const auto samples = std::min(frames - start, scratch.getNumSamples());
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto source = LooperEngine::sourceChannel(mode, inputChannels, ch);
                if (source < inputChannels && inputs[source] != nullptr)
                    juce::FloatVectorOperations::copy(stereo[ch], inputs[source] + start, samples);
                else
                    juce::FloatVectorOperations::clear(stereo[ch], samples);
            }
            utilities.before(stereo[0], stereo[1], samples);
            if (guard.isLocked()) processEffects(stereo, samples, 0, beforeCount);
            engine.process(engineInputs, 2, stereo, 2, samples, true, click);
            if (guard.isLocked()) processEffects(stereo, samples, beforeCount, count);
            utilities.after(stereo[0], stereo[1], samples, click);
            for (int ch = 0; ch < outputChannels; ++ch)
            {
                if (outputs[ch] == nullptr) continue;
                if (outputChannels == 1)
                    for (int i = 0; i < samples; ++i) outputs[0][start + i] = (stereo[0][i] + stereo[1][i]) * .5f;
                else if (ch < 2)
                    juce::FloatVectorOperations::copy(outputs[ch] + start, stereo[ch], samples);
                else
                    juce::FloatVectorOperations::clear(outputs[ch] + start, samples);
            }
            start += samples;
        }
    }

    BoardUtilities utilities;

private:
    void prepareEffect(Effect& effect)
    {
        if (effect.prepared) effect.instance->releaseResources();
        effect.instance->setRateAndBufferSizeDetails(rate.load(), blockSize.load());
        effect.instance->prepareToPlay(rate.load(), blockSize.load());
        effect.prepared = true;
    }

    void processEffects(float** stereo, int samples, int begin, int end)
    {
        for (int index = begin; index < end; ++index)
        {
            auto& effect = *effects[static_cast<std::size_t>(index)];
            if (effect.bypassed.load(std::memory_order_relaxed)) continue;
            if (effect.channels == 1)
                for (int i = 0; i < samples; ++i) stereo[0][i] = (stereo[0][i] + stereo[1][i]) * .5f;
            juce::AudioBuffer<float> block(stereo, effect.channels, samples);
            midi.clear();
            effect.instance->processBlock(block, midi);
            if (effect.channels == 1) juce::FloatVectorOperations::copy(stereo[1], stereo[0], samples);
        }
    }

    juce::AudioPluginFormatManager formats;
    juce::AudioBuffer<float> scratch;
    juce::MidiBuffer midi;
    juce::CriticalSection lifecycle;
    juce::SpinLock lock;
    std::array<Effect*, maxEffects> effects{};
    int count = 0, beforeCount = 0;
    bool running = false;
    std::atomic<double> rate { 48000.0 };
    std::atomic<int> blockSize { 512 };
};
