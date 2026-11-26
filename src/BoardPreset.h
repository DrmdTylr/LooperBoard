#pragma once

#include "PedalPreset.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <vector>

struct BoardPreset
{
    struct Pedal
    {
        juce::String templateId;
        bool loaded = false;
        PedalPreset plugin;
    };
    std::vector<Pedal> pedals;
    int deckIndex = 0;
    std::array<double, 4> levels { 1, 1, 1, .7 };
    std::array<double, 5> footer { 0, -50, 0, 15, 0 };
    bool gate = false, doubler = false, tuner = true, layered = true, metronome = true;
    int bpm = 120, beats = 4, unit = 4, countIn = 0, autoStop = 0;
};

inline juce::Result saveBoardPreset(const juce::File& file, const BoardPreset& board)
{
    juce::XmlElement xml("LOOPER_BOARD");
    xml.setAttribute("version", 1);
    xml.setAttribute("deck", board.deckIndex);
    const char* levelNames[] { "input", "live", "loop", "output" };
    const char* footerNames[] { "trim", "threshold", "transpose", "delay", "master" };
    for (std::size_t i = 0; i < board.levels.size(); ++i) xml.setAttribute(levelNames[i], board.levels[i]);
    for (std::size_t i = 0; i < board.footer.size(); ++i) xml.setAttribute(footerNames[i], board.footer[i]);
    xml.setAttribute("gate", board.gate);
    xml.setAttribute("doubler", board.doubler);
    xml.setAttribute("tuner", board.tuner);
    xml.setAttribute("layered", board.layered);
    xml.setAttribute("metronome", board.metronome);
    xml.setAttribute("bpm", board.bpm);
    xml.setAttribute("beats", board.beats);
    xml.setAttribute("unit", board.unit);
    xml.setAttribute("countIn", board.countIn);
    xml.setAttribute("autoStop", board.autoStop);
    for (const auto& pedal : board.pedals)
    {
        auto* child = xml.createNewChildElement("PEDAL");
        child->setAttribute("template", pedal.templateId);
        child->setAttribute("loaded", pedal.loaded);
        if (pedal.loaded)
        {
            const auto result = writePedalPresetXml(*child, pedal.plugin);
            if (result.failed()) return result;
        }
    }
    return file.replaceWithText(xml.toString())
        ? juce::Result::ok() : juce::Result::fail("Could not save the pedalboard preset.");
}

inline juce::Result loadBoardPreset(const juce::File& file, BoardPreset& destination)
{
    if (!file.existsAsFile() || file.getSize() <= 0 || file.getSize() > 384 * 1024 * 1024)
        return juce::Result::fail("Pedalboard preset is missing or too large.");
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr || !xml->hasTagName("LOOPER_BOARD") || xml->getStringAttribute("version") != "1")
        return juce::Result::fail("Invalid or unsupported pedalboard preset.");

    BoardPreset board;
    auto number = [&xml](const char* name, double low, double high, double& value)
    {
        if (!xml->hasAttribute(name)) return false;
        const auto text = xml->getStringAttribute(name);
        const auto* begin = text.toRawUTF8();
        char* end = nullptr;
        const auto parsed = std::strtod(begin, &end);
        if (begin == end || *end != '\0' || !std::isfinite(parsed) || parsed < low || parsed > high) return false;
        value = parsed;
        return true;
    };
    auto integer = [&number](const char* name, int low, int high, int& value)
    {
        double parsed = 0;
        if (!number(name, low, high, parsed) || std::floor(parsed) != parsed) return false;
        value = static_cast<int>(parsed);
        return true;
    };
    auto flag = [&integer](const char* name, bool& value)
    {
        int parsed = 0;
        if (!integer(name, 0, 1, parsed)) return false;
        value = parsed != 0;
        return true;
    };
    const char* levelNames[] { "input", "live", "loop", "output" };
    const char* footerNames[] { "trim", "threshold", "transpose", "delay", "master" };
    const double lows[] { -24, -80, -12, 0, -24 }, highs[] { 24, 0, 12, 50, 24 };
    for (std::size_t i = 0; i < board.levels.size(); ++i)
        if (!number(levelNames[i], 0, 1, board.levels[i])) return juce::Result::fail("Invalid deck level in preset.");
    for (std::size_t i = 0; i < board.footer.size(); ++i)
        if (!number(footerNames[i], lows[i], highs[i], board.footer[i])) return juce::Result::fail("Invalid board control in preset.");
    if (!integer("deck", 0, 16, board.deckIndex) || !integer("bpm", 30, 300, board.bpm)
        || !integer("beats", 1, 16, board.beats) || !integer("unit", 1, 16, board.unit)
        || !integer("countIn", 0, 4, board.countIn) || !integer("autoStop", 0, 16, board.autoStop)
        || !flag("gate", board.gate) || !flag("doubler", board.doubler) || !flag("tuner", board.tuner)
        || !flag("layered", board.layered) || !flag("metronome", board.metronome)
        || (board.unit != 1 && board.unit != 2 && board.unit != 4 && board.unit != 8 && board.unit != 16)
        || (board.countIn != 0 && board.countIn != 1 && board.countIn != 2 && board.countIn != 4)
        || (board.autoStop != 0 && board.autoStop != 1 && board.autoStop != 2 && board.autoStop != 4
            && board.autoStop != 8 && board.autoStop != 16))
        return juce::Result::fail("Invalid looper setting in preset.");
    for (const auto* child : xml->getChildIterator())
    {
        if (!child->hasTagName("PEDAL") || board.pedals.size() >= 16 || !child->hasAttribute("template")
            || (child->getStringAttribute("loaded") != "0" && child->getStringAttribute("loaded") != "1"))
            return juce::Result::fail("Invalid pedal in preset.");
        BoardPreset::Pedal pedal;
        pedal.templateId = child->getStringAttribute("template");
        pedal.loaded = child->getBoolAttribute("loaded");
        if (pedal.loaded)
        {
            const auto result = readPedalPresetXml(*child, pedal.plugin);
            if (result.failed()) return result;
        }
        board.pedals.push_back(std::move(pedal));
    }
    if (board.deckIndex > static_cast<int>(board.pedals.size()))
        return juce::Result::fail("Invalid deck position in preset.");
    destination = std::move(board);
    return juce::Result::ok();
}
