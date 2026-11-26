#include "LooperEngine.h"
#include "BoardUtilities.h"
#include "LoopFile.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>

static bool near(float a, float b) { return std::abs(a - b) < 0.0001f; }

static void run(LooperEngine& engine, const float* input, float* output, int frames)
{
    const float* inputs[] { input };
    float* outputs[] { output };
    engine.process(inputs, 1, outputs, 1, frames);
}

static void command(LooperEngine& engine, LooperEngine::Command action, int layer = -1)
{
    assert(engine.enqueue(action, layer));
    run(engine, nullptr, nullptr, 0);
}

static void setup(LooperEngine& engine)
{
    assert(engine.prepare(100));
    engine.outputGain = 1.0f;
    engine.liveGain = 0.0f;
    engine.metronome = false;
}

static void firstLoop(LooperEngine& engine)
{
    const float input[] { 1, 2, 3 };
    float output[3]{};
    command(engine, LooperEngine::Command::record);
    run(engine, input, output, 3);
    command(engine, LooperEngine::Command::record);
    assert(engine.getLayerCount() == 1);
    assert(engine.getLength() == 3);
}

static void testProgress()
{
    LooperEngine engine;
    setup(engine);
    float silence[4]{};
    command(engine, LooperEngine::Command::record);
    run(engine, silence, nullptr, 4);
    assert(engine.getProgress() == 0.0);
    command(engine, LooperEngine::Command::record);
    run(engine, silence, nullptr, 1);
    assert(engine.getProgress() == .25);
    command(engine, LooperEngine::Command::record);
    run(engine, silence, nullptr, 1);
    assert(engine.getState() == LooperEngine::State::waiting && engine.getProgress() == .5);
    run(engine, silence, nullptr, 2);
    assert(engine.getProgress() == 0.0);
    run(engine, silence, nullptr, 1);
    assert(engine.getState() == LooperEngine::State::overdubbing && engine.getProgress() == .25);
    command(engine, LooperEngine::Command::stop);
    const auto stopped = engine.getProgress();
    run(engine, silence, nullptr, 2);
    assert(engine.getProgress() == stopped);
}

static void testLayers()
{
    LooperEngine engine;
    setup(engine);
    firstLoop(engine);
    float silence[7]{}, output[7]{};
    run(engine, silence, output, 7);
    const float expected[] { 1, 2, 3, 1, 2, 3, 1 };
    for (int i = 0; i < 7; ++i) assert(near(output[i], expected[i]));
    command(engine, LooperEngine::Command::record);
    run(engine, silence, output, 2);
    assert(engine.getState() == LooperEngine::State::waiting);
    assert(engine.getPosition() == 0);
    const float take[] { 10, 20, 30 };
    run(engine, take, output, 3);
    assert(engine.getState() == LooperEngine::State::playing);
    assert(engine.getLayerCount() == 2);
    run(engine, silence, output, 3);
    assert(near(output[0], 11) && near(output[1], 22) && near(output[2], 33));

    engine.getLayerControls(1).volume = 0.5f;
    run(engine, silence, output, 3);
    assert(near(output[0], 6) && near(output[1], 12) && near(output[2], 18));
    engine.getLayerControls(0).solo = true;
    run(engine, silence, output, 3);
    assert(near(output[0], 1) && near(output[1], 2) && near(output[2], 3));
    engine.getLayerControls(1).solo = true;
    run(engine, silence, output, 3);
    assert(near(output[0], 6) && near(output[2], 18));
    engine.getLayerControls(1).solo = false;
    engine.getLayerControls(0).active = false;
    run(engine, silence, output, 3);
    assert(near(output[0], 5) && near(output[1], 10) && near(output[2], 15));
    engine.getLayerControls(0).active = true;
    engine.getLayerControls(0).solo = false;
    engine.getLayerControls(1).volume = 1.0f;
    command(engine, LooperEngine::Command::undo);
    run(engine, silence, output, 3);
    assert(engine.getLayerCount() == 1 && near(output[0], 1));
    command(engine, LooperEngine::Command::undo);
    run(engine, silence, output, 3);
    assert(engine.getLayerCount() == 2 && near(output[0], 11));

    command(engine, LooperEngine::Command::rerecord, 0);
    const float replacement[] { 4, 5, 6 };
    run(engine, replacement, output, 3);
    assert(near(output[0], 11) && near(output[2], 33)); // Old take plays until replacement completes.
    run(engine, silence, output, 3);
    assert(near(output[0], 14) && near(output[1], 25) && near(output[2], 36));
    command(engine, LooperEngine::Command::undo);
    run(engine, silence, output, 3);
    assert(near(output[0], 11) && near(output[2], 33));
    command(engine, LooperEngine::Command::undo);
    run(engine, silence, output, 3);
    assert(near(output[0], 14) && near(output[2], 36));

    command(engine, LooperEngine::Command::rerecord, 0);
    run(engine, take, output, 1);
    command(engine, LooperEngine::Command::stop);
    command(engine, LooperEngine::Command::record);
    run(engine, silence, output, 3);
    assert(near(output[0], 14) && near(output[2], 36));
    assert(!engine.prepare(200));
    command(engine, LooperEngine::Command::rerecord, 9);
    assert(engine.takeNotice() == LooperEngine::Notice::invalidCommand);
    command(engine, LooperEngine::Command::clear);
    assert(engine.getLayerCount() == 0 && engine.getLength() == 0);
    assert(engine.prepare(200));
}

