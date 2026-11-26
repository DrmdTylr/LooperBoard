#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <vector>

class LooperEngine
{
public:
    enum class State { empty, countingIn, recording, playing, waiting, overdubbing, stopped };
    enum class Command { record, stop, undo, clear, rerecord, removeLayer };
    enum class InputMode { monoFirst, monoSecond, stereo };
    enum class Notice { none, recordingLimit, layerLimit, invalidCommand, invalidSetting };
    struct TimeSignature { int beats, unit; };
    static constexpr int maxSeconds = 60;
    static constexpr int maxLayers = 8;

    struct LayerControls
    {
        std::atomic<float> volume { 1.0f }, pan { 0.0f };
        std::atomic<bool> active { true }, solo { false };
    };

    struct SavedLayer
    {
        std::array<std::vector<float>, 2> audio;
        float volume = 1.0f, pan = 0.0f;
        bool active = true, solo = false;
    };

    struct Project
    {
        double sampleRate = 0.0, tempo = 120.0;
        TimeSignature signature { 4, 4 };
        bool layered = true, click = true;
        int countIn = 0, autoStop = 0;
        std::array<float, 4> gains { 1.0f, 1.0f, 1.0f, 0.7f };
        std::vector<SavedLayer> layers;
    };

    static std::size_t capacityForRate(double rate)
    {
        constexpr std::size_t memoryBudget = 256 * 1024 * 1024;
        return std::min(static_cast<std::size_t>(rate * maxSeconds),
                        memoryBudget / ((maxLayers + 1) * 2 * sizeof(float)));
    }

    static bool validProject(const Project& project)
    {
        if (!std::isfinite(project.sampleRate) || project.sampleRate <= 0.0 || project.sampleRate > 384000.0
            || !std::isfinite(project.tempo) || project.tempo < 30.0 || project.tempo > 300.0
            || project.signature.beats < 1 || project.signature.beats > 16
            || (project.signature.unit != 1 && project.signature.unit != 2 && project.signature.unit != 4
                && project.signature.unit != 8 && project.signature.unit != 16)
            || (project.countIn != 0 && project.countIn != 1 && project.countIn != 2 && project.countIn != 4)
            || (project.autoStop != 0 && project.autoStop != 1 && project.autoStop != 2
                && project.autoStop != 4 && project.autoStop != 8 && project.autoStop != 16)
            || project.layers.empty() || project.layers.size() > maxLayers
            || (!project.layered && project.layers.size() != 1))
            return false;
        for (auto gain : project.gains)
            if (!std::isfinite(gain) || gain < 0.0f || gain > 1.0f) return false;
        const auto frames = project.layers.front().audio[0].size();
        if (frames == 0 || frames > capacityForRate(project.sampleRate)) return false;
        for (const auto& layer : project.layers)
        {
            if (!std::isfinite(layer.volume) || layer.volume < 0.0f || layer.volume > 1.0f
                || !std::isfinite(layer.pan) || layer.pan < -1.0f || layer.pan > 1.0f)
                return false;
            for (const auto& channel : layer.audio)
                if (channel.size() != frames
                    || !std::all_of(channel.begin(), channel.end(), [](float value) { return std::isfinite(value); }))
                    return false;
        }
        return true;
    }

    // Call only with the audio callback excluded; file operations require stopped transport.
    bool captureProject(Project& project) const
    {
        if (getState() != State::stopped || getLayerCount() == 0) return false;
        project = {};
        project.sampleRate = getSampleRate();
        project.tempo = bpm.load();
        project.signature = getTimeSignature();
        project.layered = isLayeredLoop();
        project.click = metronome.load();
        project.countIn = countInBars.load();
        project.autoStop = autoStopBars.load();
        project.gains = { inputGain.load(), liveGain.load(), loopGain.load(), outputGain.load() };
        for (int i = 0; i < getLayerCount(); ++i)
        {
            const auto& layer = layers[static_cast<std::size_t>(i)];
            SavedLayer saved;
            saved.volume = layer.controls.volume.load();
            saved.pan = layer.controls.pan.load();
            saved.active = layer.controls.active.load();
            saved.solo = layer.controls.solo.load();
            for (std::size_t ch = 0; ch < 2; ++ch)
                saved.audio[ch].assign(layer.audio[ch].begin(), layer.audio[ch].begin() + getLength());
            project.layers.push_back(std::move(saved));
        }
        return true;
    }

