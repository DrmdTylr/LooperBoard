#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include "LoopFile.h"
#include <filesystem>
#include <fstream>

namespace LoopAudioFile
{
inline juce::Result saveProject(const juce::File& file, const LooperEngine::Project& project)
{
    juce::TemporaryFile temporary(file);
    {
        std::ofstream stream(std::filesystem::path(temporary.getFile().getFullPathName().toWideCharPointer()),
                             std::ios::binary | std::ios::trunc);
        if (!LoopFile::save(stream, project))
            return juce::Result::fail("Could not write the loop project. The existing file was not replaced.");
        stream.flush();
        if (!stream) return juce::Result::fail("Could not flush the loop project to disk.");
        stream.close();
        if (stream.fail()) return juce::Result::fail("Could not finish writing the loop project.");
    }
    return temporary.overwriteTargetFileWithTemporary()
        ? juce::Result::ok() : juce::Result::fail("Could not replace the destination loop project.");
}

inline juce::Result loadProject(const juce::File& file, LooperEngine::Project& project)
{
    std::ifstream stream(std::filesystem::path(file.getFullPathName().toWideCharPointer()), std::ios::binary);
    if (!stream) return juce::Result::fail("Could not open the loop project.");
    std::string error;
    return LoopFile::load(stream, project, error) ? juce::Result::ok() : juce::Result::fail(error);
}

inline juce::Result resample(LooperEngine::Project& project, double rate)
{
    if (!LooperEngine::validProject(project) || !std::isfinite(rate) || rate <= 0.0 || rate > 384000.0)
        return juce::Result::fail("Invalid loop audio or device sample rate.");
    if (project.sampleRate == rate) return juce::Result::ok();
    const auto sourceFrames = project.layers.front().audio[0].size();
    const auto targetFrames = std::round(static_cast<double>(sourceFrames) * rate / project.sampleRate);
    if (targetFrames < 1 || targetFrames > static_cast<double>(LooperEngine::capacityForRate(rate)))
        return juce::Result::fail("This loop exceeds the recording limit at the current device sample rate.");
    for (auto& layer : project.layers)
    {
        float* channels[] { layer.audio[0].data(), layer.audio[1].data() };
        juce::AudioBuffer<float> input(channels, 2, static_cast<int>(sourceFrames));
        juce::MemoryAudioSource source(input, false, false);
        juce::ResamplingAudioSource converter(&source, false, 2);
        converter.setResamplingRatio(project.sampleRate / rate);
        converter.prepareToPlay(4096, rate);
        juce::AudioBuffer<float> output(2, static_cast<int>(targetFrames));
        for (int start = 0; start < output.getNumSamples(); start += 4096)
            converter.getNextAudioBlock({ &output, start, std::min(4096, output.getNumSamples() - start) });
        converter.releaseResources();
        for (std::size_t ch = 0; ch < 2; ++ch)
            layer.audio[ch].assign(output.getReadPointer(static_cast<int>(ch)),
                                   output.getReadPointer(static_cast<int>(ch)) + output.getNumSamples());
    }
    project.sampleRate = rate;
    return juce::Result::ok();
}

inline juce::Result importWav(const juce::File& file, double rate, LooperEngine::Project& destination)
{
    // JUCE pads truncated WAV data with silence rather than reporting a read error.
    struct CheckedWavStream final : juce::FileInputStream
    {
        explicit CheckedWavStream(const juce::File& source) : FileInputStream(source) {}
        int read(void* data, int bytes) override
        {
            const auto actual = FileInputStream::read(data, bytes);
            incomplete = incomplete || actual != bytes;
            return actual;
        }
        bool incomplete = false;
    };
    auto stream = std::make_unique<CheckedWavStream>(file);
    if (stream->failedToOpen()) return juce::Result::fail("Could not open this WAV file.");
    auto* checked = stream.get();
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(stream.release(), true));
    if (reader == nullptr) return juce::Result::fail("Could not read this WAV file.");
    if (!std::isfinite(reader->sampleRate) || reader->sampleRate <= 0 || reader->sampleRate > 384000
        || reader->numChannels < 1 || reader->numChannels > 2 || reader->lengthInSamples <= 0
        || reader->lengthInSamples > static_cast<juce::int64>(LooperEngine::capacityForRate(reader->sampleRate)))
        return juce::Result::fail("Import requires a mono or stereo WAV within the looper's duration and memory limits.");
    const auto frames = static_cast<int>(reader->lengthInSamples);
    juce::AudioBuffer<float> audio(2, frames);
    checked->incomplete = false;
    if (!reader->read(&audio, 0, frames, 0, true, true) || checked->incomplete)
        return juce::Result::fail("The WAV audio could not be read completely.");
    LooperEngine::Project project;
    project.sampleRate = reader->sampleRate;
    project.layers.resize(1);
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto* samples = audio.getReadPointer(reader->numChannels == 1 ? 0 : ch);
        project.layers[0].audio[static_cast<std::size_t>(ch)].assign(samples, samples + frames);
    }
    const auto result = resample(project, rate);
    if (result.failed()) return result;
    destination = std::move(project);
    return juce::Result::ok();
}

inline juce::Result exportWav(const juce::File& file, const LooperEngine::Project& project)
{
    if (!LooperEngine::validProject(project)) return juce::Result::fail("There is no valid loop to export.");
    const auto audio = LoopFile::mix(project);
    juce::TemporaryFile temporary(file);
    auto stream = temporary.getFile().createOutputStream();
    if (stream == nullptr) return juce::Result::fail("Could not create the WAV file.");
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.get(), project.sampleRate, 2, 32, {}, 0));
    if (writer == nullptr) return juce::Result::fail("Could not create the WAV writer.");
    auto* output = stream.release(); // The writer owns the stream.
    const float* channels[] { audio[0].data(), audio[1].data() };
    if (!writer->writeFromFloatArrays(channels, 2, static_cast<int>(audio[0].size())) || !writer->flush())
        return juce::Result::fail("Could not write the complete WAV audio.");
    output->flush();
    const auto status = output->getStatus();
    writer.reset();
    if (status.failed()) return status;
    return temporary.overwriteTargetFileWithTemporary()
        ? juce::Result::ok() : juce::Result::fail("Could not replace the destination WAV file.");
}
}