static void testStereoAndPan()
{
    LooperEngine engine;
    setup(engine);
    const float left[] { 0.5f, 0.5f, 0.5f };
    const float right[] { 0, 0, 0 };
    const float* inputs[] { left, right };
    float outputLeft[3]{}, outputRight[3]{};
    float* outputs[] { outputLeft, outputRight };
    command(engine, LooperEngine::Command::record);
    engine.process(inputs, 2, outputs, 2, 3);
    command(engine, LooperEngine::Command::record);
    engine.process(nullptr, 0, outputs, 2, 3);
    for (int i = 0; i < 3; ++i)
        assert(near(outputLeft[i], 0.5f) && near(outputRight[i], 0.5f));
    engine.getLayerControls(0).pan = -1.0f;
    engine.process(nullptr, 0, outputs, 2, 3);
    assert(near(outputLeft[0], 0.5f) && near(outputRight[0], 0));
    engine.getLayerControls(0).pan = 1.0f;
    engine.process(nullptr, 0, outputs, 2, 3);
    assert(near(outputLeft[0], 0) && near(outputRight[0], 0.5f));

    command(engine, LooperEngine::Command::clear);
    engine.inputMode = LooperEngine::InputMode::stereo;
    command(engine, LooperEngine::Command::record);
    engine.process(inputs, 2, outputs, 2, 3);
    command(engine, LooperEngine::Command::record);
    engine.process(nullptr, 0, outputs, 2, 3);
    assert(near(outputLeft[0], 0.5f) && near(outputRight[0], 0));

    command(engine, LooperEngine::Command::clear);
    engine.inputMode = LooperEngine::InputMode::monoSecond;
    const float* secondInput[] { right, left };
    engine.liveGain = 1.0f;
    command(engine, LooperEngine::Command::record);
    engine.process(secondInput, 2, outputs, 2, 3);
    assert(near(outputLeft[0], 0.5f) && near(outputRight[0], 0.5f));
}

