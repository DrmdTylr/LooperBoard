#include "../src/LoopAudioFile.h"
#include "../src/PedalPreset.h"
#include "../src/BoardPreset.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>

int main()
{
    juce::TemporaryFile pedalFile(".pedal");
    PedalPreset pedal;
    pedal.plugin.name = "Example";
    pedal.plugin.pluginFormatName = "VST3";
    pedal.plugin.fileOrIdentifier = "Example.vst3";
    pedal.templateId = "custom";
    pedal.mappings.add("gain-id");
    pedal.mappings.add("");
    pedal.bypassed = true;
    pedal.state.append("test state", 10);
    assert(savePedalPreset(pedalFile.getFile(), pedal).wasOk());
    PedalPreset loadedPedal;
    assert(loadPedalPreset(pedalFile.getFile(), loadedPedal).wasOk());
    assert(loadedPedal.plugin.name == pedal.plugin.name && loadedPedal.templateId == pedal.templateId
        && loadedPedal.mappings == pedal.mappings && loadedPedal.bypassed && loadedPedal.state == pedal.state);
    assert(pedalFile.getFile().replaceWithText("<LOOPER_PEDAL version=\"1\" template=\"custom\" bypassed=\"1\">"
        "<PLUGIN name=\"Example\" format=\"VST3\" file=\"Example.vst3\"/><STATE>-1.invalid</STATE><MAPPINGS/></LOOPER_PEDAL>"));
    assert(loadPedalPreset(pedalFile.getFile(), loadedPedal).failed());
    assert(loadedPedal.state == pedal.state && loadedPedal.mappings == pedal.mappings);
    pedal.state.reset();
    assert(savePedalPreset(pedalFile.getFile(), pedal).wasOk());
    assert(loadPedalPreset(pedalFile.getFile(), loadedPedal).wasOk() && loadedPedal.state.isEmpty());

    juce::StringArray legacyMappings { "decay", "tone", "mix" };
    assert(upgradePedalMappings(legacyMappings, 8, 3));
    assert(legacyMappings.size() == 8 && legacyMappings[1] == "tone" && legacyMappings[2] == "mix");
    juce::StringArray invalidMappings { "one", "two" };
    assert(!upgradePedalMappings(invalidMappings, 8, 3) && invalidMappings.size() == 2);
    auto extendedPedal = pedal;
    extendedPedal.mappings = legacyMappings;
    assert(savePedalPreset(pedalFile.getFile(), extendedPedal).wasOk());
    assert(loadPedalPreset(pedalFile.getFile(), loadedPedal).wasOk() && loadedPedal.mappings == legacyMappings);
    extendedPedal.mappings.add("too-many");
    assert(savePedalPreset(pedalFile.getFile(), extendedPedal).wasOk());
    assert(loadPedalPreset(pedalFile.getFile(), loadedPedal).failed());

    juce::TemporaryFile boardFile(".board");
    BoardPreset board;
    board.deckIndex = 1;
    board.bpm = 95;
    board.footer[1] = -36;
    board.gate = true;
    board.pedals.push_back({ "custom", true, pedal });
    board.pedals.push_back({ "compressor", false, {} });
    assert(saveBoardPreset(boardFile.getFile(), board).wasOk());
    BoardPreset openedBoard;
    assert(loadBoardPreset(boardFile.getFile(), openedBoard).wasOk());
    assert(openedBoard.deckIndex == 1 && openedBoard.bpm == 95 && openedBoard.gate
        && openedBoard.footer[1] == -36 && openedBoard.pedals.size() == 2);
    assert(openedBoard.pedals[0].loaded && openedBoard.pedals[0].plugin.mappings == pedal.mappings
        && !openedBoard.pedals[1].loaded);
    auto invalidBoard = juce::XmlDocument::parse(boardFile.getFile());
    assert(invalidBoard != nullptr);
    invalidBoard->setAttribute("deck", 3);
    assert(boardFile.getFile().replaceWithText(invalidBoard->toString()));
    assert(loadBoardPreset(boardFile.getFile(), openedBoard).failed());
    assert(openedBoard.deckIndex == 1 && openedBoard.pedals.size() == 2);
    invalidBoard->setAttribute("deck", 1);
    invalidBoard->setAttribute("trim", "NaN");
    assert(boardFile.getFile().replaceWithText(invalidBoard->toString()));
    assert(loadBoardPreset(boardFile.getFile(), openedBoard).failed());
    assert(openedBoard.footer[1] == -36);

    juce::TemporaryFile projectFile(".loop"), wavFile(".wav"), monoFile(".wav");
    LooperEngine::Project project;
    project.sampleRate = 8000;
    project.gains = { 1, 0, 1, 1 };
    project.layers.resize(2);
    for (auto& layer : project.layers)
    {
        layer.audio[0].assign(800, .25f);
        layer.audio[1].assign(800, -.125f);
    }
    project.layers[0].pan = -1;
    project.layers[1].volume = .5f;
    assert(LoopAudioFile::saveProject(projectFile.getFile(), project).wasOk());
    LooperEngine::Project opened;
    assert(LoopAudioFile::loadProject(projectFile.getFile(), opened).wasOk());
    assert(opened.layers.size() == 2 && opened.layers[1].volume == .5f);
    assert(opened.layers[0].audio == project.layers[0].audio);
    juce::MemoryBlock original, after;
    assert(projectFile.getFile().loadFileAsData(original));
    auto invalid = project;
    invalid.layers.clear();
    assert(LoopAudioFile::saveProject(projectFile.getFile(), invalid).failed());
    assert(projectFile.getFile().loadFileAsData(after) && after == original);
    assert(LoopAudioFile::exportWav(wavFile.getFile(), project).wasOk());
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatReader> reader(
        format.createReaderFor(wavFile.getFile().createInputStream().release(), true));
    assert(reader != nullptr && reader->numChannels == 2 && reader->lengthInSamples == 800);
    assert(reader->usesFloatingPointData);
    assert(LoopAudioFile::importWav(wavFile.getFile(), 8000, opened).wasOk());
    assert(opened.layers.size() == 1 && opened.sampleRate == 8000);
    assert(std::abs(opened.layers[0].audio[0][0] - .375f) < .00001f);
    assert(std::abs(opened.layers[0].audio[1][0] + .0625f) < .00001f);
    assert(LoopAudioFile::resample(opened, 16000).wasOk());
    assert(opened.sampleRate == 16000 && opened.layers[0].audio[0].size() == 1600);
    assert(std::abs(opened.layers[0].audio[0][800] - .375f) < .001f);
    assert(LoopAudioFile::resample(opened, 8000).wasOk());
    assert(opened.layers[0].audio[0].size() == 800);
    {
        auto stream = monoFile.getFile().createOutputStream();
        std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.get(), 8000, 1, 16, {}, 0));
        assert(writer != nullptr);
        stream.release();
        std::vector<float> samples(80, .25f);
        const float* channels[] { samples.data() };
        assert(writer->writeFromFloatArrays(channels, 1, 80));
    }
    assert(LoopAudioFile::importWav(monoFile.getFile(), 8000, opened).wasOk());
    assert(opened.layers[0].audio[0] == opened.layers[0].audio[1]);
    assert(std::abs(opened.layers[0].audio[0][0] - .25f) < .001f);
    assert(LoopAudioFile::importWav(projectFile.getFile(), 8000, opened).failed());
    assert(LoopAudioFile::resample(opened, 0).failed());
    assert(opened.layers[0].audio[0].size() == 80);
    assert(LoopAudioFile::loadProject(wavFile.getFile(), opened).failed());
    juce::MemoryBlock truncated;
    assert(wavFile.getFile().loadFileAsData(truncated));
    truncated.setSize(truncated.getSize() - 8);
    assert(wavFile.getFile().replaceWithData(truncated.getData(), truncated.getSize()));
    assert(LoopAudioFile::importWav(wavFile.getFile(), 8000, opened).failed());
    assert(opened.layers[0].audio[0].size() == 80);
    std::cout << "Project persistence, safe overwrite, WAV mix/import and resampling tests passed\n";
}