    bool restoreProject(const Project& project)
    {
        if ((getState() != State::empty && getState() != State::stopped)
            || project.sampleRate != getSampleRate() || !validProject(project))
            return false;
        reset();
        bpm = project.tempo;
        timeSignature = (project.signature.beats << 8) | project.signature.unit;
        layeredLoop = project.layered;
        metronome = project.click;
        countInBars = project.countIn;
        autoStopBars = project.autoStop;
        inputGain = project.gains[0];
        liveGain = project.gains[1];
        loopGain = project.gains[2];
        outputGain = project.gains[3];
        for (std::size_t i = 0; i < project.layers.size(); ++i)
        {
            const auto& saved = project.layers[i];
            auto& layer = layers[i];
            for (std::size_t ch = 0; ch < 2; ++ch)
                std::copy(saved.audio[ch].begin(), saved.audio[ch].end(), layer.audio[ch].begin());
            layer.controls.volume = saved.volume;
            layer.controls.pan = saved.pan;
            layer.controls.active = saved.active;
            layer.controls.solo = saved.solo;
        }
        length = project.layers.front().audio[0].size();
        layerCount = static_cast<int>(project.layers.size());
        state = State::stopped;
        return true;
    }

    bool prepare(double rate)
    {
        if (!std::isfinite(rate) || rate <= 0.0 || rate > 384000.0)
            return false;
        if (sampleRate.load() != 0.0 && sampleRate.load() != rate && length.load() != 0)
            return false;
        if (sampleRate.load() != rate)
        {
            capacity = capacityForRate(rate);
            for (auto& layer : layers)
                for (auto& channel : layer.audio) channel.assign(capacity, 0.0f);
            for (auto& channel : take) channel.assign(capacity, 0.0f);
            sampleRate.store(rate);
            reset();
        }
        return true;
    }

    bool enqueue(Command command, int layer = -1)
    {
        const auto write = writeIndex.load(std::memory_order_relaxed);
        const auto next = (write + 1) % commands.size();
        if (next == readIndex.load(std::memory_order_acquire))
            return false;
        commands[write] = { command, layer };
        writeIndex.store(next, std::memory_order_release);
        return true;
    }

