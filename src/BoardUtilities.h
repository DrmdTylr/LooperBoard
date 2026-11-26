#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

class BoardUtilities
{
public:
    std::atomic<float> inputDb { 0 }, gateDb { -50 }, transpose { 0 }, doublerMs { 15 }, outputDb { 0 };
    std::atomic<bool> gate { false }, doubler { false }, tuner { true };
    std::atomic<float> inputPeak { 0 }, outputPeak { 0 };

    void prepare(double rate)
    {
        sampleRate = rate;
        window = static_cast<int>(rate * .035);
        for (auto& channel : history) channel.assign(static_cast<std::size_t>(rate * .06) + 4, 0.0f);
        cursor = postCursor = 0;
        divider = 0;
        pitchPhase = 0;
        envelope = 0;
        gateGain = 1;
        pitchMix = doublerMix = 0;
        inGain = gain(inputDb.load());
        outGain = gain(outputDb.load());
        tunerGeneration.fetch_add(1, std::memory_order_release);
        decimation.store(std::max(1, static_cast<int>(rate / 6000.0)));
        tunerRate.store(rate / decimation.load());
        smoothing = static_cast<float>(std::min(1.0, 200.0 / rate));
    }

    void before(float* left, float* right, int frames)
    {
        const auto gainTarget = gain(inputDb.load());
        const auto threshold = gain(gateDb.load());
        const auto shift = transpose.load();
        const auto speed = (std::exp2(shift / 12.0f) - 1.0f) / static_cast<float>(window);
        const bool useGate = gate.load(), tune = tuner.load();
        float peak = 0;
        for (int i = 0; i < frames; ++i)
        {
            inGain += smoothing * (gainTarget - inGain);
            float l = left[i] * inGain, r = right[i] * inGain;
            const auto level = std::max(std::abs(l), std::abs(r));
            peak = std::max(peak, level);
            if (tune && ++divider >= decimation.load(std::memory_order_relaxed))
            {
                divider = 0;
                pushTuner(std::abs(l - r) > std::abs(l + r) ? l : (l + r) * .5f);
            }
            envelope = std::max(level, envelope * (1.0f - smoothing * .08f));
            gateGain += smoothing * ((!useGate || envelope >= threshold ? 1.0f : 0.0f) - gateGain);
            l *= gateGain;
            r *= gateGain;
            history[0][cursor] = l;
            history[1][cursor] = r;
            pitchMix += smoothing * ((shift == 0 ? 0.0f : 1.0f) - pitchMix);
            if (pitchMix > .0001f)
            {
                pitchPhase -= speed;
                pitchPhase -= std::floor(pitchPhase);
                const auto second = pitchPhase < .5f ? pitchPhase + .5f : pitchPhase - .5f;
                const auto blend = 1.0f - std::abs(2.0f * pitchPhase - 1.0f);
                const auto a = 1.0f + pitchPhase * static_cast<float>(window);
                const auto b = 1.0f + second * static_cast<float>(window);
                l += pitchMix * ((read(0, a) * blend + read(0, b) * (1.0f - blend)) - l);
                r += pitchMix * ((read(1, a) * blend + read(1, b) * (1.0f - blend)) - r);
            }
            left[i] = l;
            right[i] = r;
            advance();
        }
        inputPeak.store(peak);
    }

    void after(float* left, float* right, int frames, const float* click = nullptr)
    {
        const auto gainTarget = gain(outputDb.load());
        const auto delay = std::max(1.0f, doublerMs.load() * static_cast<float>(sampleRate * .001));
        const auto mix = doubler.load() ? .34f : 0.0f;
        float peak = 0;
        for (int i = 0; i < frames; ++i)
        {
            const auto l = history[2][postCursor] = std::isfinite(left[i]) ? left[i] : 0.0f;
            const auto r = history[3][postCursor] = std::isfinite(right[i]) ? right[i] : 0.0f;
            doublerMix += smoothing * (mix - doublerMix);
            outGain += smoothing * (gainTarget - outGain);
            const auto tick = click == nullptr ? 0.0f : click[i];
            left[i] = ((1.0f - doublerMix) * l + doublerMix * read(3, delay) + tick) * outGain;
            right[i] = ((1.0f - doublerMix) * r + doublerMix * read(2, delay * .79f) + tick) * outGain;
            if (++postCursor == history[2].size()) postCursor = 0;
            peak = std::max({ peak, std::abs(left[i]), std::abs(right[i]) });
        }
        outputPeak.store(peak);
    }