static void testCountIn()
{
    for (int bars : { 1, 2, 4 })
    {
        LooperEngine engine;
        setup(engine);
        engine.countInBars = bars;
        engine.bpm = 120.0;
        engine.metronome = true;
        const int duration = 50 * 4 * bars;
        std::vector<float> input(static_cast<std::size_t>(duration), 0.5f);
        std::vector<float> output(static_cast<std::size_t>(duration));
        command(engine, LooperEngine::Command::record);
        assert(engine.getState() == LooperEngine::State::countingIn);
        run(engine, input.data(), output.data(), duration - 1);
        assert(engine.getLength() == 0 && engine.getLayerCount() == 0);
        assert(engine.getState() == LooperEngine::State::countingIn);
        for (int beat = 0; beat < bars * 4; ++beat)
            assert(near(output[static_cast<std::size_t>(beat * 50)], 0.12f));
        run(engine, input.data(), output.data(), 1);
        assert(engine.getState() == LooperEngine::State::recording);
        assert(engine.getLength() == 0);
        const float sample[] { 0.25f };
        run(engine, sample, output.data(), 1);
        command(engine, LooperEngine::Command::record);
        engine.metronome = false;
        run(engine, nullptr, output.data(), 1);
        assert(near(output[0], 0.25f)); // Click and count-in input were not recorded.
        command(engine, LooperEngine::Command::record);
        run(engine, sample, output.data(), 1);
        assert(engine.getLayerCount() == 2); // Subsequent layers have no count-in.
    }
    LooperEngine cancelled;
    setup(cancelled);
    cancelled.countInBars = 4;
    command(cancelled, LooperEngine::Command::record);
    command(cancelled, LooperEngine::Command::stop);
    assert(cancelled.getState() == LooperEngine::State::empty);
    assert(cancelled.getLayerCount() == 0 && cancelled.getCountBeats() == 0);

    cancelled.countInBars = 1;
    command(cancelled, LooperEngine::Command::record);
    float mutedOutput[200]{};
    run(cancelled, nullptr, mutedOutput, 200);
    assert(cancelled.getState() == LooperEngine::State::recording);
    assert(cancelled.getLength() == 0);
    for (float sample : mutedOutput) assert(near(sample, 0));
}

static void testAutoStopBars()
{
    LooperEngine engine;
    setup(engine);
    assert(engine.setAutoStopBars(1));
    assert(!engine.setAutoStopBars(3));
    assert(engine.takeNotice() == LooperEngine::Notice::invalidSetting);
    std::vector<float> input(200, 0.25f), output(200);
    command(engine, LooperEngine::Command::record);
    run(engine, input.data(), output.data(), 199);
    assert(engine.getState() == LooperEngine::State::recording && engine.getLength() == 199);
    run(engine, input.data() + 199, output.data() + 199, 1);
    assert(engine.getState() == LooperEngine::State::playing);
    assert(engine.getLength() == 200 && engine.getLayerCount() == 1);
    assert(!engine.setAutoStopBars(2));
    assert(engine.takeNotice() == LooperEngine::Notice::invalidSetting);

    LooperEngine countedIn;
    setup(countedIn);
    countedIn.countInBars = 1;
    assert(countedIn.setAutoStopBars(1));
    std::vector<float> bar(200), recorded(200);
    command(countedIn, LooperEngine::Command::record);
    run(countedIn, bar.data(), recorded.data(), 200);
    assert(countedIn.getState() == LooperEngine::State::recording && countedIn.getLength() == 0);
    run(countedIn, bar.data(), recorded.data(), 199);
    assert(countedIn.getState() == LooperEngine::State::recording);
    run(countedIn, bar.data() + 199, recorded.data() + 199, 1);
    assert(countedIn.getState() == LooperEngine::State::playing && countedIn.getLength() == 200);

    LooperEngine signature;
    setup(signature);
    assert(signature.setTimeSignature(3, 8));
    assert(signature.setAutoStopBars(2));
    std::vector<float> twoBars(150), twoBarOutput(150);
    command(signature, LooperEngine::Command::record);
    run(signature, twoBars.data(), twoBarOutput.data(), 149);
    assert(signature.getState() == LooperEngine::State::recording);
    run(signature, twoBars.data() + 149, twoBarOutput.data() + 149, 1);
    assert(signature.getState() == LooperEngine::State::playing && signature.getLength() == 150);
}