    void process(const float* const* inputs, int inputChannels, float* const* outputs,
                 int outputChannels, int frames, bool stereoPrepared = false, float* separateClick = nullptr)
    {
        drainCommands();
        bool anySolo = false;
        const auto count = layerCount.load();
        for (int layer = 0; layer < count; ++layer)
            anySolo = anySolo || (layers[static_cast<std::size_t>(layer)].controls.active.load()
                                  && layers[static_cast<std::size_t>(layer)].controls.solo.load());
        std::array<std::array<float, 2>, maxLayers> targets{};
        for (int layer = 0; layer < maxLayers; ++layer)
        {
            const auto& controls = layers[static_cast<std::size_t>(layer)].controls;
            const float gain = controls.active.load() && (!anySolo || controls.solo.load())
                ? controls.volume.load() : 0.0f;
            const float pan = controls.pan.load();
            targets[static_cast<std::size_t>(layer)] = {
                gain * (pan > 0.0f ? 1.0f - pan : 1.0f),
                gain * (pan < 0.0f ? 1.0f + pan : 1.0f)
            };
        }
        const double rate = sampleRate.load();
        if (rate <= 0.0)
            return;
        const float smoothing = static_cast<float>(std::min(1.0, 1.0 / (rate * 0.005)));
        const double currentBpm = std::clamp(bpm.load(), 30.0, 300.0);
        float inPeak = 0.0f, outPeak = 0.0f;
        for (int i = 0; i < frames; ++i)
        {
            auto current = state.load(std::memory_order_relaxed);
            auto pos = position.load(std::memory_order_relaxed);
            const auto len = length.load(std::memory_order_relaxed);
            if (current == State::waiting && pos == 0)
            {
                undoKind = UndoKind::none;
                hasUndo.store(false);
                undoApplied = false;
                state.store(State::overdubbing);
                current = State::overdubbing;
            }

            std::array<float, 2> in{};
            const auto mode = inputMode.load(std::memory_order_relaxed);
            for (int ch = 0; ch < 2; ++ch)
            {
                const int source = sourceChannel(mode, inputChannels, ch, stereoPrepared);
                if (source < inputChannels && inputs != nullptr && inputs[source] != nullptr)
                    in[static_cast<std::size_t>(ch)] = inputs[source][i] * inputGain.load(std::memory_order_relaxed);
                inPeak = std::max(inPeak, std::abs(in[static_cast<std::size_t>(ch)]));
            }

            std::array<float, 2> played{};
            for (int layer = 0; layer < maxLayers; ++layer)
            {
                const auto index = static_cast<std::size_t>(layer);
                for (std::size_t ch = 0; ch < 2; ++ch)
                {
                    layers[index].smoothed[ch] += smoothing * (targets[index][ch] - layers[index].smoothed[ch]);
                    if (layer < layerCount.load(std::memory_order_relaxed) && len > 0
                        && current != State::empty && current != State::stopped && current != State::countingIn)
                        played[ch] += layers[index].audio[ch][pos] * layers[index].smoothed[ch];
                }
            }
            if (current == State::recording || current == State::overdubbing)
                for (std::size_t ch = 0; ch < 2; ++ch)
                    take[ch][pos] = in[ch] + (current == State::overdubbing && mergeTake ? layers[0].audio[ch][pos] : 0.0f);

            const bool clockRunning = current != State::empty && current != State::stopped;
            const float click = clockRunning ? tick(current == State::countingIn ? countBpm : currentBpm,
                current == State::countingIn ? countSignature : getTimeSignature()) : 0.0f;
            const auto master = outputGain.load(std::memory_order_relaxed);
            if (separateClick != nullptr)
                separateClick[i] = click * master;
            std::array<float, 2> out{};
            for (std::size_t ch = 0; ch < 2; ++ch)
            {
                out[ch] = (in[ch] * liveGain.load(std::memory_order_relaxed)
                           + played[ch] * loopGain.load(std::memory_order_relaxed)
                           + (separateClick == nullptr ? click : 0.0f)) * master;
                outPeak = std::max(outPeak, std::abs(out[ch]));
            }
            if (outputs != nullptr)
                for (int ch = 0; ch < outputChannels; ++ch)
                    if (outputs[ch] != nullptr)
                        outputs[ch][i] = outputChannels == 1
                            ? (out[0] + out[1]) * 0.5f
                            : (ch < 2 ? out[static_cast<std::size_t>(ch)] : 0.0f);

            if (current == State::countingIn)
            {
                if (--countFrames == 0)
                {
                    remainingCountBeats.store(0);
                    resetClock();
                    state.store(State::recording);
                }
                else
                    remainingCountBeats.store(static_cast<int>(std::ceil(static_cast<double>(countFrames)
                                                                        / (rate * 60.0 / countBpm * 4.0 / countSignature.unit))));
            }
            else if (current == State::recording)
            {
                length.store(++pos, std::memory_order_relaxed);
                const auto maxFrames = capacity.load(std::memory_order_relaxed);
                const auto targetFrames = recordingFrames.load(std::memory_order_relaxed);
                const bool reachedAutoStop = targetFrames > 0 && pos >= targetFrames;
                if (pos >= maxFrames || reachedAutoStop)
                {
                    const bool exceededCapacity = pos >= maxFrames && (!reachedAutoStop || targetFrames > maxFrames);
                    finishFirst();
                    pos = 0;
                    if (exceededCapacity)
                        notice.store(Notice::recordingLimit);
                }
            }
            else if (len > 0 && current != State::stopped && current != State::empty)
            {
                pos = (pos + 1) % len;
                if (current == State::overdubbing && pos == 0)
                    finishLayer();
            }
            position.store(pos, std::memory_order_relaxed);
        }
        inputPeak.store(inPeak);
        outputPeak.store(outPeak);
    }