    float readPitch()
    {
        auto readAt = tunerRead.load(std::memory_order_relaxed);
        const auto end = tunerWrite.load(std::memory_order_acquire);
        const auto generation = tunerGeneration.load(std::memory_order_acquire);
        if (!tuner.load() || generation != readGeneration)
        {
            tunerRead.store(end, std::memory_order_release);
            tunerCount = 0;
            readGeneration = generation;
            return 0;
        }
        int samples = 0;
        while (readAt != end)
        {
            tunerFrame[tunerCount++ % tunerFrame.size()] = tunerQueue[readAt];
            readAt = (readAt + 1) % tunerQueue.size();
            ++samples;
        }
        tunerRead.store(readAt, std::memory_order_release);
        if (samples == 0 || tunerCount < tunerFrame.size()) return 0;
        std::array<float, 1024> ordered{};
        for (std::size_t i = 0; i < ordered.size(); ++i)
            ordered[i] = tunerFrame[(tunerCount + i) % tunerFrame.size()];
        return estimatePitch(ordered, tunerRate.load());
    }

    static float estimatePitch(const std::array<float, 1024>& frame, double rate)
    {
        const auto lastLag = std::min(160, static_cast<int>(rate / 48.0));
        const auto firstLag = std::max(2, static_cast<int>(rate / 1400.0));
        std::array<float, 162> difference{};
        float strength = 0;
        constexpr int span = 850;
        for (int i = 0; i < span; ++i) strength += frame[static_cast<std::size_t>(i)] * frame[static_cast<std::size_t>(i)];
        if (strength < span * .000025f || firstLag >= lastLag) return 0;
        float sum = 0;
        for (int lag = 1; lag <= lastLag; ++lag)
        {
            float score = 0;
            for (int i = 0; i < span; ++i)
            {
                const auto delta = frame[static_cast<std::size_t>(i)] - frame[static_cast<std::size_t>(i + lag)];
                score += delta * delta;
            }
            sum += score;
            difference[static_cast<std::size_t>(lag)] = sum > 0 ? score * static_cast<float>(lag) / sum : 1.0f;
        }
        for (int lag = firstLag; lag < lastLag; ++lag)
        {
            const auto index = static_cast<std::size_t>(lag);
            const auto middle = difference[index];
            if (middle > .17f || middle > difference[index - 1] || middle >= difference[index + 1]) continue;
            const auto offset = .5f * (difference[index - 1] - difference[index + 1])
                / (difference[index - 1] + difference[index + 1] - 2.0f * middle);
            return static_cast<float>(rate / (static_cast<double>(lag) + offset));
        }
        return 0;
    }

private:
    static float gain(float db) { return std::pow(10.0f, db / 20.0f); }
    float read(std::size_t channel, float delay) const
    {
        const auto& data = history[channel];
        const auto whole = static_cast<std::size_t>(delay);
        const auto fraction = delay - static_cast<float>(whole);
        const auto current = channel < 2 ? cursor : postCursor;
        const auto index = (current + data.size() - whole) % data.size();
        return data[index] * (1.0f - fraction) + data[(index + data.size() - 1) % data.size()] * fraction;
    }
    void advance() { if (++cursor == history[0].size()) cursor = 0; }
    void pushTuner(float value)
    {
        const auto position = tunerWrite.load(std::memory_order_relaxed);
        const auto next = (position + 1) % tunerQueue.size();
        if (next == tunerRead.load(std::memory_order_acquire)) return;
        tunerQueue[position] = value;
        tunerWrite.store(next, std::memory_order_release);
    }

    std::array<std::vector<float>, 4> history;
    std::size_t cursor = 0, postCursor = 0, tunerCount = 0;
    int window = 0, divider = 0;
    float pitchPhase = 0, pitchMix = 0, doublerMix = 0, envelope = 0, gateGain = 1, inGain = 1, outGain = 1, smoothing = 0;
    double sampleRate = 48000;
    std::array<float, 4096> tunerQueue{};
    std::array<float, 1024> tunerFrame{};
    std::atomic<std::size_t> tunerWrite { 0 }, tunerRead { 0 };
    std::atomic<int> decimation { 8 };
    std::atomic<double> tunerRate { 6000 };
    std::atomic<unsigned> tunerGeneration { 0 };
    unsigned readGeneration = 0;
};