static void testMetronome()
{
    LooperEngine engine;
    assert(engine.prepare(10000));
    engine.outputGain = 1.0f;
    engine.liveGain = 0.0f;
    std::vector<float> left(20010), right(20010);
    float* outputs[] { left.data(), right.data() };
    command(engine, LooperEngine::Command::record);
    engine.process(nullptr, 0, outputs, 2, 20010);
    for (int beat : { 0, 5000, 10000, 15000, 20000 })
        assert(near(left[static_cast<std::size_t>(beat)], 0.12f));
    for (std::size_t i = 0; i < left.size(); ++i) assert(near(left[i], right[i]));
    assert(near(left[1], left[20001]));
    assert(!near(left[1], left[5001])); // Downbeat has a different pitch.
    assert(near(left[250], 0) && near(left[4999], 0));
    command(engine, LooperEngine::Command::record);
    engine.metronome = false;
    engine.process(nullptr, 0, outputs, 2, 20010);
    for (float sample : left) assert(near(sample, 0));
    engine.metronome = true;
    command(engine, LooperEngine::Command::stop);
    engine.process(nullptr, 0, outputs, 2, 10);
    for (int i = 0; i < 10; ++i) assert(near(left[static_cast<std::size_t>(i)], 0));

    LooperEngine fractional;
    assert(fractional.prepare(1000));
    fractional.outputGain = 1.0f;
    fractional.bpm = 123.0;
    command(fractional, LooperEngine::Command::record);
    std::vector<float> samples(1960);
    for (int offset = 0; offset < 1960; offset += 37)
        run(fractional, nullptr, samples.data() + offset, std::min(37, 1960 - offset));
    std::vector<int> starts;
    for (int i = 0; i < 1960; ++i)
        if (near(samples[static_cast<std::size_t>(i)], 0.12f)) starts.push_back(i);
    assert((starts == std::vector<int> { 0, 488, 976, 1464, 1952 }));
}

static void testTimeSignatures()
{
    for (const auto signature : { LooperEngine::TimeSignature { 3, 4 }, { 6, 8 }, { 7, 8 }, { 5, 4 } })
    {
        LooperEngine engine;
        setup(engine);
        assert(engine.setTimeSignature(signature.beats, signature.unit));
        engine.countInBars = 2;
        engine.metronome = true;
        const int beatFrames = 50 * 4 / signature.unit;
        const int duration = beatFrames * signature.beats * 2;
        std::vector<float> output(static_cast<std::size_t>(duration));
        command(engine, LooperEngine::Command::record);
        assert(engine.getCountBeats() == signature.beats * 2);
        assert(!engine.setTimeSignature(4, 4));
        assert(engine.takeNotice() == LooperEngine::Notice::invalidSetting);
        run(engine, nullptr, output.data(), duration - 1);
        assert(engine.getLength() == 0 && engine.getState() == LooperEngine::State::countingIn);
        for (int i = 0; i < signature.beats * 2; ++i)
            assert(near(output[static_cast<std::size_t>(i * beatFrames)], 0.12f));
        run(engine, nullptr, output.data(), 1);
        assert(engine.getState() == LooperEngine::State::recording && engine.getLength() == 0);
        assert(engine.setTimeSignature(4, 4));
    }

    LooperEngine engine;
    assert(!engine.setTimeSignature(0, 4));
    assert(!engine.setTimeSignature(17, 4));
    assert(!engine.setTimeSignature(3, 3));
    assert(engine.getTimeSignature().beats == 4 && engine.getTimeSignature().unit == 4);
    assert(engine.takeNotice() == LooperEngine::Notice::invalidSetting);
    assert(engine.prepare(10000));
    engine.outputGain = 1.0f;
    engine.liveGain = 0.0f;
    assert(engine.setTimeSignature(3, 8));
    std::vector<float> output(7510);
    command(engine, LooperEngine::Command::record);
    run(engine, nullptr, output.data(), 7510);
    assert(near(output[0], 0.12f) && near(output[2500], 0.12f) && near(output[7500], 0.12f));
    assert(near(output[1], output[7501]));
    assert(!near(output[1], output[2501]));
}