    State getState() const { return state.load(); }
    static int sourceChannel(InputMode mode, int channels, int channel, bool stereoPrepared = false)
    {
        return (stereoPrepared || mode == InputMode::stereo) && channels > 1 ? channel
            : (mode == InputMode::monoSecond && channels > 1 ? 1 : 0);
    }
    std::size_t getLength() const { return length.load(); }
    std::size_t getPosition() const { return position.load(); }
    double getProgress() const
    {
        const auto len = getLength();
        return getLayerCount() > 0 && len > 0
            ? std::min(1.0, static_cast<double>(getPosition()) / static_cast<double>(len)) : 0.0;
    }
    std::size_t getRecordingFrames() const { return recordingFrames.load(); }
    double getSampleRate() const { return sampleRate.load(); }
    double getMaxDuration() const { return sampleRate.load() > 0.0 ? static_cast<double>(capacity.load()) / sampleRate.load() : 60.0; }
    int getLayerCount() const { return layerCount.load(); }
    int getRecordingLayer() const { return recordingLayer.load(); }
    int getCountBeats() const { return remainingCountBeats.load(); }
    bool canUndo() const { return hasUndo.load(); }
    Notice takeNotice() { return notice.exchange(Notice::none); }
    LayerControls& getLayerControls(int layer) { return layers.at(static_cast<std::size_t>(layer)).controls; }
    bool isLayeredLoop() const { return layeredLoop.load(); }
    bool setLayeredLoop(bool enabled)
    {
        if (getState() != State::empty || getLength() != 0)
        {
            notice.store(Notice::invalidSetting);
            return false;
        }
        layeredLoop.store(enabled);
        return true;
    }
    bool setAutoStopBars(int bars)
    {
        if ((bars != 0 && bars != 1 && bars != 2 && bars != 4 && bars != 8 && bars != 16)
            || getState() != State::empty || getLength() != 0)
        {
            notice.store(Notice::invalidSetting);
            return false;
        }
        autoStopBars.store(bars);
        return true;
    }
    TimeSignature getTimeSignature() const
    {
        const int signature = timeSignature.load();
        return { signature >> 8, signature & 0xff };
    }
    bool setTimeSignature(int beats, int unit)
    {
        if (beats < 1 || beats > 16 || (unit != 1 && unit != 2 && unit != 4 && unit != 8 && unit != 16)
            || getState() == State::countingIn)
        {
            notice.store(Notice::invalidSetting);
            return false;
        }
        timeSignature.store((beats << 8) | unit);
        return true;
    }

    std::atomic<float> inputGain { 1.0f }, liveGain { 1.0f }, loopGain { 1.0f }, outputGain { 0.7f };
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f };
    std::atomic<InputMode> inputMode { InputMode::monoFirst };
    std::atomic<double> bpm { 120.0 };
    std::atomic<bool> metronome { true };
    std::atomic<int> countInBars { 0 }, autoStopBars { 0 };

