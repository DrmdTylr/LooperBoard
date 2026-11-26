#pragma once

#include "LooperEngine.h"
#include <cstdint>
#include <cstring>
#include <istream>
#include <ostream>
#include <string>

namespace LoopFile
{
// Version 1 stores little-endian IEEE audio and scalars (the Windows app's native format).
template <typename T> bool write(std::ostream& stream, const T& value)
{
    return static_cast<bool>(stream.write(reinterpret_cast<const char*>(&value), sizeof(value)));
}

template <typename T> bool read(std::istream& stream, T& value)
{
    return static_cast<bool>(stream.read(reinterpret_cast<char*>(&value), sizeof(value)));
}

inline bool save(std::ostream& stream, const LooperEngine::Project& project)
{
    if (!LooperEngine::validProject(project)) return false;
    stream.write("LOOPv001", 8);
    write(stream, project.sampleRate);
    write(stream, project.tempo);
    for (int value : { project.signature.beats, project.signature.unit, static_cast<int>(project.layered),
                      static_cast<int>(project.click), project.countIn, project.autoStop,
                      static_cast<int>(project.layers.size()),
                      static_cast<int>(project.layers.front().audio[0].size()) })
        write(stream, static_cast<std::uint32_t>(value));
    for (float gain : project.gains) write(stream, gain);
    for (const auto& layer : project.layers)
    {
        write(stream, layer.volume);
        write(stream, layer.pan);
        write(stream, static_cast<std::uint32_t>(layer.active));
        write(stream, static_cast<std::uint32_t>(layer.solo));
        for (const auto& channel : layer.audio)
            stream.write(reinterpret_cast<const char*>(channel.data()),
                         static_cast<std::streamsize>(channel.size() * sizeof(float)));
    }
    return static_cast<bool>(stream);
}

inline bool load(std::istream& stream, LooperEngine::Project& destination, std::string& error)
{
    error = "Invalid, unsupported, or truncated loop project.";
    LooperEngine::Project project;
    char magic[8]{};
    stream.read(magic, 8);
    if (std::memcmp(magic, "LOOPv001", 8) != 0
        || !read(stream, project.sampleRate) || !read(stream, project.tempo))
        return false;
    std::array<std::uint32_t, 8> fields{};
    for (auto& field : fields) if (!read(stream, field)) return false;
    if (!std::isfinite(project.sampleRate) || project.sampleRate <= 0.0 || project.sampleRate > 384000.0
        || fields[0] < 1 || fields[0] > 16
        || (fields[1] != 1 && fields[1] != 2 && fields[1] != 4 && fields[1] != 8 && fields[1] != 16)
        || fields[2] > 1 || fields[3] > 1 || fields[4] > 4 || fields[5] > 16
        || fields[6] < 1 || fields[6] > LooperEngine::maxLayers || fields[7] == 0
        || fields[7] > LooperEngine::capacityForRate(project.sampleRate))
        return false;
    project.signature = { static_cast<int>(fields[0]), static_cast<int>(fields[1]) };
    project.layered = fields[2] != 0;
    project.click = fields[3] != 0;
    project.countIn = static_cast<int>(fields[4]);
    project.autoStop = static_cast<int>(fields[5]);
    for (auto& gain : project.gains) if (!read(stream, gain)) return false;
    project.layers.resize(fields[6]);
    for (auto& layer : project.layers)
    {
        std::uint32_t active = 0, solo = 0;
        if (!read(stream, layer.volume) || !read(stream, layer.pan)
            || !read(stream, active) || !read(stream, solo) || active > 1 || solo > 1)
            return false;
        layer.active = active != 0;
        layer.solo = solo != 0;
        for (auto& channel : layer.audio)
        {
            channel.resize(fields[7]);
            if (!stream.read(reinterpret_cast<char*>(channel.data()),
                             static_cast<std::streamsize>(channel.size() * sizeof(float))))
                return false;
        }
    }
    if (!LooperEngine::validProject(project) || stream.peek() != std::char_traits<char>::eof())
        return false;
    destination = std::move(project);
    error.clear();
    return true;
}

inline std::array<std::vector<float>, 2> mix(const LooperEngine::Project& project)
{
    const auto frames = project.layers.front().audio[0].size();
    std::array<std::vector<float>, 2> result { std::vector<float>(frames), std::vector<float>(frames) };
    const bool solo = std::any_of(project.layers.begin(), project.layers.end(),
        [](const auto& layer) { return layer.active && layer.solo; });
    for (const auto& layer : project.layers)
    {
        if (!layer.active || (solo && !layer.solo)) continue;
        const float gain = layer.volume * project.gains[2] * project.gains[3];
        const std::array<float, 2> gains { gain * (layer.pan > 0 ? 1 - layer.pan : 1),
                                         gain * (layer.pan < 0 ? 1 + layer.pan : 1) };
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (std::size_t i = 0; i < frames; ++i)
                result[ch][i] += layer.audio[ch][i] * gains[ch];
    }
    return result;
}
}