static void testClassicOverdub()
{
    LooperEngine engine;
    setup(engine);
    assert(engine.setLayeredLoop(false));
    firstLoop(engine);
    assert(!engine.setLayeredLoop(true));
    assert(engine.takeNotice() == LooperEngine::Notice::invalidSetting);
    const float input[] { 10, 20, 30 };
    float output[3]{};
    command(engine, LooperEngine::Command::record);
    run(engine, input, output, 3);
    assert(engine.getState() == LooperEngine::State::playing && engine.getLayerCount() == 1);
    run(engine, nullptr, output, 3);
    assert(near(output[0], 11) && near(output[2], 33));
    command(engine, LooperEngine::Command::undo);
    run(engine, nullptr, output, 3);
    assert(near(output[0], 1) && near(output[2], 3));
    command(engine, LooperEngine::Command::undo);
    run(engine, nullptr, output, 3);
    assert(near(output[0], 11) && near(output[2], 33));
    command(engine, LooperEngine::Command::record);
    run(engine, input, output, 1);
    command(engine, LooperEngine::Command::stop);
    command(engine, LooperEngine::Command::record);
    run(engine, nullptr, output, 3);
    assert(near(output[0], 11) && near(output[2], 33));
    command(engine, LooperEngine::Command::rerecord, 0);
    run(engine, input, output, 3);
    run(engine, nullptr, output, 3);
    assert(near(output[0], 10) && near(output[2], 30));
    assert(engine.getLayerCount() == 1);
    command(engine, LooperEngine::Command::clear);
    assert(engine.setLayeredLoop(true));
    firstLoop(engine);
    command(engine, LooperEngine::Command::record);
    run(engine, input, output, 3);
    assert(engine.getLayerCount() == 2);
}

static void testDeleteLayers()
{
    LooperEngine engine;
    setup(engine);
    firstLoop(engine);
    float output[3]{};
    const float second[] { 10, 20, 30 }, third[] { 100, 200, 300 };
    command(engine, LooperEngine::Command::record);
    run(engine, second, output, 3);
    command(engine, LooperEngine::Command::record);
    run(engine, third, output, 3);
    engine.getLayerControls(2).volume = 0.5f;
    engine.getLayerControls(2).pan = 1.0f;
    command(engine, LooperEngine::Command::removeLayer, 1);
    assert(engine.getLayerCount() == 2 && engine.getLength() == 3);
    assert(!engine.canUndo());
    assert(near(engine.getLayerControls(1).volume.load(), 0.5f));
    assert(near(engine.getLayerControls(1).pan.load(), 1.0f));
    run(engine, nullptr, output, 3);
    assert(near(output[0], 26) && near(output[1], 52) && near(output[2], 78));
    engine.getLayerControls(1).solo = true;
    engine.getLayerControls(1).active = false;
    command(engine, LooperEngine::Command::stop);
    command(engine, LooperEngine::Command::removeLayer, 0);
    assert(engine.getState() == LooperEngine::State::stopped && engine.getLayerCount() == 1);
    assert(engine.getLayerControls(0).solo.load() && !engine.getLayerControls(0).active.load());
    engine.getLayerControls(0).active = true;
    command(engine, LooperEngine::Command::record);
    run(engine, nullptr, output, 3);
    assert(near(output[0], 25) && near(output[2], 75));
    command(engine, LooperEngine::Command::removeLayer, 9);
    assert(engine.takeNotice() == LooperEngine::Notice::invalidCommand && engine.getLayerCount() == 1);
    command(engine, LooperEngine::Command::record);
    command(engine, LooperEngine::Command::removeLayer, 0);
    assert(engine.takeNotice() == LooperEngine::Notice::invalidCommand && engine.getLayerCount() == 1);
    command(engine, LooperEngine::Command::record);
    command(engine, LooperEngine::Command::removeLayer, 0);
    assert(engine.getState() == LooperEngine::State::empty && engine.getLength() == 0);
    assert(engine.getLayerCount() == 0 && !engine.canUndo());
    firstLoop(engine);
    run(engine, nullptr, output, 3);
    assert(near(output[0], 1) && near(output[2], 3));
}