private:
    struct Layer
    {
        std::array<std::vector<float>, 2> audio;
        LayerControls controls;
        std::array<float, 2> smoothed { 1.0f, 1.0f };
    };
    struct Event { Command command = Command::record; int layer = -1; };
    enum class UndoKind { none, added, replaced };

    void resetClock()
    {
        beatPhase = 0.0;
        beat = 0;
        clickRemaining = 0;
        clockStarted = false;
    }

    float tick(double tempo, TimeSignature signature)
    {
        if (signature.beats != clockSignature.beats || signature.unit != clockSignature.unit)
        {
            clockSignature = signature;
            resetClock();
        }
        const double rate = sampleRate.load();
        const double beatFrames = rate * 60.0 / tempo * 4.0 / signature.unit;
        if (!clockStarted || beatPhase >= beatFrames)
        {
            if (clockStarted)
            {
                beatPhase = std::fmod(beatPhase, beatFrames);
                beat = (beat + 1) % signature.beats;
            }
            clockStarted = true;
            clickRemaining = static_cast<int>(std::max(1.0, rate * 0.02));
            clickLength = clickRemaining;
            clickPhase = 0.0;
        }
        float value = 0.0f;
        if (clickRemaining > 0)
        {
            const double envelope = static_cast<double>(clickRemaining) / clickLength;
            value = metronome.load(std::memory_order_relaxed)
                ? static_cast<float>(std::cos(clickPhase) * envelope * 0.12) : 0.0f;
            clickPhase += 6.283185307179586 * (beat == 0 ? 1500.0 : 1000.0) / rate;
            --clickRemaining;
        }
        beatPhase += 1.0;
        return value;
    }

    void reset()
    {
        state.store(State::empty);
        length.store(0);
        position.store(0);
        recordingFrames = 0;
        layerCount.store(0);
        recordingLayer.store(-1);
        remainingCountBeats.store(0);
        hasUndo.store(false);
        undoKind = UndoKind::none;
        undoApplied = false;
        for (auto& layer : layers)
        {
            layer.controls.volume.store(1.0f);
            layer.controls.pan.store(0.0f);
            layer.controls.active.store(true);
            layer.controls.solo.store(false);
            layer.smoothed = { 1.0f, 1.0f };
        }
        resetClock();
    }

    void finishFirst()
    {
        layers[0].audio.swap(take);
        layerCount.store(1);
        recordingLayer.store(-1);
        position.store(0);
        recordingFrames = 0;
        state.store(State::playing);
    }

    std::size_t framesForBars(int bars, double tempo, TimeSignature signature) const
    {
        if (bars <= 0)
            return 0;
        const double frames = sampleRate.load() * 60.0 / tempo * 4.0
            / signature.unit * static_cast<double>(bars) * signature.beats;
        return static_cast<std::size_t>(std::max(1.0, std::round(frames)));
    }

    void finishLayer()
    {
        const int target = recordingLayer.load();
        layers[static_cast<std::size_t>(target)].audio.swap(take);
        undoLayer = target;
        undoKind = target == layerCount.load() ? UndoKind::added : UndoKind::replaced;
        if (undoKind == UndoKind::added)
            layerCount.store(target + 1);
        hasUndo.store(true);
        undoApplied = false;
        recordingLayer.store(-1);
        state.store(State::playing);
    }

    void drainCommands()
    {
        auto read = readIndex.load(std::memory_order_relaxed);
        while (read != writeIndex.load(std::memory_order_acquire))
        {
            handle(commands[read]);
            read = (read + 1) % commands.size();
            readIndex.store(read, std::memory_order_release);
        }
    }

    void handle(Event event)
    {
        const auto current = state.load();
        const auto len = length.load();
        if (event.command == Command::clear)
            reset();
        else if (event.command == Command::stop)
        {
            if (current == State::recording && len > 0)
                finishFirst();
            recordingLayer.store(-1);
            remainingCountBeats.store(0);
            recordingFrames = 0;
            state.store(len > 0 ? State::stopped : State::empty);
            position.store(0);
            resetClock();
        }
        else if (event.command == Command::undo && hasUndo.load()
                 && (current == State::playing || current == State::stopped))
        {
            if (undoKind == UndoKind::added)
                layerCount.store(layerCount.load() + (undoApplied ? 1 : -1));
            else if (undoKind == UndoKind::replaced)
                layers[static_cast<std::size_t>(undoLayer)].audio.swap(take);
            undoApplied = !undoApplied;
        }
        else if (event.command == Command::rerecord)
        {
            if ((current != State::playing && current != State::stopped)
                || event.layer < 0 || event.layer >= layerCount.load())
            {
                notice.store(Notice::invalidCommand);
                return;
            }
            recordingLayer.store(event.layer);
            mergeTake = false;
            if (current == State::stopped) { position.store(0); resetClock(); }
            state.store(State::waiting);
        }
        else if (event.command == Command::removeLayer)
        {
            const int count = layerCount.load();
            if ((current != State::playing && current != State::stopped)
                || event.layer < 0 || event.layer >= count)
            {
                notice.store(Notice::invalidCommand);
                return;
            }
            undoKind = UndoKind::none;
            hasUndo.store(false);
            undoApplied = false;
            for (int index = event.layer; index < count - 1; ++index)
            {
                auto& destination = layers[static_cast<std::size_t>(index)];
                auto& source = layers[static_cast<std::size_t>(index + 1)];
                destination.audio.swap(source.audio);
                destination.controls.volume.store(source.controls.volume.load());
                destination.controls.pan.store(source.controls.pan.load());
                destination.controls.active.store(source.controls.active.load());
                destination.controls.solo.store(source.controls.solo.load());
                destination.smoothed = source.smoothed;
            }
            layerCount.store(count - 1);
            if (count == 1) reset();
        }
        else if (event.command == Command::record)
        {
            if (current == State::empty && capacity > 0)
            {
                recordingLayer.store(0);
                resetClock();
                recordingFrames.store(framesForBars(autoStopBars.load(), std::clamp(bpm.load(), 30.0, 300.0),
                                                    getTimeSignature()));
                const int bars = countInBars.load();
                if (bars == 1 || bars == 2 || bars == 4)
                {
                    countBpm = std::clamp(bpm.load(), 30.0, 300.0);
                    countSignature = getTimeSignature();
                    countFrames = framesForBars(bars, countBpm, countSignature);
                    remainingCountBeats.store(bars * countSignature.beats);
                    state.store(State::countingIn);
                }
                else
                    state.store(State::recording);
            }
            else if (current == State::countingIn)
                reset();
            else if (current == State::recording && len > 0)
                finishFirst();
            else if (current == State::stopped && len > 0)
            {
                position.store(0);
                resetClock();
                state.store(State::playing);
            }
            else if (current == State::playing)
            {
                const int count = layerCount.load();
                if (!layeredLoop.load())
                {
                    recordingLayer.store(0);
                    mergeTake = true;
                    state.store(State::waiting);
                    return;
                }
                if (count == maxLayers)
                {
                    notice.store(Notice::layerLimit);
                    return;
                }
                auto& controls = layers[static_cast<std::size_t>(count)].controls;
                controls.volume.store(1.0f);
                controls.pan.store(0.0f);
                controls.active.store(true);
                controls.solo.store(false);
                recordingLayer.store(count);
                mergeTake = false;
                state.store(State::waiting);
            }
            else if (current == State::overdubbing || current == State::waiting)
            {
                recordingLayer.store(-1);
                state.store(State::playing);
            }
        }
    }

    std::array<Layer, maxLayers> layers;
    std::array<std::vector<float>, 2> take;
    std::array<Event, 32> commands{};
    std::atomic<std::size_t> writeIndex { 0 }, readIndex { 0 };
    std::atomic<State> state { State::empty };
    std::atomic<std::size_t> length { 0 }, position { 0 };
    std::atomic<int> layerCount { 0 }, recordingLayer { -1 }, remainingCountBeats { 0 };
    std::atomic<bool> hasUndo { false };
    std::atomic<bool> layeredLoop { true };
    std::atomic<int> timeSignature { (4 << 8) | 4 };
    std::atomic<Notice> notice { Notice::none };
    std::atomic<std::size_t> capacity { 0 };
    std::size_t countFrames = 0;
    std::atomic<std::size_t> recordingFrames { 0 };
    std::atomic<double> sampleRate { 0.0 };
    UndoKind undoKind = UndoKind::none;
    int undoLayer = -1;
    bool undoApplied = false, clockStarted = false;
    bool mergeTake = false;
    TimeSignature countSignature { 4, 4 }, clockSignature { 4, 4 };
    double beatPhase = 0.0, clickPhase = 0.0, countBpm = 120.0;
    int beat = 0, clickRemaining = 0, clickLength = 1;
};