static void testLimits()
{
    LooperEngine engine;
    setup(engine);
    firstLoop(engine);
    const float input[] { 1, 1, 1 };
    float output[3]{};
    for (int i = 1; i < LooperEngine::maxLayers; ++i)
    {
        command(engine, LooperEngine::Command::record);
        run(engine, input, output, 3);
    }
    command(engine, LooperEngine::Command::record);
    assert(engine.takeNotice() == LooperEngine::Notice::layerLimit);
    assert(engine.getLayerCount() == 8);

    LooperEngine limited;
    assert(limited.prepare(2));
    limited.metronome = false;
    limited.outputGain = 1.0f;
    limited.loopGain = 0.0f;
    float longInput[128], longOutput[128];
    for (auto& sample : longInput) sample = 0.25f;
    command(limited, LooperEngine::Command::record);
    run(limited, longInput, longOutput, 128);
    assert(limited.getLength() == 120 && limited.getLayerCount() == 1);
    assert(limited.getState() == LooperEngine::State::playing);
    assert(limited.takeNotice() == LooperEngine::Notice::recordingLimit);
    for (float sample : longOutput) assert(near(sample, 0.25f));
}

static void testBoardUtilities()
{
    BoardUtilities board;
    std::array<float, 4096> left{}, right{};
    board.inputDb = 6.0206f;
    board.outputDb = -6.0206f;
    board.prepare(48000);
    left.fill(.1f);
    right.fill(.2f);
    board.before(left.data(), right.data(), static_cast<int>(left.size()));
    assert(near(left.back(), .2f) && near(right.back(), .4f));
    board.after(left.data(), right.data(), static_cast<int>(left.size()));
    assert(near(left.back(), .1f) && near(right.back(), .2f));

    board.inputDb = 0;
    board.gateDb = -20;
    board.gate = true;
    board.prepare(48000);
    left.fill(.01f);
    right.fill(.01f);
    board.before(left.data(), right.data(), static_cast<int>(left.size()));
    assert(std::abs(left.back()) < .00001f);
    left.fill(.3f);
    right.fill(.3f);
    board.before(left.data(), right.data(), static_cast<int>(left.size()));
    assert(near(left.back(), .3f));

    constexpr double pi = 3.141592653589793238;
    constexpr auto frames = 4096;
    board.gate = false;
    board.tuner = true;
    board.transpose = 12;
    board.prepare(48000);
    board.readPitch(); // discard data from the old device preparation
    int captured = 0;
    for (int block = 0; block < 4; ++block)
    {
        for (auto& sample : left) sample = static_cast<float>(.4 * std::sin(2 * pi * 110 * captured++ / 48000));
        right = left;
        board.before(left.data(), right.data(), frames);
    }
    assert(std::abs(board.readPitch() - 110) < .6f);
    int zeroCrossings = 0;
    for (int index = 1; index < frames; ++index)
        if (left[static_cast<std::size_t>(index - 1)] < 0 && left[static_cast<std::size_t>(index)] >= 0) ++zeroCrossings;
    assert(zeroCrossings >= 17 && zeroCrossings <= 22);
    board.tuner = false;
    assert(board.readPitch() == 0);

    board.outputDb = 0;
    board.doubler = true;
    board.doublerMs = 20;
    board.prepare(48000);
    left.fill(0);
    right.fill(0);
    board.after(left.data(), right.data(), frames);
    left[0] = 1.0f;
    board.after(left.data(), right.data(), frames);
    assert(near(left[0], .66f) && near(right[0], 0));
    assert(right[758] > .1f || right[759] > .1f);
}

static void testPreparedStereo()
{
    LooperEngine engine;
    setup(engine);
    float left[] { .2f, .4f }, right[] { -.3f, -.6f };
    const float* in[] { left, right };
    float l[2]{}, r[2]{};
    float* out[] { l, r };
    command(engine, LooperEngine::Command::record);
    engine.process(in, 2, out, 2, 2, true);
    command(engine, LooperEngine::Command::record);
    engine.process(nullptr, 0, out, 2, 2, true);
    assert(near(l[0], .2f) && near(l[1], .4f) && near(r[0], -.3f) && near(r[1], -.6f));

    LooperEngine withClick;
    assert(withClick.prepare(4800));
    command(withClick, LooperEngine::Command::record);
    std::array<float, 128> music{}, clicks{};
    float* musicOut[] { music.data() };
    withClick.process(nullptr, 0, musicOut, 1, static_cast<int>(music.size()), false, clicks.data());
    assert(std::all_of(music.begin(), music.end(), [](float sample) { return sample == 0; }));
    assert(std::any_of(clicks.begin(), clicks.end(), [](float sample) { return std::abs(sample) > .01f; }));
}

static void testLoopProjects()
{
    LooperEngine engine;
    setup(engine);
    firstLoop(engine);
    command(engine, LooperEngine::Command::record);
    const float take[] { 10, 20, 30 };
    run(engine, take, nullptr, 3);
    LooperEngine::Project captured;
    assert(!engine.captureProject(captured));
    command(engine, LooperEngine::Command::stop);
    engine.getLayerControls(0).volume = .5f;
    engine.getLayerControls(0).pan = -1;
    engine.getLayerControls(1).solo = true;
    engine.getLayerControls(1).active = false;
    engine.bpm = 137;
    engine.countInBars = 2;
    engine.autoStopBars = 4;
    assert(engine.setTimeSignature(6, 8));
    assert(engine.captureProject(captured));
    assert(captured.layers.size() == 2);
    captured.layers[1].audio[1] = { 7, 8, 9 };
    std::ostringstream encoded(std::ios::binary);
    assert(LoopFile::save(encoded, captured));
    LooperEngine::Project decoded;
    std::istringstream input(encoded.str(), std::ios::binary);
    std::string error;
    assert(LoopFile::load(input, decoded, error) && error.empty());
    LooperEngine restored;
    setup(restored);
    assert(restored.restoreProject(decoded));
    assert(restored.getState() == LooperEngine::State::stopped);
    assert(restored.getLength() == 3 && restored.getLayerCount() == 2);
    assert(restored.bpm == 137 && restored.getTimeSignature().unit == 8
        && restored.getTimeSignature().beats == 6 && restored.countInBars == 2 && restored.autoStopBars == 4);
    assert(restored.getLayerControls(1).solo && !restored.getLayerControls(1).active);
    LooperEngine::Project again;
    assert(restored.captureProject(again));
    assert(again.layers[1].audio[1] == captured.layers[1].audio[1]);
    auto mixed = LoopFile::mix(decoded);
    assert(near(mixed[0][0], .5f) && near(mixed[1][0], 0));
    decoded.layers[1].active = true;
    mixed = LoopFile::mix(decoded);
    assert(near(mixed[0][0], 10) && near(mixed[1][0], 7));
    command(restored, LooperEngine::Command::record);
    assert(!restored.restoreProject(decoded));
    float left[3]{}, right[3]{};
    float* out[] { left, right };
    restored.process(nullptr, 0, out, 2, 3);
    assert(near(left[0], .5f) && near(right[0], 0));
    command(restored, LooperEngine::Command::stop);
    decoded.layers[0].audio[0][0] = std::numeric_limits<float>::quiet_NaN();
    assert(!restored.restoreProject(decoded));
    assert(restored.getLayerCount() == 2 && restored.getLength() == 3);
    for (const auto& data : { std::string("not a loop"), encoded.str().substr(0, encoded.str().size() - 1),
                              encoded.str() + "trailing bytes" })
    {
        std::istringstream broken(data, std::ios::binary);
        assert(!LoopFile::load(broken, again, error) && !error.empty());
        assert(again.layers[1].audio[1] == captured.layers[1].audio[1]);
    }
    auto oversized = encoded.str();
    const std::uint32_t frames = 0xffffffff;
    std::memcpy(&oversized[52], &frames, sizeof(frames));
    std::istringstream invalidFrames(oversized, std::ios::binary);
    assert(!LoopFile::load(invalidFrames, again, error));
    decoded = captured;
    decoded.sampleRate = 200;
    assert(!restored.restoreProject(decoded));
    decoded = captured;
    decoded.layers[0].audio[1].pop_back();
    assert(!restored.restoreProject(decoded));
}

int main()
{
    testProgress();
    testLayers();
    testStereoAndPan();
    testCountIn();
    testAutoStopBars();
    testMetronome();
    testTimeSignatures();
    testClassicOverdub();
    testDeleteLayers();
    testLimits();
    testBoardUtilities();
    testPreparedStereo();
    testLoopProjects();
    std::cout << "Layer, stereo/pan, rerecord, metronome, count-in and auto-stop tests passed\n";
}
