#include <juce_audio_utils/juce_audio_utils.h>
#include "BinaryData.h"
#include "LooperEngine.h"
#include "LoopAudioFile.h"
#include "PedalHost.h"
#include "BoardPreset.h"
#include "AudioSettings.h"
#include "VintageLook.h"
#include <cstring>
#include <limits>

namespace
{
const auto background = juce::Colour(0xff241a14);
const auto panel = juce::Colour(0xff493327);
const auto muted = juce::Colour(0xffd0bfa5);
const auto accent = juce::Colour(0xffe6b678);
const auto cream = juce::Colour(0xfff5efdf);
const auto layerInk = juce::Colour(0xff40392e);
const auto layerMuted = juce::Colour(0xff766850);
const auto layerSurface = juce::Colour(0xfff8f2e3);
const auto layerAccent = juce::Colour(0xff9a7541);

class LayerRow final : public juce::Component
{
public:
    LayerRow(LooperEngine& owner, int layer, std::function<void(int)> rerecord, std::function<void(int)> remove)
        : engine(owner), index(layer)
    {
        auto& controls = engine.getLayerControls(index);
        for (auto* slider : { &volume, &pan })
        {
            addAndMakeVisible(slider);
            slider->setSliderStyle(juce::Slider::LinearHorizontal);
            slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 46, 22);
            slider->setColour(juce::Slider::thumbColourId, accent);
            slider->setColour(juce::Slider::trackColourId, accent);
            slider->setColour(juce::Slider::textBoxTextColourId, layerInk);
        }
        volume.setName("Layer " + juce::String(index + 1) + " volume");
        volume.setRange(0.0, 1.0, 0.01);
        volume.setValue(1.0);
        pan.setName("Layer " + juce::String(index + 1) + " pan");
        pan.setRange(-1.0, 1.0, 0.01);
        pan.setDoubleClickReturnValue(true, 0.0);
        volume.onValueChange = [this] { engine.getLayerControls(index).volume.store(static_cast<float>(volume.getValue())); };
        pan.onValueChange = [this] { engine.getLayerControls(index).pan.store(static_cast<float>(pan.getValue())); };
        for (auto* button : { &active, &solo, &replace, &removeButton })
        {
            addAndMakeVisible(button);
            button->setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe5dcc6));
            button->setColour(juce::TextButton::buttonOnColourId, layerAccent);
            button->setColour(juce::TextButton::textColourOffId, layerInk);
            button->setColour(juce::TextButton::textColourOnId, cream);
        }
        active.setClickingTogglesState(true);
        active.setToggleState(controls.active.load(), juce::dontSendNotification);
        solo.setClickingTogglesState(true);
        active.onClick = [this] { engine.getLayerControls(index).active.store(active.getToggleState()); };
        solo.onClick = [this] { engine.getLayerControls(index).solo.store(solo.getToggleState()); };
        replace.onClick = [rerecord, layer] { rerecord(layer); };
        removeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffa697));
        removeButton.onClick = [remove, layer] { remove(layer); };
    }

    void refresh()
    {
        const bool exists = index < engine.getLayerCount();
        capturing = engine.getRecordingLayer() == index;
        setVisible(exists);
        auto& controls = engine.getLayerControls(index);
        active.setEnabled(exists);
        solo.setEnabled(exists);
        volume.setEnabled(exists);
        pan.setEnabled(exists);
        active.setToggleState(controls.active.load(), juce::dontSendNotification);
        active.setButtonText(controls.active.load() ? "ON" : "OFF");
        solo.setToggleState(controls.solo.load(), juce::dontSendNotification);
        if (!volume.isMouseButtonDown())
            volume.setValue(controls.volume.load(), juce::dontSendNotification);
        if (!pan.isMouseButtonDown())
            pan.setValue(controls.pan.load(), juce::dontSendNotification);
        const auto state = engine.getState();
        replace.setEnabled(exists && (state == LooperEngine::State::playing || state == LooperEngine::State::stopped));
        removeButton.setEnabled(replace.isEnabled());
        repaint();
    }

    void resized() override
    {
        auto row = getLocalBounds().reduced(12, 6);
        row.removeFromLeft(108);
        removeButton.setBounds(row.removeFromRight(80).withSizeKeepingCentre(76, 34));
        row.removeFromRight(8);
        replace.setBounds(row.removeFromRight(112).withSizeKeepingCentre(108, 34));
        row.removeFromRight(8);
        solo.setBounds(row.removeFromRight(64).withSizeKeepingCentre(60, 34));
        active.setBounds(row.removeFromRight(64).withSizeKeepingCentre(60, 34));
        const auto width = row.getWidth() / 2;
        volume.setBounds(row.removeFromLeft(width).withTrimmedTop(17).reduced(5, 0));
        pan.setBounds(row.withTrimmedTop(17).reduced(5, 0));
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(index % 2 == 0 ? layerSurface : juce::Colour(0xfff3ecdb));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.0f, 2.0f), 8.0f);
        g.setColour(layerInk);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText("Layer " + juce::String(index + 1), 14, 12, 96, 23, juce::Justification::left);
        g.setFont(juce::FontOptions(11.0f));
        g.setColour(capturing ? juce::Colour(0xffa8483f) : layerMuted);
        g.drawText(capturing ? "TAKE / QUEUED" : "RECORDED",
                   14, 34, 100, 18, juce::Justification::left);
        g.drawText("VOLUME", volume.getX() + 4, 7, volume.getWidth(), 16, juce::Justification::left);
        g.drawText("PAN   L  <  0  >  R", pan.getX() + 4, 7, pan.getWidth(), 16, juce::Justification::left);
    }

private:
    LooperEngine& engine;
    int index;
    bool capturing = false;
    juce::Slider volume, pan;
    juce::TextButton active { "ON" }, solo { "SOLO" }, replace { "RERECORD" }, removeButton { "DELETE" };
};

class SettingsPanel final : public juce::Component, private juce::Timer
{
public:
    SettingsPanel(LooperEngine& owner, juce::AudioDeviceManager& devices, std::function<void(bool)> saveAudio, bool showAudio = false)
        : engine(owner), deviceManager(devices), saveAudioSettingsOnClose(std::move(saveAudio)),
          selector(devices, 1, 2, 1, 2, true, false, false, false)
    {
        initialInputMode = engine.inputMode.load();
        initialSetup = devices.getAudioDeviceSetup();
        initialDeviceType = devices.getCurrentAudioDeviceType();
        if (auto state = devices.createStateXml()) initialDeviceState = state->toString();
        addAndMakeVisible(tabs);
        tabs.addTab("Audio", cream, &audioPage, false);
        tabs.addTab("Looper", cream, &looperPage, false);
        tabs.setColour(juce::TabbedComponent::backgroundColourId, cream);
        tabs.setColour(juce::TabbedComponent::outlineColourId, juce::Colour(0xffd2c5ab));
        tabs.setColour(juce::TabbedButtonBar::tabTextColourId, layerMuted);
        tabs.setColour(juce::TabbedButtonBar::frontTextColourId, layerInk);
        audioPage.addAndMakeVisible(audioNote);
        audioPage.addAndMakeVisible(inputLabel);
        audioPage.addAndMakeVisible(inputMode);
        audioPage.addAndMakeVisible(audioViewport);
        audioViewport.setViewedComponent(&selector, false);
        audioViewport.setScrollBarsShown(true, false);
        selector.setItemHeight(28);
        selector.setSize(700, 820);
        inputLabel.setText("Input mode", juce::dontSendNotification);
        inputMode.setName("Input mode");
        inputMode.setColour(juce::ComboBox::backgroundColourId, layerSurface);
        inputMode.setColour(juce::ComboBox::textColourId, layerInk);
        inputMode.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xffd2c5ab));
        inputMode.addItem("Mono: first input (centered)", 1);
        inputMode.addItem("Mono: second input (centered)", 2);
        inputMode.addItem("Stereo: two selected inputs", 3);
        inputMode.setSelectedId(engine.inputMode.load() == LooperEngine::InputMode::stereo ? 3
            : engine.inputMode.load() == LooperEngine::InputMode::monoSecond ? 2 : 1, juce::dontSendNotification);
        inputMode.onChange = [this]
        {
            engine.inputMode.store(inputMode.getSelectedId() == 3 ? LooperEngine::InputMode::stereo
                : inputMode.getSelectedId() == 2 ? LooperEngine::InputMode::monoSecond : LooperEngine::InputMode::monoFirst);
        };

        for (auto* control : std::array<juce::Component*, 14> { &layeredLoop, &modeNote, &metronome, &bpm,
                 &countIn, &beats, &unit, &loopLength, &tempoNote, &bpmLabel, &countLabel, &loopLengthLabel,
                 &signatureLabel, &slash })
            looperPage.addAndMakeVisible(control);
        for (auto* toggle : { &layeredLoop, &metronome })
        {
            toggle->setColour(juce::ToggleButton::textColourId, layerInk);
            toggle->setColour(juce::ToggleButton::tickColourId, layerAccent);
            toggle->setColour(juce::ToggleButton::tickDisabledColourId, layerMuted);
        }
        layeredLoop.setToggleState(engine.isLayeredLoop(), juce::dontSendNotification);
        layeredLoop.onClick = [this]
        {
            if (!engine.setLayeredLoop(layeredLoop.getToggleState()))
                layeredLoop.setToggleState(engine.isLayeredLoop(), juce::dontSendNotification);
        };
        metronome.setToggleState(engine.metronome.load(), juce::dontSendNotification);
        metronome.onClick = [this] { engine.metronome.store(metronome.getToggleState()); };
        for (auto* slider : { &bpm, &beats })
        {
            slider->setSliderStyle(juce::Slider::IncDecButtons);
            slider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 64, 28);
            slider->setColour(juce::Slider::textBoxTextColourId, layerInk);
            slider->setColour(juce::Slider::textBoxBackgroundColourId, layerSurface);
            slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xffd2c5ab));
        }
        bpm.setName("Metronome BPM");
        bpm.setRange(30.0, 300.0, 1.0);
        bpm.setValue(engine.bpm.load(), juce::dontSendNotification);
        bpm.onValueChange = [this] { engine.bpm.store(bpm.getValue()); };
        countIn.setName("First recording count-in");
        countIn.addItem("Off", 1);
        countIn.addItem("1 bar", 2);
        countIn.addItem("2 bars", 3);
        countIn.addItem("4 bars", 4);
        const auto bars = engine.countInBars.load();
        countIn.setSelectedId(bars == 4 ? 4 : bars == 2 ? 3 : bars == 1 ? 2 : 1, juce::dontSendNotification);
        for (auto* combo : { &countIn, &unit, &loopLength })
        {
            combo->setColour(juce::ComboBox::backgroundColourId, layerSurface);
            combo->setColour(juce::ComboBox::textColourId, layerInk);
            combo->setColour(juce::ComboBox::outlineColourId, juce::Colour(0xffd2c5ab));
        }
        countIn.onChange = [this]
        {
            const int values[] { 0, 0, 1, 2, 4 };
            engine.countInBars.store(values[countIn.getSelectedId()]);
        };
        loopLength.setName("Auto-stop loop length");
        int loopLengthId = 1;
        for (int barCount : { 0, 1, 2, 4, 8, 16 })
            loopLength.addItem(barCount == 0 ? "Off" : juce::String(barCount)
                + (barCount == 1 ? " bar" : " bars"), loopLengthId++);
        const auto loopBars = engine.autoStopBars.load();
        loopLength.setSelectedId(loopBars == 16 ? 6 : loopBars == 8 ? 5 : loopBars == 4 ? 4
            : loopBars == 2 ? 3 : loopBars == 1 ? 2 : 1, juce::dontSendNotification);
        loopLength.onChange = [this]
        {
            const int values[] { 0, 1, 2, 4, 8, 16 };
            if (!engine.setAutoStopBars(values[loopLength.getSelectedId() - 1]))
            {
                const auto currentBars = engine.autoStopBars.load();
                loopLength.setSelectedId(currentBars == 16 ? 6 : currentBars == 8 ? 5 : currentBars == 4 ? 4
                    : currentBars == 2 ? 3 : currentBars == 1 ? 2 : 1, juce::dontSendNotification);
            }
        };
        const auto signature = engine.getTimeSignature();
        beats.setName("Beats per bar");
        beats.setRange(1.0, 16.0, 1.0);
        beats.setValue(signature.beats, juce::dontSendNotification);
        unit.setName("Time signature beat unit");
        for (int value : { 1, 2, 4, 8, 16 }) unit.addItem(juce::String(value), value);
        unit.setSelectedId(signature.unit, juce::dontSendNotification);
        beats.onValueChange = [this] { changeSignature(); };
        unit.onChange = [this] { changeSignature(); };
        bpmLabel.setText("BPM (quarter notes)", juce::dontSendNotification);
        countLabel.setText("First recording count-in", juce::dontSendNotification);
        loopLengthLabel.setText("Stop recording after", juce::dontSendNotification);
        signatureLabel.setText("Time signature", juce::dontSendNotification);
        slash.setText("/", juce::dontSendNotification);
        tempoNote.setText("Each click follows the selected beat unit; the first beat of each bar is accented.\n"
                         "Tempo and time signature changes do not stretch or quantize existing loops.",
                         juce::dontSendNotification);
        for (auto* label : { &audioNote, &inputLabel, &modeNote, &tempoNote, &bpmLabel,
                            &countLabel, &loopLengthLabel, &signatureLabel, &slash })
            label->setColour(juce::Label::textColourId, layerInk);
        for (auto* label : { &modeNote, &tempoNote, &audioNote })
            label->setColour(juce::Label::textColourId, layerMuted);
        closeSettings.setButtonText("×");
        closeSettings.setColour(juce::TextButton::buttonColourId, layerSurface);
        closeSettings.setColour(juce::TextButton::textColourOffId, layerInk);
        closeSettings.onClick = [this]
        {
            if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
                window->exitModalState(0);
        };
        addAndMakeVisible(settingsEyebrow);
        addAndMakeVisible(settingsHeading);
        addAndMakeVisible(closeSettings);
        settingsEyebrow.setText("MAKE IT YOURS", juce::dontSendNotification);
        settingsEyebrow.setColour(juce::Label::textColourId, layerMuted);
        settingsHeading.setText("Settings", juce::dontSendNotification);
        settingsHeading.setColour(juce::Label::textColourId, layerInk);
        refresh();
        if (!showAudio && engine.getState() != LooperEngine::State::empty) tabs.setCurrentTabIndex(1);
        setSize(650, 650);
        startTimerHz(10);
    }

    ~SettingsPanel() override
    {
        stopTimer();
        audioViewport.setViewedComponent(nullptr, false);
        const auto state = deviceManager.createStateXml();
        if (engine.inputMode.load() != initialInputMode
            || (state != nullptr ? state->toString() : juce::String()) != initialDeviceState)
            saveAudioSettingsOnClose(initialSetup != deviceManager.getAudioDeviceSetup()
                || initialDeviceType != deviceManager.getCurrentAudioDeviceType());
    }

    void resized() override
    {
        settingsEyebrow.setBounds(24, 16, 200, 16);
        settingsHeading.setBounds(24, 34, 300, 43);
        closeSettings.setBounds(getWidth() - 69, 17, 45, 45);
        tabs.setBounds(14, 88, getWidth() - 28, getHeight() - 100);
        const auto width = getWidth() - 40;
        audioNote.setBounds(20, 10, width, 40);
        inputLabel.setBounds(20, 58, 170, 28);
        inputMode.setBounds(200, 58, width - 180, 28);
        audioViewport.setBounds(10, 98, getWidth() - 20, getHeight() - 140);
        selector.setSize(audioViewport.getWidth() - audioViewport.getScrollBarThickness(), selector.getHeight());
        layeredLoop.setBounds(24, 24, width, 30);
        modeNote.setBounds(24, 60, width, 64);
        metronome.setBounds(24, 140, width, 30);
        bpmLabel.setBounds(24, 193, 200, 28);
        bpm.setBounds(240, 193, 150, 28);
        signatureLabel.setBounds(24, 244, 200, 28);
        beats.setBounds(240, 244, 130, 28);
        slash.setBounds(376, 244, 24, 28);
        unit.setBounds(406, 244, 88, 28);
        countLabel.setBounds(24, 295, 200, 28);
        countIn.setBounds(240, 295, 254, 28);
        loopLengthLabel.setBounds(24, 346, 200, 28);
        loopLength.setBounds(240, 346, 254, 28);
        tempoNote.setBounds(24, 401, width, 72);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(cream);
        g.setColour(juce::Colour(0xffd2c5ab));
        for (int y : { 134, 184, 235, 286, 336, 386 })
            g.drawHorizontalLine(y, 24.0f, static_cast<float>(getWidth() - 24));
    }

private:
    void changeSignature()
    {
        if (!engine.setTimeSignature(static_cast<int>(beats.getValue()), unit.getSelectedId()))
        {
            const auto signature = engine.getTimeSignature();
            beats.setValue(signature.beats, juce::dontSendNotification);
            unit.setSelectedId(signature.unit, juce::dontSendNotification);
        }
    }

    void timerCallback() override { refresh(); }

    void refresh()
    {
        const auto state = engine.getState();
        const bool empty = state == LooperEngine::State::empty && engine.getLength() == 0;
        selector.setEnabled(empty);
        audioNote.setText(empty ? "Choose an audio device and input/output channels below."
            : "Audio device settings are locked to protect your recording. Clear the loop to change them.",
            juce::dontSendNotification);
        inputMode.setEnabled(state == LooperEngine::State::empty || state == LooperEngine::State::playing
                             || state == LooperEngine::State::stopped);
        layeredLoop.setEnabled(empty);
        modeNote.setText("On: up to eight independently editable layers. Off: classic overdubs into Layer 1.\n"
                         "Choose the mode before recording; clear the loop to change it.",
                         juce::dontSendNotification);
        bpm.setEnabled(state != LooperEngine::State::countingIn);
        beats.setEnabled(state != LooperEngine::State::countingIn);
        unit.setEnabled(state != LooperEngine::State::countingIn);
        countIn.setEnabled(empty);
        loopLength.setEnabled(empty);
    }

    LooperEngine& engine;
    juce::AudioDeviceManager& deviceManager;
    std::function<void(bool)> saveAudioSettingsOnClose;
    LooperEngine::InputMode initialInputMode;
    juce::AudioDeviceManager::AudioDeviceSetup initialSetup;
    juce::String initialDeviceType;
    juce::String initialDeviceState;
    juce::Component audioPage, looperPage;
    juce::AudioDeviceSelectorComponent selector;
    juce::Viewport audioViewport;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::Label audioNote, inputLabel, modeNote, tempoNote, bpmLabel, countLabel, loopLengthLabel, signatureLabel, slash,
        settingsEyebrow, settingsHeading;
    juce::ComboBox inputMode, countIn, unit, loopLength;
    juce::ToggleButton layeredLoop { "Layered Loop" }, metronome { "Metronome" };
    juce::Slider bpm, beats;
    juce::TextButton closeSettings;
};

struct PedalTemplate
{
    juce::String id, category, name;
    std::vector<juce::String> knobs;
};

const std::array<PedalTemplate, 24>& getPedalTemplates()
{
    static const std::array<PedalTemplate, 24> templates {{
        { "compressor", "Dynamics", "Compressor", { "Blend", "Compression", "Tone", "Volume" } },
        { "gate", "Dynamics", "Noise Gate", { "Threshold", "Release", "Hold", "Level" } },
        { "limiter", "Dynamics", "Limiter", { "Threshold", "Release", "Ceiling" } },
        { "grit", "Gain", "Overdrive", { "Drive", "Tone", "Level" } },
        { "distortion", "Gain", "Distortion", { "Gain", "Tone", "Level" } },
        { "fuzz", "Gain", "Fuzz", { "Fuzz", "Tone", "Volume" } },
        { "boost", "Gain", "Boost", { "Boost", "Tone", "Level" } },
        { "wah", "Filter", "Wah", { "Position", "Range", "Level" } },
        { "envelope", "Filter", "Envelope Filter", { "Sensitivity", "Filter", "Mix" } },
        { "eq", "Filter", "Equalizer", { "Bass", "Mid", "Treble", "Level" } },
        { "octave", "Pitch", "Octaver", { "Octave", "Blend", "Direct", "Level" } },
        { "pitch", "Pitch", "Pitch Shift", { "Shift", "Fine", "Blend", "Level" } },
        { "harmonizer", "Pitch", "Harmonizer", { "Key", "Interval", "Mix", "Level" } },
        { "chorus", "Modulation", "Chorus", { "Rate", "Depth", "Tone", "Mix" } },
        { "flanger", "Modulation", "Flanger", { "Rate", "Depth", "Feedback", "Mix" } },
        { "phaser", "Modulation", "Phaser", { "Rate", "Depth", "Feedback", "Mix" } },
        { "tremolo", "Modulation", "Tremolo", { "Rate", "Depth", "Shape", "Level" } },
        { "tide", "Delay", "Digital Delay", { "Time", "Feedback", "Mix", "Tone" } },
        { "tape", "Delay", "Tape Echo", { "Time", "Age", "Feedback", "Mix" } },
        { "halo", "Reverb", "Room Reverb", { "Decay", "Tone", "Mix" } },
        { "spring", "Reverb", "Spring Reverb", { "Dwell", "Tone", "Mix" } },
        { "shimmer", "Reverb", "Shimmer", { "Decay", "Shimmer", "Mix" } },
        { "amp", "Amp & Cab", "Amp Sim", { "Gain", "Bass", "Treble", "Level" } },
        { "cab", "Amp & Cab", "Cab Sim", { "Mic", "Room", "Air", "Level" } }
    }};
    return templates;
}

const PedalTemplate* findPedalTemplate(const juce::String& id)
{
    const auto& templates = getPedalTemplates();
    const auto found = std::find_if(templates.begin(), templates.end(),
        [&id](const auto& pedal) { return pedal.id == id; });
    return found == templates.end() ? nullptr : &*found;
}

const PedalTemplate& customPedalTemplate()
{
    static const PedalTemplate pedal { "custom", "Custom", "Custom VST3", { "Knob 1", "Knob 2", "Knob 3", "Knob 4" } };
    return pedal;
}

class EffectLibrary final : public juce::ListBoxModel
{
public:
    int getNumRows() override { return paths.size(); }
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (selected) g.fillAll(juce::Colour(0xff735138));
        g.setColour(cream);
        if (juce::isPositiveAndBelow(row, paths.size()))
            g.drawFittedText(juce::File(paths[row]).getFileNameWithoutExtension(), 9, 0, w - 18, h, juce::Justification::centredLeft, 1);
    }
    juce::var getDragSourceDescription(const juce::SparseSet<int>& rows) override
    {
        return rows.size() > 0 ? juce::var(paths[rows[0]]) : juce::var();
    }
    bool mayDragToExternalWindows() const override { return false; }
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override { returnKeyPressed(row); }
    void returnKeyPressed(int row) override
    {
        if (juce::isPositiveAndBelow(row, paths.size())) open(juce::File(paths[row]));
    }
    juce::StringArray paths;
    std::function<void(const juce::File&)> open;
};

struct BoardRail final : juce::Component, juce::DragAndDropTarget, juce::FileDragAndDropTarget
{
    bool isInterestedInDragSource(const SourceDetails& source) override { return source.description.isString() && isPlugin(source.description.toString()); }
    bool isInterestedInFileDrag(const juce::StringArray& files) override
    {
        return std::any_of(files.begin(), files.end(), [](const auto& file) { return isPlugin(file); });
    }
    void itemDragMove(const SourceDetails& source) override { highlight(source.localPosition.x); }
    void itemDragExit(const SourceDetails&) override { highlight(-1); }
    void itemDropped(const SourceDetails& source) override { filesDropped({ source.description.toString() }, source.localPosition.x, source.localPosition.y); }
    void fileDragMove(const juce::StringArray&, int x, int) override { highlight(x); }
    void fileDragExit(const juce::StringArray&) override { highlight(-1); }
    void filesDropped(const juce::StringArray& files, int x, int) override
    {
        highlight(-1);
        for (const auto& file : files)
            if (isPlugin(file)) insert(juce::File(file), x);
    }
    static bool isPlugin(const juce::String& path) { return PedalHost::isPlugin(juce::File(path)); }
    void paintOverChildren(juce::Graphics& g) override
    {
        if (marker >= 0)
        {
            g.setColour(accent);
            g.fillRoundedRectangle(static_cast<float>(marker) - 2, 8, 4, static_cast<float>(getHeight() - 16), 2);
        }
    }
    std::function<void(int)> highlight;
    std::function<void(const juce::File&, int)> insert;
    int marker = -1;
};

class LooperView final : public juce::Component, public juce::DragAndDropContainer,
                         private juce::AudioIODeviceCallback,
                         private juce::ChangeListener,
                         private juce::Timer
{
    static juce::WebBrowserComponent::Options createWebOptions(LooperView* owner)
    {
        auto safe = juce::Component::SafePointer<LooperView>(owner);
        auto options = juce::WebBrowserComponent::Options().withNativeIntegrationEnabled()
            .withEventListener("native-ready", [safe](const juce::var&)
            {
                if (safe != nullptr) safe->publishWebState();
            })
            .withEventListener("native-action", [safe](const juce::var& event)
            {
                if (safe != nullptr) safe->handleWebAction(event);
            });
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
        options = options.withResourceProvider([](const juce::String& path)
        {
            const char* name = nullptr;
            const char* mime = nullptr;
            if (path == "/" || path == "/index.html") { name = "index_html"; mime = "text/html"; }
            else if (path == "/styles.css") { name = "styles_css"; mime = "text/css"; }
            else if (path == "/demo.js") { name = "demo_js"; mime = "text/javascript"; }
            else if (path == "/native.js") { name = "native_js"; mime = "text/javascript"; }
            if (name == nullptr) return std::optional<juce::WebBrowserComponent::Resource> {};
            int size = 0;
            const auto* bytes = BinaryData::getNamedResource(name, size);
            if (bytes == nullptr || size <= 0) return std::optional<juce::WebBrowserComponent::Resource> {};
            juce::WebBrowserComponent::Resource resource;
            resource.data.resize(static_cast<std::size_t>(size));
            std::memcpy(resource.data.data(), bytes, static_cast<std::size_t>(size));
            resource.mimeType = mime;
            return std::optional<juce::WebBrowserComponent::Resource> { std::move(resource) };
        });
#if JUCE_WINDOWS
        options = options.withBackend(juce::WebBrowserComponent::Options::Backend::webview2);
#endif
#endif
        return options;
    }

    struct EffectCard
    {
        EffectCard(const PedalTemplate& definition, int identifier)
            : card(definition.name, false), id("effect-" + juce::String(identifier)), templateInfo(definition)
        {
            card.addAndMakeVisible(vst);
            card.addAndMakeVisible(bypass);
            card.addAndMakeVisible(remove);
        }
        std::unique_ptr<PedalHost::Effect> effect;
        VintageCard card;
        juce::String pluginName;
        juce::String id;
        PedalTemplate templateInfo;
        juce::StringArray mappings;
        juce::Array<juce::var> parameters;
        juce::TextButton vst { "ADD VST3" }, bypass { "BYPASS" }, remove { "REMOVE" };
    };

public:
    LooperView() : webView(createWebOptions(this))
    {
        setLookAndFeel(&vintage);
        setWantsKeyboardFocus(true);
        addAndMakeVisible(boardViewport);
        boardViewport.setViewedComponent(&rail, false);
        boardViewport.setScrollBarsShown(false, true);
        boardViewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
        rail.addAndMakeVisible(deck);
        chain.push_back(&deck);
        connectCard(deck);
        deck.paintFace = [this](juce::Graphics& g) { paintDeck(g); };
        rail.highlight = [this](int x) { markSlot(x < 0 ? -1 : slotAt(x)); };
        rail.insert = [this](const juce::File& file, int x) { openPlugin(file, slotAt(x)); };
        for (auto* button : { &record, &stop, &clear, &settings, &layers, &effects, &tuner, &gateToggle, &doubleToggle })
        {
            (button == &record || button == &stop || button == &clear ? static_cast<juce::Component&>(deck) : *this).addAndMakeVisible(button);
        }
        record.setName("footswitch");
        stop.setName("footswitch");
        record.onClick = [this] { send(LooperEngine::Command::record); };
        stop.onClick = [this] { send(LooperEngine::Command::stop); };
        clear.onClick = [this] { confirmClear(); };
        settings.onClick = [this] { showSettings(); };
        layers.onClick = [this] { setLayersOpen(!layerPopup.isVisible()); };
        effects.onClick = [this] { showEffects(!effectDrawer.isVisible()); };
        tuner.setName("tuner");
        tuner.setTooltip("Enable or disable the live guitar tuner");
        tuner.onClick = [this] { host.utilities.tuner = !host.utilities.tuner.load(); updateTuner(); };
        setupToggle(gateToggle, "Gate", host.utilities.gate);
        setupToggle(doubleToggle, "Doubler", host.utilities.doubler);
        setupDial(input, deck, "Deck input", engine.inputGain, 0, 1, .01, "");
        setupDial(live, deck, "Live monitor", engine.liveGain, 0, 1, .01, "");
        setupDial(loop, deck, "Loop playback", engine.loopGain, 0, 1, .01, "");
        setupDial(output, deck, "Deck output", engine.outputGain, 0, 1, .01, "");
        setupDial(inTrim, *this, "Board input gain", host.utilities.inputDb, -24, 24, .5, " dB", true);
        setupDial(gateDb, *this, "Gate threshold", host.utilities.gateDb, -80, 0, 1, " dB", true);
        setupDial(pitch, *this, "Transpose", host.utilities.transpose, -12, 12, 1, " st", true);
        setupDial(delay, *this, "Doubler delay", host.utilities.doublerMs, 0, 50, 1, " ms", true);
        setupDial(outTrim, *this, "Board output gain", host.utilities.outputDb, -24, 24, .5, " dB", true);

        addChildComponent(layerPopup);
        layerPopup.addAndMakeVisible(emptyMixer);
        layerPopup.addAndMakeVisible(closeLayers);
        closeLayers.setColour(juce::TextButton::buttonColourId, layerSurface);
        closeLayers.setColour(juce::TextButton::textColourOffId, layerInk);
        closeLayers.onClick = [this] { setLayersOpen(false); };
        emptyMixer.setText("Record a phrase to create your first layer.", juce::dontSendNotification);
        emptyMixer.setColour(juce::Label::textColourId, layerMuted);
        emptyMixer.setJustificationType(juce::Justification::centred);
        layerPopup.addAndMakeVisible(layerViewport);
        layerViewport.setViewedComponent(&layerList, false);
        layerViewport.setScrollBarsShown(true, false);
        for (int i = 0; i < LooperEngine::maxLayers; ++i)
        {
            rows[static_cast<std::size_t>(i)] = std::make_unique<LayerRow>(engine, i,
                [this](int layer) { send(LooperEngine::Command::rerecord, layer); },
                [this](int layer) { confirmDeleteLayer(layer); });
            layerList.addChildComponent(*rows[static_cast<std::size_t>(i)]);
        }
        addChildComponent(effectDrawer);
        for (auto* child : std::array<juce::Component*, 7> { &effectList, &effectSearch, &browse, &closeEffects, &refreshEffects, &addPre, &addPost })
            effectDrawer.addAndMakeVisible(child);
        effectList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff34271e));
        effectList.setRowHeight(29);
        effectSearch.setTextToShowWhenEmpty("Find a VST3 effect", muted);
        effectSearch.onTextChange = [this] { filterEffects(); };
        effectLibrary.open = [this](const juce::File& file) { openPlugin(file, deckIndex()); };
        closeEffects.onClick = [this] { showEffects(false); };
        refreshEffects.onClick = [this] { startEffectScan(); };
        addPre.onClick = [this] { addSelected(deckIndex()); };
        addPost.onClick = [this] { addSelected(deckIndex() + 1); };
        browse.onClick = [this] { browsePlugins(); };
        setSize(1500, 980);

        std::unique_ptr<juce::XmlElement> savedAudio;
        const auto settingsResult = readAudioSettings(audioSettingsFile(), savedAudio);
        if (settingsResult.failed()) errorText = settingsResult.getErrorMessage();
        if (savedAudio != nullptr)
        {
            if (const auto* setup = savedAudio->getChildByName("DEVICESETUP"))
                rememberedAudioDevice = std::make_unique<juce::XmlElement>(*setup);
            const auto mode = savedAudio->getIntAttribute("inputMode");
            engine.inputMode = mode == 2 ? LooperEngine::InputMode::stereo
                : mode == 1 ? LooperEngine::InputMode::monoSecond : LooperEngine::InputMode::monoFirst;
        }
        const auto error = devices.initialise(2, 2,
            savedAudio != nullptr ? savedAudio->getChildByName("DEVICESETUP") : nullptr, false);
        if (rememberedAudioDevice != nullptr)
        {
            const auto savedType = rememberedAudioDevice->getStringAttribute("deviceType");
            const auto savedInput = rememberedAudioDevice->getStringAttribute("audioInputDeviceName");
            const auto savedOutput = rememberedAudioDevice->getStringAttribute("audioOutputDeviceName");
            const auto current = devices.getAudioDeviceSetup();
            preserveRememberedAudioDevice = error.isNotEmpty()
                || (savedType.isNotEmpty() && savedType != devices.getCurrentAudioDeviceType())
                || (savedInput.isNotEmpty() && savedInput != current.inputDeviceName)
                || (savedOutput.isNotEmpty() && savedOutput != current.outputDeviceName);
        }
        if (error.isNotEmpty())
            errorText = "Audio setup: " + error;
        devices.addAudioCallback(this);
        devices.addChangeListener(this);
        refreshBoardPresets();
        startTimerHz(30);

#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
        boardViewport.setVisible(false);
        for (auto* component : std::array<juce::Component*, 18> { &record, &stop, &clear, &settings, &layers, &effects,
                 &tuner, &gateToggle, &doubleToggle, &input, &live, &loop, &output, &inTrim, &gateDb, &pitch, &delay, &outTrim })
            component->setVisible(false);
        addAndMakeVisible(webView);
        webView.toBack();
        webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot() + "#wood");
#endif
        loadPluginDirectories();
        startEffectScan();
    }

    ~LooperView() override
    {
        stopTimer();
        devices.removeChangeListener(this);
        if (settingsWindow != nullptr)
            delete settingsWindow.getComponent();
        chooser.reset();
        loopChooser.reset();
        pedalChooser.reset();
        layerViewport.setViewedComponent(nullptr, false);
        boardViewport.setViewedComponent(nullptr, false);
        devices.removeAudioCallback(this);
        devices.closeAudioDevice();
        setLookAndFeel(nullptr);
    }

    bool hasLoop() const { return engine.getLength() != 0 || engine.getState() == LooperEngine::State::recording
                                || engine.getState() == LooperEngine::State::countingIn; }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(background);
        for (int x = 0; x < getWidth(); x += 142)
        {
            const auto trim = x % 3 == 0 ? 0xff5d3c29 : 0xff65442f;
            g.setGradientFill({ juce::Colour(trim), static_cast<float>(x), 0, juce::Colour(0xff3f291e), static_cast<float>(x + 142), 0, false });
            g.fillRect(x, 0, 141, getHeight());
            for (int line = 5; line < 140; line += 5)
            {
                g.setColour(juce::Colour(line % 3 == 0 ? 0x11f2c695 : 0x220e0805));
                g.fillRect(x + line, 0, 1, getHeight());
            }
        }
        for (int y : { boardViewport.getY() + 100, boardViewport.getBottom() - 168 })
        {
            g.setGradientFill({ juce::Colour(0xff51463b), 0, static_cast<float>(y), juce::Colour(0xff17120f), 0, static_cast<float>(y + 46), false });
            g.fillRect(0, y, getWidth(), 44);
        }
        g.setColour(juce::Colour(0x24000000));
        g.fillRect(0, 0, getWidth(), 72);
        g.setColour(juce::Colour(0x28fff1da));
        g.drawHorizontalLine(71, 0.0f, static_cast<float>(getWidth()));
        g.setColour(juce::Colour(0xff362a1d));
        g.fillRoundedRectangle(16.0f, 18.0f, 30.0f, 36.0f, 6.0f);
        g.setColour(juce::Colour(0xffa58a5d));
        g.drawRoundedRectangle(16.5f, 18.5f, 29.0f, 35.0f, 6.0f, 1.0f);
        g.setColour(juce::Colour(0xffeed7ae));
        g.setFont(juce::FontOptions("Georgia", 17.0f, juce::Font::italic));
        g.drawText("L/", 20, 25, 22, 21, juce::Justification::centred);
        g.setColour(cream);
        g.setFont(juce::FontOptions("Georgia", 21.0f, juce::Font::bold | juce::Font::italic));
        g.drawText("LOOPER", 55, 17, 91, 28, juce::Justification::left);
        g.setColour(muted);
        g.setFont(juce::FontOptions(7.0f));
        g.drawText("VINTAGE WORKBENCH", 56, 43, 102, 14, juce::Justification::left);
        g.setColour(juce::Colour(0xb3291d13));
        g.fillRoundedRectangle(328.0f, 21.0f, std::min(360.0f, static_cast<float>(getWidth() - 650)), 31.0f, 6.0f);
        g.setColour(accent);
        g.fillEllipse(341.0f, 33.0f, 5.0f, 5.0f);
        g.setColour(cream);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText("Late night ideas", 353, 27, 105, 18, juce::Justification::left);
        g.setColour(muted);
        g.setFont(juce::FontOptions(7.0f));
        g.drawText("DEMO BOARD", 460, 28, 90, 17, juce::Justification::left);
        g.setColour(muted);
        g.setFont(juce::FontOptions(8.0f));
        g.drawFittedText(errorText.isNotEmpty() ? errorText : deviceText, getWidth() - 300, 25, 190, 25,
                         juce::Justification::centredRight, 2);
        g.setColour(muted);
        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        g.drawText("SIGNAL CHAIN", 20, 92, 130, 13, juce::Justification::left);
        g.setColour(cream);
        g.setFont(juce::FontOptions("Georgia", 20.0f, juce::Font::italic));
        g.drawText("The loop deck", 20, 105, 230, 29, juce::Justification::left);
        g.setColour(muted);
        g.setFont(juce::FontOptions(8.0f));
        g.drawText("Drag pedal headers to change signal order, or focus a header and use Left and Right. Effects adds VST3s before or after the deck.",
                   20, 139, getWidth() - 270, 18, juce::Justification::left);
        const auto signature = engine.getTimeSignature();
        g.setColour(accent);
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText(juce::String(engine.bpm.load(), 0) + " BPM · " + juce::String(signature.beats) + "/" + juce::String(signature.unit),
                   getWidth() - 255, 104, 96, 17, juce::Justification::right);
        juce::StringArray order;
        for (auto* card : chain) order.add(card == &deck ? "LOOP DECK" : card->getTitle());
        g.setColour(juce::Colour(0xffd6c2a4));
        g.setFont(juce::FontOptions(8.0f));
        g.drawFittedText("PROCESSING ORDER    " + order.joinIntoString("  >  "), 20, boardViewport.getBottom() - 22,
                         getWidth() - 40, 16, juce::Justification::left, 1);
        const auto y = getHeight() - 104;
        g.setGradientFill({ juce::Colour(0xfff5efdf), 0, static_cast<float>(y), juce::Colour(0xffece5d5), 0, static_cast<float>(getHeight()), false });
        g.fillRect(0, y, getWidth(), getHeight() - y);
        g.setColour(juce::Colour(0xffb9aa8e));
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));
        g.setColour(juce::Colour(0xff4a3424));
        for (const auto& pair : { std::pair { &inTrim, "INPUT" }, { &gateDb, "GATE" }, { &pitch, "TRANSPOSE" },
                                 { &delay, "DOUBLER" }, { &outTrim, "OUTPUT" } })
            g.drawText(pair.second, pair.first->getX(), y + 9, pair.first->getWidth(), 15, juce::Justification::centred);
    }

    void paintDeck(juce::Graphics& g)
    {
        const auto w = deck.getWidth();
        const auto y = deck.getHeight() - 226;
        auto grille = juce::Rectangle<int>(22, y, w - 44, 118);
        g.setColour(juce::Colour(0xffc7ad86));
        g.fillRoundedRectangle(grille.toFloat(), 6);
        g.setColour(juce::Colour(0x45614530));
        for (int x = grille.getX() + 4; x < grille.getRight() - 3; x += 3)
            g.fillRect(x, y + 4, 1, 110);
        for (int line = y + 4; line < y + 114; line += 3)
            g.fillRect(grille.getX() + 4, line, grille.getWidth() - 8, 1);
        auto screen = grille.withSizeKeepingCentre(std::min(176, w - 172), 90);
        g.setColour(juce::Colour(0xff25201b));
        g.fillRoundedRectangle(screen.toFloat(), 6);
        for (int side : { -1, 1 })
        {
            auto reel = juce::Rectangle<float>(60, 60).withCentre({ static_cast<float>(grille.getCentreX() + side * 136), static_cast<float>(grille.getCentreY()) });
            g.setColour(juce::Colour(0xff32281f));
            g.fillEllipse(reel);
            g.setColour(juce::Colour(0xffd0b38b));
            g.drawEllipse(reel.reduced(3), 2);
            const auto fraction = engine.getLength() == 0 ? 0.0f : static_cast<float>(engine.getPosition()) / static_cast<float>(engine.getLength());
            for (int spoke = 0; spoke < 5; ++spoke)
            {
                const auto angle = juce::MathConstants<float>::twoPi * (static_cast<float>(spoke) / 5 + fraction);
                juce::Path fan;
                fan.addPieSegment(reel.reduced(8), angle, angle + .44f, .31f);
                g.fillPath(fan);
            }
        }
        const auto state = engine.getState();
        const auto label = state == LooperEngine::State::countingIn
            ? "COUNT-IN: " + juce::String(engine.getCountBeats()) + " BEATS LEFT"
            : state == LooperEngine::State::waiting || state == LooperEngine::State::overdubbing
                ? stateName(state) + " " + juce::String(engine.getRecordingLayer() + 1) : stateName(state);
        g.setColour(state == LooperEngine::State::recording || state == LooperEngine::State::overdubbing ? juce::Colour(0xffff9779) : accent);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawFittedText(label, screen.getX() + 6, screen.getY() + 5, screen.getWidth() - 12, 15, juce::Justification::centred, 1);
        g.setFont(juce::FontOptions("Consolas", 20.0f, juce::Font::plain));
        g.drawText(juce::String(engine.bpm.load(), 0), screen.getX(), screen.getY() + 20, screen.getWidth(), 27, juce::Justification::centred);
        g.setFont(juce::FontOptions("Consolas", 8.0f, juce::Font::bold));
        g.drawText("BPM", screen.getX() + screen.getWidth() / 2 + 23, screen.getY() + 28, 31, 14, juce::Justification::left);
        const auto signature = engine.getTimeSignature();
        g.setFont(juce::FontOptions("Consolas", 16.0f, juce::Font::plain));
        g.drawText(juce::String(signature.beats) + "/" + juce::String(signature.unit), screen.getX(), screen.getY() + 53, screen.getWidth(), 22, juce::Justification::centred);
        g.setColour(muted);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        const char* labels[] = { "INPUT", "LIVE", "LOOP", "OUTPUT" };
        for (int i = 0; i < 4; ++i)
            g.drawText(labels[i], 20 + i * ((w - 40) / 4), 70, (w - 40) / 4, 17, juce::Justification::centred);
        g.drawText(juce::String(engine.getLayerCount()) + (engine.metronome.load() ? " LAYERS  /  CLICK ON" : " LAYERS  /  CLICK OFF"),
                   24, deck.getHeight() - 33, w - 114, 18, juce::Justification::left);
    }

    void resized() override
    {
        webView.setBounds(getLocalBounds());
        settings.setBounds(getWidth() - 97, 20, 78, 33);
        layers.setBounds(getWidth() - 82, 96, 64, 34);
        effects.setBounds(getWidth() - 152, 96, 62, 34);
        tuner.setBounds(164, 17, 143, 46);
        boardViewport.setBounds(0, 170, getWidth(), getHeight() - 274);
        layoutBoard();
        auto popup = getLocalBounds().withSizeKeepingCentre(std::min(getWidth() - 80, 860), std::min(getHeight() - 110, 390));
        layerPopup.setBounds(popup);
        closeLayers.setBounds(popup.getWidth() - 62, 17, 45, 45);
        layerViewport.setBounds(20, 145, popup.getWidth() - 40, popup.getHeight() - 160);
        emptyMixer.setBounds(16, 153, layerViewport.getWidth(), 48);
        layerList.setSize(layerViewport.getWidth() - layerViewport.getScrollBarThickness(), engine.getLayerCount() * 58);
        for (int i = 0; i < LooperEngine::maxLayers; ++i)
            if (rows[static_cast<std::size_t>(i)] != nullptr)
                rows[static_cast<std::size_t>(i)]->setBounds(0, i * 58, layerList.getWidth(), 56);
        effectDrawer.setBounds(getWidth() - 326, 82, 310, std::min(590, getHeight() - 106));
        effectSearch.setBounds(14, 85, 282, 34);
        effectList.setBounds(14, 128, 282, effectDrawer.getHeight() - 265);
        refreshEffects.setBounds(14, effectDrawer.getHeight() - 128, 78, 30);
        browse.setBounds(99, effectDrawer.getHeight() - 128, 197, 30);
        addPre.setBounds(14, effectDrawer.getHeight() - 90, 137, 30);
        addPost.setBounds(159, effectDrawer.getHeight() - 90, 137, 30);
        closeEffects.setBounds(267, 9, 30, 28);
        const auto footerY = getHeight() - 70;
        inTrim.setBounds(16, footerY, 80, 54);
        gateDb.setBounds(92, footerY, 80, 54);
        pitch.setBounds(168, footerY, 80, 54);
        delay.setBounds(getWidth() - 216, footerY, 80, 54);
        outTrim.setBounds(getWidth() - 140, footerY, 80, 54);
        gateToggle.setBounds(104, getHeight() - 16, 56, 14);
        doubleToggle.setBounds(getWidth() - 204, getHeight() - 16, 56, 14);
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::spaceKey) { send(LooperEngine::Command::record); return true; }
        if (key == juce::KeyPress('s')) { send(LooperEngine::Command::stop); return true; }
        if (key == juce::KeyPress::escapeKey)
        {
            if (layerPopup.isVisible()) { setLayersOpen(false); return true; }
            if (effectDrawer.isVisible()) { showEffects(false); return true; }
        }
        return false;
    }

private:
    juce::var makeWebState()
    {
        auto state = std::make_unique<juce::DynamicObject>();
        const auto engineState = engine.getState();
        const bool recording = engineState == LooperEngine::State::recording
            || engineState == LooperEngine::State::waiting || engineState == LooperEngine::State::overdubbing;
        const bool playing = engineState == LooperEngine::State::playing || recording
            || engineState == LooperEngine::State::countingIn;
        auto status = stateName(engineState);
        if (engineState == LooperEngine::State::countingIn)
            status += ": " + juce::String(engine.getCountBeats()) + " BEATS LEFT";
        const auto recordLabel = engineState == LooperEngine::State::empty ? "RECORD"
            : engineState == LooperEngine::State::countingIn ? "CANCEL COUNT-IN"
            : engineState == LooperEngine::State::recording ? "FINISH LOOP"
            : engineState == LooperEngine::State::stopped ? "PLAY"
            : engineState == LooperEngine::State::playing ? engine.isLayeredLoop() ? "ADD LAYER" : "OVERDUB"
            : "CANCEL TAKE";
        const auto length = engine.getLength();
        const auto progress = engine.getProgress();
        const auto signature = engine.getTimeSignature();
        juce::Array<juce::var> layerData, effectData, templateData;
        for (const auto& pedal : getPedalTemplates())
        {
            auto item = std::make_unique<juce::DynamicObject>();
            juce::Array<juce::var> knobs;
            for (const auto& knob : pedal.knobs) knobs.add(knob);
            item->setProperty("id", pedal.id);
            item->setProperty("category", pedal.category);
            item->setProperty("name", pedal.name);
            item->setProperty("knobs", juce::var(knobs));
            templateData.add(juce::var(item.release()));
        }
        for (int i = 0; i < engine.getLayerCount(); ++i)
        {
            const auto& controls = engine.getLayerControls(i);
            auto layer = std::make_unique<juce::DynamicObject>();
            layer->setProperty("id", i);
            layer->setProperty("volume", static_cast<int>(std::round(controls.volume.load() * 100.0f)));
            layer->setProperty("pan", static_cast<int>(std::round(controls.pan.load() * 100.0f)));
            layer->setProperty("active", controls.active.load());
            layer->setProperty("solo", controls.solo.load());
            layer->setProperty("capturing", engine.getRecordingLayer() == i);
            layerData.add(juce::var(layer.release()));
        }
        for (auto* card : chain)
            if (card != &deck)
                for (const auto& item : effectCards)
                    if (&item->card == card)
                    {
                        auto effect = std::make_unique<juce::DynamicObject>();
                        effect->setProperty("id", item->id);
                        effect->setProperty("template", item->templateInfo.id);
                        effect->setProperty("category", item->templateInfo.category);
                        effect->setProperty("name", item->templateInfo.name);
                        effect->setProperty("pluginName", item->pluginName);
                        effect->setProperty("loaded", item->effect != nullptr);
                        effect->setProperty("bypassed", item->effect != nullptr && item->effect->bypassed.load());
                        effect->setProperty("channels", item->effect != nullptr ? item->effect->channels : 0);
                        juce::Array<juce::var> knobs;
                        for (const auto& knob : item->templateInfo.knobs) knobs.add(knob);
                        effect->setProperty("knobs", juce::var(knobs));
                        juce::Array<juce::var> assignments;
                        if (item->effect != nullptr)
                        {
                            auto& plugin = *item->effect->instance;
                            for (const auto& id : item->mappings)
                            {
                                auto assignment = std::make_unique<juce::DynamicObject>();
                                auto* parameter = findPedalParameter(plugin, id);
                                assignment->setProperty("id", id);
                                assignment->setProperty("value", parameter != nullptr ? parameter->getValue() : 0.0f);
                                assignment->setProperty("text", parameter != nullptr ? parameter->getCurrentValueAsText() : "");
                                assignments.add(juce::var(assignment.release()));
                            }
                        }
                        effect->setProperty("parameters", juce::var(item->parameters));
                        effect->setProperty("assignments", juce::var(assignments));
                        effectData.add(juce::var(effect.release()));
                    }

        state->setProperty("status", status);
        state->setProperty("recordLabel", recordLabel);
        state->setProperty("recording", recording);
        state->setProperty("playing", playing);
        state->setProperty("canStop", engineState != LooperEngine::State::empty);
        state->setProperty("canClear", hasLoop());
        state->setProperty("canLoadLoop", !loopFileDialogOpen
            && (engineState == LooperEngine::State::empty || engineState == LooperEngine::State::stopped));
        state->setProperty("canSaveLoop", !loopFileDialogOpen && engineState == LooperEngine::State::stopped && length > 0);
        state->setProperty("loopFileStatus", loopFileStatus);
        state->setProperty("progress", juce::jlimit(0.0, 1.0, progress));
        state->setProperty("bpm", static_cast<int>(std::round(engine.bpm.load())));
        state->setProperty("beats", signature.beats);
        state->setProperty("unit", signature.unit);
        state->setProperty("metronome", engine.metronome.load());
        state->setProperty("layered", engine.isLayeredLoop());
        state->setProperty("countIn", engine.countInBars.load());
        state->setProperty("loopBars", engine.autoStopBars.load());
        state->setProperty("settingsLocked", engineState != LooperEngine::State::empty || length != 0);
        state->setProperty("countingIn", engineState == LooperEngine::State::countingIn);
        state->setProperty("canEditLayers", engineState == LooperEngine::State::playing || engineState == LooperEngine::State::stopped);
        state->setProperty("canChangeInput", engineState == LooperEngine::State::empty
            || engineState == LooperEngine::State::playing || engineState == LooperEngine::State::stopped);
        state->setProperty("inputMode", engine.inputMode.load() == LooperEngine::InputMode::stereo ? 2
            : engine.inputMode.load() == LooperEngine::InputMode::monoSecond ? 1 : 0);
        state->setProperty("deviceReady", deviceReady.load());
        state->setProperty("inputPeak", engine.inputPeak.load());
        state->setProperty("duration", engine.getSampleRate() > 0.0 ? static_cast<double>(length) / engine.getSampleRate() : 0.0);
        state->setProperty("layers", juce::var(layerData));
        state->setProperty("effects", juce::var(effectData));
        state->setProperty("pedalTemplates", juce::var(templateData));
        state->setProperty("vstTarget", vstTargetId);
        state->setProperty("vstPickerError", vstPickerError);
        state->setProperty("vstPickerLoading", scanInProgress && vstTargetId.isNotEmpty());
        state->setProperty("vstReplacing", vstReplacing);
        state->setProperty("deckIndex", deckIndex());
        juce::Array<juce::var> presets;
        for (const auto& name : boardPresetNames) presets.add(name);
        state->setProperty("boardPresets", juce::var(presets));
        state->setProperty("boardPreset", selectedBoardPreset);
        state->setProperty("tuner", host.utilities.tuner.load());
        state->setProperty("tunerText", tuner.getButtonText());
        state->setProperty("tunerNote", tunerNote);
        state->setProperty("tunerCents", tunerCents);
        state->setProperty("gate", host.utilities.gate.load());
        state->setProperty("doubler", host.utilities.doubler.load());
        state->setProperty("deviceStatus", errorText.isNotEmpty() ? errorText : deviceText);
        state->setProperty("error", errorText);

        auto levels = std::make_unique<juce::DynamicObject>();
        levels->setProperty("input", static_cast<int>(std::round(engine.inputGain.load() * 100.0f)));
        levels->setProperty("live", static_cast<int>(std::round(engine.liveGain.load() * 100.0f)));
        levels->setProperty("loop", static_cast<int>(std::round(engine.loopGain.load() * 100.0f)));
        levels->setProperty("output", static_cast<int>(std::round(engine.outputGain.load() * 100.0f)));
        state->setProperty("levels", juce::var(levels.release()));
        auto footer = std::make_unique<juce::DynamicObject>();
        footer->setProperty("input", static_cast<int>(std::round(host.utilities.inputDb.load())));
        footer->setProperty("gate", static_cast<int>(std::round(host.utilities.gateDb.load())));
        footer->setProperty("transpose", static_cast<int>(std::round(host.utilities.transpose.load())));
        footer->setProperty("doubler", static_cast<int>(std::round(host.utilities.doublerMs.load())));
        footer->setProperty("output", static_cast<int>(std::round(host.utilities.outputDb.load())));
        state->setProperty("footer", juce::var(footer.release()));
        return juce::var(state.release());
    }

    void publishWebState()
    {
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
        webView.emitEventIfBrowserIsVisible("native-state", makeWebState());
#endif
    }

    void publishVstCatalog()
    {
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
        auto catalog = std::make_unique<juce::DynamicObject>();
        juce::Array<juce::var> plugins;
        for (std::size_t i = 0; i < discoveredVsts.size(); ++i)
        {
            auto plugin = std::make_unique<juce::DynamicObject>();
            plugin->setProperty("index", static_cast<int>(i));
            plugin->setProperty("name", discoveredVsts[i].name);
            plugin->setProperty("manufacturer", discoveredVsts[i].manufacturerName);
            plugins.add(juce::var(plugin.release()));
        }
        catalog->setProperty("plugins", juce::var(plugins));
        webView.emitEventIfBrowserIsVisible("vst-catalog", juce::var(catalog.release()));
#endif
    }

    void publishAudioState(bool scan)
    {
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
        auto result = std::make_unique<juce::DynamicObject>();
        auto strings = [](const juce::StringArray& values)
        {
            juce::Array<juce::var> items;
            for (const auto& value : values) items.add(value);
            return juce::var(items);
        };
        juce::StringArray types;
        for (auto* type : devices.getAvailableDeviceTypes()) types.add(type->getTypeName());
        result->setProperty("types", strings(types));
        result->setProperty("type", devices.getCurrentAudioDeviceType());
        auto* type = devices.getCurrentDeviceTypeObject();
        if (type != nullptr && scan) type->scanForDevices();
        result->setProperty("separate", type != nullptr && type->hasSeparateInputsAndOutputs());
        result->setProperty("outputs", strings(type != nullptr ? type->getDeviceNames(false) : juce::StringArray {}));
        result->setProperty("inputs", strings(type != nullptr ? type->getDeviceNames(true) : juce::StringArray {}));
        const auto setup = devices.getAudioDeviceSetup();
        result->setProperty("output", setup.outputDeviceName);
        result->setProperty("input", setup.inputDeviceName);
        juce::Array<juce::var> rates, buffers, inputChannels, outputChannels, midiInputs;
        if (auto* device = devices.getCurrentAudioDevice())
        {
            for (const auto rate : device->getAvailableSampleRates()) rates.add(rate);
            for (const auto size : device->getAvailableBufferSizes()) buffers.add(size);
            auto channels = [](const juce::StringArray& names, const juce::BigInteger& active)
            {
                juce::Array<juce::var> items;
                for (int i = 0; i < names.size(); ++i)
                {
                    auto channel = std::make_unique<juce::DynamicObject>();
                    channel->setProperty("name", names[i]);
                    channel->setProperty("active", active[i]);
                    items.add(juce::var(channel.release()));
                }
                return items;
            };
            inputChannels = channels(device->getInputChannelNames(), device->getActiveInputChannels());
            outputChannels = channels(device->getOutputChannelNames(), device->getActiveOutputChannels());
            result->setProperty("controlPanel", device->hasControlPanel());
        }
        else result->setProperty("controlPanel", false);
        for (const auto& info : juce::MidiInput::getAvailableDevices())
        {
            auto midi = std::make_unique<juce::DynamicObject>();
            midi->setProperty("id", info.identifier);
            midi->setProperty("name", info.name);
            midi->setProperty("active", devices.isMidiInputDeviceEnabled(info.identifier));
            midiInputs.add(juce::var(midi.release()));
        }
        result->setProperty("rates", juce::var(rates));
        result->setProperty("buffers", juce::var(buffers));
        result->setProperty("inputChannels", juce::var(inputChannels));
        result->setProperty("outputChannels", juce::var(outputChannels));
        result->setProperty("midiInputs", juce::var(midiInputs));
        result->setProperty("rate", setup.sampleRate);
        result->setProperty("buffer", setup.bufferSize);
        result->setProperty("error", audioError);
        webView.emitEventIfBrowserIsVisible("audio-state", juce::var(result.release()));
#else
        juce::ignoreUnused(scan);
#endif
    }

    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        if (audioPanelOpen) publishAudioState(false);
    }

    static juce::File audioSettingsFile()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Looper").getChildFile("audio-settings.xml");
    }

    void persistAudioSettings()
    {
        const auto mode = engine.inputMode.load();
        auto* device = devices.getCurrentAudioDevice();
        const auto result = saveAudioSettings(audioSettingsFile(), devices,
            mode == LooperEngine::InputMode::stereo ? 2 : mode == LooperEngine::InputMode::monoSecond ? 1 : 0,
            preserveRememberedAudioDevice || device == nullptr || !device->isOpen() ? rememberedAudioDevice.get() : nullptr);
        if (result.failed()) errorText = "Audio settings could not be saved: " + result.getErrorMessage();
        else if (!preserveRememberedAudioDevice && device != nullptr && device->isOpen())
            rememberedAudioDevice = devices.createStateXml();
    }

    void handleAudioAction(const juce::var& event)
    {
        const auto name = event.getProperty("name", {}).toString();
        const auto text = event.getProperty("text", {}).toString();
        const auto value = static_cast<double>(event.getProperty("value", 0));
        audioError.clear();
        if (name == "refresh") { publishAudioState(true); return; }
        if (engine.getState() != LooperEngine::State::empty || engine.getLength() != 0)
        {
            audioError = "Clear the loop before changing audio devices or MIDI inputs.";
            publishAudioState(false);
            return;
        }
        auto* type = devices.getCurrentDeviceTypeObject();
        auto* device = devices.getCurrentAudioDevice();
        auto setup = devices.getAudioDeviceSetup();
        bool applySetup = false;
        if (name == "type")
        {
            juce::AudioIODeviceType* selected = nullptr;
            for (auto* candidate : devices.getAvailableDeviceTypes())
                if (candidate->getTypeName() == text) selected = candidate;
            if (selected == nullptr) audioError = "That audio device type is no longer available.";
            else
            {
                selected->scanForDevices();
                devices.setCurrentAudioDeviceType(text, true);
                if (devices.getCurrentAudioDevice() == nullptr || !devices.getCurrentAudioDevice()->isOpen())
                    audioError = "Could not open an audio device for " + text + ". Select an available device.";
            }
        }
        else if (name == "input" || name == "output")
        {
            const bool isInput = name == "input";
            if (type == nullptr || (text.isNotEmpty() && !type->getDeviceNames(isInput).contains(text)))
                audioError = "That audio device is no longer available. Refresh the device list.";
            else
            {
                if (isInput) { setup.inputDeviceName = text; setup.useDefaultInputChannels = true; }
                else { setup.outputDeviceName = text; setup.useDefaultOutputChannels = true; }
                if (!type->hasSeparateInputsAndOutputs())
                {
                    setup.inputDeviceName = text;
                    setup.outputDeviceName = text;
                    setup.useDefaultInputChannels = true;
                    setup.useDefaultOutputChannels = true;
                }
                applySetup = true;
            }
        }
        else if (name == "rate" && device != nullptr && device->getAvailableSampleRates().contains(value))
        {
            setup.sampleRate = value;
            applySetup = true;
        }
        else if (name == "buffer" && device != nullptr && std::isfinite(value) && std::floor(value) == value
                 && value > 0 && value <= std::numeric_limits<int>::max()
                 && device->getAvailableBufferSizes().contains(static_cast<int>(value)))
        {
            setup.bufferSize = static_cast<int>(value);
            applySetup = true;
        }
        else if ((name == "inputChannel" || name == "outputChannel") && device != nullptr)
        {
            const bool isInput = name == "inputChannel";
            auto& bits = isInput ? setup.inputChannels : setup.outputChannels;
            bits = isInput ? device->getActiveInputChannels() : device->getActiveOutputChannels();
            const int index = static_cast<int>(event.getProperty("id", -1));
            const int count = isInput ? device->getInputChannelNames().size() : device->getOutputChannelNames().size();
            if (!setAudioChannel(bits, index, count, value != 0))
                audioError = "Select at least one channel. Up to two channels can be active.";
            else
            {
                if (isInput) setup.useDefaultInputChannels = false;
                else setup.useDefaultOutputChannels = false;
                applySetup = true;
            }
        }
        else if (name == "midi")
        {
            const auto available = juce::MidiInput::getAvailableDevices();
            const auto found = std::find_if(available.begin(), available.end(), [&text](const auto& info) { return info.identifier == text; });
            if (found == available.end()) audioError = "That MIDI input is no longer available. Refresh the device list.";
            else
            {
                devices.setMidiInputDeviceEnabled(text, value != 0);
                if (devices.isMidiInputDeviceEnabled(text) != (value != 0))
                    audioError = "Could not change the selected MIDI input.";
            }
        }
        else if (name == "test" && device != nullptr && device->isOpen()) devices.playTestSound();
        else if (name == "controlPanel" && device != nullptr && device->hasControlPanel())
        {
            juce::Component modalWindow;
            modalWindow.setOpaque(true);
            modalWindow.addToDesktop(0);
            modalWindow.enterModalState();
            if (device->showControlPanel())
            {
                devices.closeAudioDevice();
                devices.restartLastAudioDevice();
                if (devices.getCurrentAudioDevice() == nullptr || !devices.getCurrentAudioDevice()->isOpen())
                    audioError = "Could not restart the audio device after closing its control panel.";
            }
            else audioError = "Could not open the audio device control panel.";
        }
        else if (name == "reset" && device != nullptr && device->hasControlPanel())
        {
            devices.closeAudioDevice();
            devices.restartLastAudioDevice();
            if (devices.getCurrentAudioDevice() == nullptr || !devices.getCurrentAudioDevice()->isOpen())
                audioError = "Could not restart the audio device.";
        }
        else audioError = "Audio setting unavailable or invalid.";
        if (applySetup)
            audioError = devices.setAudioDeviceSetup(setup, true);
        if (audioError.isEmpty() && (applySetup || name == "type" || name == "midi"))
        {
            if ((applySetup || name == "type") && devices.getCurrentAudioDevice() != nullptr
                && devices.getCurrentAudioDevice()->isOpen())
                preserveRememberedAudioDevice = false;
            persistAudioSettings();
        }
        publishAudioState(false);
    }

    EffectCard* findEffect(const juce::String& id) const
    {
        const auto found = std::find_if(effectCards.begin(), effectCards.end(),
            [&id](const auto& item) { return item->id == id; });
        return found == effectCards.end() ? nullptr : found->get();
    }

    void updatePedalParameters(EffectCard& item)
    {
        item.parameters.clear();
        if (item.effect == nullptr) return;
        for (auto* parameter : item.effect->instance->getParameters())
            if (auto* hosted = dynamic_cast<juce::HostedAudioProcessorParameter*>(parameter))
                if (parameter->isAutomatable() && hosted->getParameterID().isNotEmpty())
                {
                    auto entry = std::make_unique<juce::DynamicObject>();
                    entry->setProperty("id", hosted->getParameterID());
                    entry->setProperty("name", parameter->getName(80));
                    item.parameters.add(juce::var(entry.release()));
                }
    }

    void handleWebAction(const juce::var& event)
    {
        if (!event.isObject())
        {
            errorText = "Invalid action from the interface.";
            return;
        }
        const auto action = event.getProperty("action", {}).toString();
        const auto name = event.getProperty("name", {}).toString();
        const auto value = static_cast<double>(event.getProperty("value", 0));
        if (action == "record") send(LooperEngine::Command::record);
        else if (action == "stop") send(LooperEngine::Command::stop);
        else if (action == "clear") confirmClear();
        else if (action == "save-project" || action == "load-project"
                 || action == "import-wav" || action == "export-wav") chooseLoopFile(action);
        else if (action == "audio-open") { audioPanelOpen = true; audioError.clear(); publishAudioState(true); }
        else if (action == "audio-close") audioPanelOpen = false;
        else if (action == "audio") handleAudioAction(event);
        else if (action == "effects") showEffects(!effectDrawer.isVisible());
        else if (action == "tuner") { host.utilities.tuner = !host.utilities.tuner.load(); updateTuner(); }
        else if (action == "switch")
        {
            if (name == "gate") host.utilities.gate = !host.utilities.gate.load();
            else if (name == "doubler") host.utilities.doubler = !host.utilities.doubler.load();
        }
        else if (action == "set-level")
        {
            const auto level = static_cast<float>(juce::jlimit(0.0, 100.0, value) / 100.0);
            if (name == "input") engine.inputGain = level;
            else if (name == "live") engine.liveGain = level;
            else if (name == "loop") engine.loopGain = level;
            else if (name == "output") engine.outputGain = level;
            else errorText = "Unknown deck level control.";
        }
        else if (action == "set-footer")
        {
            if (name == "input") host.utilities.inputDb = static_cast<float>(juce::jlimit(-24.0, 24.0, value));
            else if (name == "gate") host.utilities.gateDb = static_cast<float>(juce::jlimit(-80.0, 0.0, value));
            else if (name == "transpose") host.utilities.transpose = static_cast<float>(juce::jlimit(-12.0, 12.0, value));
            else if (name == "doubler") host.utilities.doublerMs = static_cast<float>(juce::jlimit(0.0, 50.0, value));
            else if (name == "output") host.utilities.outputDb = static_cast<float>(juce::jlimit(-24.0, 24.0, value));
            else errorText = "Unknown board control.";
        }
        else if (action == "layer")
        {
            const auto layer = static_cast<int>(event.getProperty("id", -1));
            if (!juce::isPositiveAndBelow(layer, engine.getLayerCount()))
            {
                errorText = "That layer is no longer available.";
                return;
            }
            auto& controls = engine.getLayerControls(layer);
            if (name == "volume") controls.volume = static_cast<float>(juce::jlimit(0.0, 100.0, value) / 100.0);
            else if (name == "pan") controls.pan = static_cast<float>(juce::jlimit(-100.0, 100.0, value) / 100.0);
            else if (name == "active") controls.active = !controls.active.load();
            else if (name == "solo") controls.solo = !controls.solo.load();
            else if (name == "retake") send(LooperEngine::Command::rerecord, layer);
            else if (name == "delete") confirmDeleteLayer(layer);
            else errorText = "Unknown layer control.";
        }
        else if (action == "add-template")
        {
            const auto* pedal = findPedalTemplate(event.getProperty("template", {}).toString());
            const auto slot = static_cast<int>(event.getProperty("slot", static_cast<int>(chain.size())));
            if (pedal == nullptr) errorText = "Unknown pedal template.";
            else if (!juce::isPositiveAndBelow(slot, static_cast<int>(chain.size()) + 1))
                errorText = "Invalid pedalboard drop position.";
            else addTemplate(*pedal, slot);
        }
        else if (action == "setting")
        {
            const auto state = engine.getState();
            const bool empty = state == LooperEngine::State::empty && engine.getLength() == 0;
            if (name == "layered") engine.setLayeredLoop(value != 0.0);
            else if (name == "metronome") engine.metronome = value != 0.0;
            else if (name == "bpm" && state != LooperEngine::State::countingIn && value >= 30 && value <= 300 && std::floor(value) == value)
                engine.bpm = value;
            else if ((name == "beats" || name == "unit") && std::isfinite(value) && std::floor(value) == value && value >= 1 && value <= 16)
            {
                const auto signature = engine.getTimeSignature();
                engine.setTimeSignature(name == "beats" ? static_cast<int>(value) : signature.beats,
                                        name == "unit" ? static_cast<int>(value) : signature.unit);
            }
            else if (name == "countIn" && empty && (value == 0 || value == 1 || value == 2 || value == 4))
                engine.countInBars = static_cast<int>(value);
            else if (name == "loopBars" && (value == 0 || value == 1 || value == 2 || value == 4 || value == 8 || value == 16))
            {
                if (!engine.setAutoStopBars(static_cast<int>(value)))
                    errorText = "Auto-stop length can only be changed before recording, and must be Off or a supported bar count.";
            }
            else if (name == "inputMode" && (state == LooperEngine::State::empty || state == LooperEngine::State::playing
                                              || state == LooperEngine::State::stopped) && (value == 0 || value == 1 || value == 2))
            {
                engine.inputMode = value == 2 ? LooperEngine::InputMode::stereo
                    : value == 1 ? LooperEngine::InputMode::monoSecond : LooperEngine::InputMode::monoFirst;
                persistAudioSettings();
            }
            else errorText = "Setting unavailable or invalid.";
        }
        else if (action == "bypass")
        {
            if (auto* item = findEffect(event.getProperty("effect", {}).toString()); item != nullptr && item->effect != nullptr)
                item->effect->bypassed = !item->effect->bypassed.load();
        }
        else if (action == "map-parameter" || action == "set-parameter")
        {
            auto* item = findEffect(event.getProperty("effect", {}).toString());
            const auto knob = static_cast<int>(event.getProperty("knob", -1));
            if (item == nullptr || item->effect == nullptr || !juce::isPositiveAndBelow(knob, item->mappings.size()))
                errorText = "That pedal control is no longer available.";
            else if (action == "map-parameter")
            {
                const auto id = event.getProperty("parameter", {}).toString();
                if (id.isNotEmpty() && findPedalParameter(*item->effect->instance, id) == nullptr)
                    errorText = "That plugin parameter is no longer available.";
                else item->mappings.set(knob, id);
            }
            else if (!std::isfinite(value) || value < 0 || value > 1)
                errorText = "Invalid plugin parameter value.";
            else if (auto* parameter = findPedalParameter(*item->effect->instance, item->mappings[knob]))
            {
                parameter->beginChangeGesture();
                parameter->setValueNotifyingHost(static_cast<float>(value));
                parameter->endChangeGesture();
            }
            else errorText = "Assign a plugin parameter to this knob first.";
        }
        else if (action == "save-board" || action == "load-board")
            requestBoardPreset(event.getProperty("name", {}).toString(), action == "save-board");
        else if (action == "add-vst" || action == "change-vst")
        {
            const bool replacing = action == "change-vst";
            if (auto* item = findEffect(event.getProperty("effect", {}).toString());
                item != nullptr && (replacing ? item->effect != nullptr : item->effect == nullptr))
            {
                vstTargetId = item->id;
                vstReplacing = replacing;
                vstPickerError.clear();
                if (!scanned || scanInProgress)
                    startEffectScan();
                else
                {
                    publishVstCatalog();
                    publishWebState();
                }
            }
            else errorText = "That pedal is no longer waiting for a VST3.";
        }
        else if (action == "refresh-vsts")
        {
            if (vstTargetId.isNotEmpty())
            {
                startEffectScan();
            }
            else errorText = "Open the VST3 picker before rescanning.";
        }
        else if (action == "select-vst")
        {
            const auto index = static_cast<int>(event.getProperty("index", -1));
            auto* item = findEffect(vstTargetId);
            if (item == nullptr || (vstReplacing ? item->effect == nullptr : item->effect != nullptr))
            {
                errorText = "That pedal's VST3 state changed before selection.";
                vstTargetId.clear();
                vstReplacing = false;
            }
            else if (scanInProgress)
                vstPickerError = "Wait for the VST3 scan to finish.";
            else if (!juce::isPositiveAndBelow(index, static_cast<int>(discoveredVsts.size())))
                vstPickerError = "Select a VST3 effect from the list.";
            else
            {
                const auto targetId = item->id;
                if (addPlugin(discoveredVsts[static_cast<std::size_t>(index)], cardIndex(item->card),
                              item->templateInfo, targetId, vstReplacing))
                {
                    vstTargetId.clear();
                    vstReplacing = false;
                    vstPickerError.clear();
                }
                else
                    vstPickerError = errorText;
            }
            publishWebState();
        }
        else if (action == "cancel-vst-picker")
        {
            vstTargetId.clear();
            vstReplacing = false;
            vstPickerError.clear();
            publishWebState();
        }
        else if (action == "editor")
        {
            if (auto* item = findEffect(event.getProperty("effect", {}).toString()); item != nullptr && item->effect != nullptr)
                openEditor(*item->effect);
            else errorText = "Add a VST3 to this pedal before opening its editor.";
        }
        else if (action == "remove-effect")
        {
            if (auto* item = findEffect(event.getProperty("effect", {}).toString()))
                juce::MessageManager::callAsync([safe = juce::Component::SafePointer<LooperView>(this), item]
                {
                    if (safe != nullptr) safe->removePlugin(item);
                });
        }
        else if (action == "remove-vst")
        {
            if (auto* item = findEffect(event.getProperty("effect", {}).toString()); item != nullptr && item->effect != nullptr)
                juce::MessageManager::callAsync([safe = juce::Component::SafePointer<LooperView>(this), item]
                {
                    if (safe != nullptr) safe->removeVst(item);
                });
            else errorText = "This pedal has no VST3 to remove.";
        }
        else if (action == "reorder" || action == "move")
        {
            const auto pedalId = event.getProperty("id", {}).toString();
            const auto found = std::find_if(chain.begin(), chain.end(), [&pedalId](const auto* card)
            {
                if (card->getTitle() == "Loop Deck") return pedalId == "looper";
                return false;
            });
            auto* card = found == chain.end() ? nullptr : *found;
            if (card == nullptr)
                for (const auto& item : effectCards)
                    if (item->id == pedalId) { card = &item->card; break; }
            if (card != nullptr)
            {
                const auto source = cardIndex(*card);
                const auto destination = action == "move"
                    ? source + static_cast<int>(event.getProperty("delta", 0))
                    : static_cast<int>(event.getProperty("slot", source));
                settleCard(*card, destination);
            }
        }
        else errorText = "Unknown interface action.";
    }

    static void drawMeter(juce::Graphics& g, float peak, int x, int y)
    {
        g.setColour(juce::Colour(0xff415750));
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y), 120.0f, 5.0f, 2.5f);
        g.setColour(peak >= 1.0f ? juce::Colour(0xffff8c77) : accent);
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                               120.0f * juce::jlimit(0.0f, 1.0f, peak), 5.0f, 2.5f);
        if (peak >= 1.0f)
        {
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.drawText("CLIP", x + 126, y - 5, 40, 16, juce::Justification::left);
        }
    }

    static juce::String stateName(LooperEngine::State state)
    {
        switch (state)
        {
            case LooperEngine::State::empty: return "READY TO RECORD";
            case LooperEngine::State::countingIn: return "COUNT-IN";
            case LooperEngine::State::recording: return "RECORDING";
            case LooperEngine::State::playing: return "PLAYING";
            case LooperEngine::State::waiting: return "WAITING FOR LOOP START";
            case LooperEngine::State::overdubbing: return "RECORDING LAYER";
            case LooperEngine::State::stopped: return "STOPPED";
        }
        return {};
    }

    void setupDial(juce::Slider& slider, juce::Component& parent, const juce::String& name, std::atomic<float>& value,
                   double minimum, double maximum, double step, const juce::String& suffix, bool onCream = false)
    {
        parent.addAndMakeVisible(slider);
        slider.setName(name);
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 73, 20);
        slider.setTextValueSuffix(suffix);
        slider.setRange(minimum, maximum, step);
        slider.setValue(value.load());
        slider.setDoubleClickReturnValue(true, value.load());
        if (onCream) slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff463022));
        slider.onValueChange = [&slider, &value] { value.store(static_cast<float>(slider.getValue())); };
    }

    static void setupToggle(juce::TextButton& button, const juce::String& name, std::atomic<bool>& enabled)
    {
        button.setTitle(name);
        button.setClickingTogglesState(true);
        button.setToggleState(enabled.load(), juce::dontSendNotification);
        button.onClick = [&button, &enabled]
        {
            enabled = button.getToggleState();
            button.setButtonText(enabled.load() ? "ON" : "OFF");
        };
    }

    struct FloatingPanel final : juce::Component
    {
        explicit FloatingPanel(juce::String heading) : title(std::move(heading)) { setTitle(title); }
        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat().reduced(1);
            const bool light = title == "Layers";
            g.setColour(light ? cream : juce::Colour(0xff2e2119));
            g.fillRoundedRectangle(area, 10);
            g.setColour(light ? juce::Colour(0xffd2c5ab) : juce::Colour(0xff967459));
            g.drawRoundedRectangle(area, 10, 1);
            if (light)
            {
                g.setColour(layerMuted);
                g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
                g.drawText("LOOP DECK", 24, 21, getWidth() - 100, 14, juce::Justification::left);
                g.setColour(layerInk);
                g.setFont(juce::FontOptions(25.0f, juce::Font::bold));
                g.drawText(title, 24, 39, getWidth() - 100, 34, juce::Justification::left);
                g.drawHorizontalLine(87, 24.0f, static_cast<float>(getWidth() - 24));
                g.setColour(layerMuted);
                g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
                g.drawText("YOUR TAKES", 24, 96, getWidth() - 48, 12, juce::Justification::left);
                g.setColour(layerInk);
                g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
                g.drawText("Layer mixer", 24, 109, 200, 21, juce::Justification::left);
            }
            else
            {
                g.setColour(cream);
                g.setFont(juce::FontOptions(19.0f, juce::Font::bold));
                g.drawText(title, 16, 20, getWidth() - 38, 29, juce::Justification::centredLeft);
                g.setColour(muted);
                g.setFont(juce::FontOptions(9.0f));
                g.drawFittedText("Installed VST3 effects", 16, 52, getWidth() - 32, 18, juce::Justification::left, 1);
            }
        }
        juce::String title;
    };

    int deckIndex() const { return static_cast<int>(std::find(chain.begin(), chain.end(), &deck) - chain.begin()); }
    int cardIndex(const VintageCard& card) const { return static_cast<int>(std::find(chain.begin(), chain.end(), &card) - chain.begin()); }

    void layoutBoard(VintageCard* moving = nullptr)
    {
        const auto count = static_cast<int>(chain.size());
        const auto width = juce::jlimit(340, 520, (boardViewport.getWidth() - 100) / 3);
        const auto height = juce::jlimit(390, 425, boardViewport.getHeight() - 96);
        const auto used = count * (width + 22) - 22;
        rail.setSize(std::max(boardViewport.getWidth(), used + 44), boardViewport.getHeight());
        const auto first = (rail.getWidth() - used) / 2;
        slots.clear();
        for (int i = 0; i < count; ++i)
        {
        const auto bounds = juce::Rectangle<int>(first + i * (width + 22), 26, width, height);
            slots.push_back(bounds);
            if (chain[static_cast<std::size_t>(i)] != moving)
            {
                juce::Desktop::getInstance().getAnimator().cancelAnimation(chain[static_cast<std::size_t>(i)], false);
                chain[static_cast<std::size_t>(i)]->setBounds(bounds);
            }
        }
        int i = 0;
        for (auto* dial : { &input, &live, &loop, &output })
        {
            const auto column = (width - 40) / 4;
            dial->setBounds(20 + i++ * column + (column - 72) / 2, 91, 72, 60);
        }
        record.setBounds(width / 4 - 64, height - 108, 128, 68);
        stop.setBounds(width * 3 / 4 - 62, height - 108, 124, 68);
        clear.setBounds(width - 91, height - 34, 69, 21);
        for (auto& item : effectCards)
        {
            item->vst.setBounds(width / 2 - 70, height - 146, 140, 30);
            item->bypass.setBounds(width / 2 - 44, height - 111, 88, 66);
            item->remove.setBounds(width - 74, height - 37, 56, 22);
        }
        rail.repaint();
        repaint();
    }

    int slotAt(int x, const VintageCard* exclude = nullptr) const
    {
        int slot = 0;
        for (std::size_t i = 0; i < slots.size(); ++i)
            if (chain[i] != exclude && x > slots[i].getCentreX()) ++slot;
        return slot;
    }

    void markSlot(int index, const VintageCard* exclude = nullptr)
    {
        rail.marker = -1;
        if (index >= 0)
        {
            int seen = 0;
            for (std::size_t i = 0; i < slots.size(); ++i)
                if (chain[i] != exclude && seen++ == index) { rail.marker = slots[i].getX() - 10; break; }
            if (rail.marker < 0 && !slots.empty()) rail.marker = slots.back().getRight() + 10;
        }
        rail.repaint();
    }

    void connectCard(VintageCard& card)
    {
        card.dragMove = [this, &card](auto point, auto down)
        {
            if (!card.lifted)
            {
                juce::Desktop::getInstance().getAnimator().cancelAnimation(&card, true);
                dragScroll = boardViewport.getViewPositionX();
                card.lifted = true;
                card.toFront(false);
            }
            const auto local = boardViewport.getLocalPoint(nullptr, point);
            if (boardViewport.getLocalBounds().contains(local))
            {
                if (local.x < 36) boardViewport.setViewPosition(boardViewport.getViewPositionX() - 16, 0);
                else if (local.x > boardViewport.getWidth() - 36) boardViewport.setViewPosition(boardViewport.getViewPositionX() + 16, 0);
                markSlot(slotAt(rail.getLocalPoint(nullptr, point).x, &card), &card);
            }
            else markSlot(-1);
            card.setBounds(slots[static_cast<std::size_t>(cardIndex(card))].translated(point.x - down.x
                + boardViewport.getViewPositionX() - dragScroll, point.y - down.y - 9));
            card.repaint();
        };
        card.dragEnd = [this, &card](auto point)
        {
            const auto local = boardViewport.getLocalPoint(nullptr, point);
            settleCard(card, boardViewport.getLocalBounds().contains(local) ? slotAt(rail.getLocalPoint(nullptr, point).x, &card) : -1);
        };
        card.dragCancel = [this, &card] { settleCard(card, -1); };
        card.keyMove = [this, &card](int by)
        {
            if (!card.lifted) settleCard(card, cardIndex(card) + by);
        };
    }

    void settleCard(VintageCard& card, int destination)
    {
        const auto original = cardIndex(card);
        const auto lifted = card.lifted;
        if (juce::isPositiveAndBelow(destination, static_cast<int>(chain.size())) && destination != original)
        {
            chain.erase(chain.begin() + original);
            chain.insert(chain.begin() + destination, &card);
            publishOrder();
        }
        card.lifted = false;
        markSlot(-1);
        layoutBoard(lifted ? &card : nullptr);
        if (lifted)
            juce::Desktop::getInstance().getAnimator().animateComponent(&card,
                slots[static_cast<std::size_t>(cardIndex(card))], 1, 200, false, 1, 0);
        else
        {
            const auto bounds = card.getBounds();
            if (bounds.getX() < boardViewport.getViewPositionX() || bounds.getRight() > boardViewport.getViewPositionX() + boardViewport.getWidth())
                boardViewport.setViewPosition(bounds.getX() - 15, 0);
        }
        card.repaint();
    }

    void publishOrder()
    {
        std::vector<PedalHost::Effect*> order;
        int beforeDeck = 0;
        bool passedDeck = false;
        for (auto* card : chain)
        {
            if (card == &deck) { passedDeck = true; continue; }
            for (auto& item : effectCards)
                if (&item->card == card && item->effect != nullptr)
                {
                    order.push_back(item->effect.get());
                    if (!passedDeck) ++beforeDeck;
                }
        }
        host.setOrder(order, beforeDeck);
        for (auto& item : effectCards) item->card.repaint();
    }

    void setLayersOpen(bool show)
    {
        layerPopup.setVisible(show);
        layers.setToggleState(show, juce::dontSendNotification);
        if (show) { showEffects(false); layerPopup.toFront(false); closeLayers.grabKeyboardFocus(); }
        else if (isShowing()) layers.grabKeyboardFocus();
    }

    void showEffects(bool show)
    {
        effectDrawer.setVisible(show);
        effects.setToggleState(show, juce::dontSendNotification);
        if (show)
        {
            layerPopup.setVisible(false);
            layers.setToggleState(false, juce::dontSendNotification);
            if (!scanned) startEffectScan();
            effectDrawer.toFront(false);
            effectSearch.grabKeyboardFocus();
        }
    }

    static juce::File pluginDirectoryFile()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Looper").getChildFile("vst-directories.txt");
    }

    void loadPluginDirectories()
    {
        const auto file = pluginDirectoryFile();
        if (!file.existsAsFile()) return;
        juce::MemoryBlock contents;
        if (!file.loadFileAsData(contents))
        {
            errorText = "Could not read the remembered VST3 directories.";
            return;
        }
        pluginDirectories = juce::StringArray::fromLines(contents.toString());
        pluginDirectories.removeEmptyStrings();
        pluginDirectories.removeDuplicates(true);
    }

    void rememberPluginDirectories(const juce::StringArray& plugins)
    {
        for (const auto& path : plugins)
            pluginDirectories.addIfNotAlreadyThere(juce::File(path).getParentDirectory().getFullPathName(), true);
        const auto file = pluginDirectoryFile();
        const auto result = file.getParentDirectory().createDirectory();
        if (result.failed() || !file.replaceWithText(pluginDirectories.joinIntoString("\n")))
            errorText = "Could not save VST3 directories for future launches.";
    }

    void startEffectScan()
    {
        if (scanInProgress)
        {
            publishWebState();
            return;
        }

        scanInProgress = true;
        vstPickerError.clear();
        publishWebState();
        const juce::Component::SafePointer<LooperView> safe(this);
        juce::FileSearchPath locations;
        for (const auto& directory : pluginDirectories) locations.add(juce::File(directory));
        vstScanPool.addJob([safe, locations]
        {
            PedalHost scanner;
            auto paths = scanner.discover(locations);
            juce::MessageManager::callAsync([safe, paths]
            {
                if (safe != nullptr) safe->rememberPluginDirectories(paths);
            });
            std::vector<juce::PluginDescription> plugins;
            for (const auto& path : paths)
            {
                auto descriptions = scanner.describe(juce::File(path));
                for (auto* description : descriptions)
                    if (!description->isInstrument)
                        plugins.push_back(*description);
            }
            std::stable_sort(plugins.begin(), plugins.end(), [](const auto& a, const auto& b)
            {
                const auto nameOrder = a.name.compareIgnoreCase(b.name);
                return nameOrder != 0 ? nameOrder < 0
                    : a.manufacturerName.compareIgnoreCase(b.manufacturerName) < 0;
            });

            juce::MessageManager::callAsync([safe, paths = std::move(paths), plugins = std::move(plugins)]() mutable
            {
                if (safe == nullptr) return;
                safe->discovered = std::move(paths);
                safe->discoveredVsts = std::move(plugins);
                safe->scanInProgress = false;
                safe->scanned = true;
                safe->filterEffects();
                safe->effectSearch.setTextToShowWhenEmpty(
                    safe->discovered.isEmpty() ? "No VST3s found; use Open File" : "Find a VST3 effect", muted);
                safe->publishVstCatalog();
                safe->publishWebState();
            });
        });
    }

    void filterEffects()
    {
        effectLibrary.paths.clear();
        for (const auto& path : discovered)
            if (juce::File(path).getFileNameWithoutExtension().containsIgnoreCase(effectSearch.getText()))
                effectLibrary.paths.add(path);
        effectList.deselectAllRows();
        effectList.updateContent();
        effectList.repaint();
    }

    void addSelected(int slot)
    {
        const auto row = effectList.getSelectedRow();
        if (juce::isPositiveAndBelow(row, effectLibrary.paths.size())) openPlugin(juce::File(effectLibrary.paths[row]), slot);
        else errorText = "Select a VST3 from Effects, or use Open File.";
    }

    void browsePlugins()
    {
        chooser = std::make_unique<juce::FileChooser>("Select a trusted VST3 effect or bundle", juce::File {}, "*.vst3");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                             | juce::FileBrowserComponent::canSelectDirectories,
            [safe = juce::Component::SafePointer<LooperView>(this)](const auto& dialog)
            {
                if (safe != nullptr && dialog.getResult() != juce::File())
                    safe->openPlugin(dialog.getResult(), safe->deckIndex());
            });
    }

    void openPlugin(const juce::File& file, int slot)
    {
        openPlugin(file, slot, customPedalTemplate());
    }

    void openPlugin(const juce::File& file, int slot, const PedalTemplate& pedal, const juce::String& targetId = {})
    {
        if (targetId.isEmpty() && effectCards.size() >= PedalHost::maxEffects) { errorText = "The board already has 16 pedals."; return; }
        if (targetId.isNotEmpty())
        {
            const auto* target = findEffect(targetId);
            if (target == nullptr || target->effect != nullptr || target->templateInfo.id != pedal.id)
            {
                errorText = "That pedal is no longer waiting for a VST3.";
                return;
            }
        }
        if (!PedalHost::isPlugin(file)) { errorText = "Choose an existing .vst3 file or bundle."; return; }
        auto descriptions = host.describe(file);
        std::vector<juce::PluginDescription> choices;
        for (auto* description : descriptions)
            if (!description->isInstrument) choices.push_back(*description);
        if (choices.empty()) { errorText = "No compatible VST3 audio effect found in " + file.getFileName(); return; }
        if (choices.size() == 1) { addPlugin(choices.front(), slot, pedal, targetId); return; }
        juce::PopupMenu menu;
        for (std::size_t i = 0; i < choices.size(); ++i) menu.addItem(static_cast<int>(i + 1), choices[i].name);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [safe = juce::Component::SafePointer<LooperView>(this), choices = std::move(choices), slot,
             pedalId = pedal.id, targetId](int id)
            {
                if (safe != nullptr && id > 0)
                    if (const auto* selectedPedal = pedalId == "custom" ? &customPedalTemplate() : findPedalTemplate(pedalId))
                        safe->addPlugin(choices[static_cast<std::size_t>(id - 1)], slot, *selectedPedal, targetId);
            });
    }

    EffectCard* createTemplateCard(const PedalTemplate& pedal, int slot)
    {
        if (effectCards.size() >= PedalHost::maxEffects) { errorText = "The board already has 16 pedals."; return nullptr; }
        auto item = std::make_unique<EffectCard>(pedal, nextEffectId++);
        for (std::size_t i = 0; i < pedal.knobs.size(); ++i) item->mappings.add({});
        auto* added = item.get();
        rail.addAndMakeVisible(item->card);
        connectCard(item->card);
        item->vst.onClick = [this, added]
        {
            if (added->effect != nullptr) openEditor(*added->effect);
            else browsePlugin(*added);
        };
        item->bypass.setName("footswitch");
        item->bypass.setClickingTogglesState(true);
        item->bypass.setEnabled(false);
        item->bypass.onClick = [added]
        {
            if (added->effect == nullptr) return;
            added->effect->bypassed = added->bypass.getToggleState();
            added->bypass.setButtonText(added->bypass.getToggleState() ? "ENGAGE" : "BYPASS");
            added->card.repaint();
        };
        item->remove.onClick = [safe = juce::Component::SafePointer<LooperView>(this), added]
        {
            juce::MessageManager::callAsync([safe, added] { if (safe != nullptr) safe->removePlugin(added); });
        };
        item->card.paintFace = [this, added](juce::Graphics& g)
        {
            const auto w = added->card.getWidth(), h = added->card.getHeight();
            const auto display = juce::Rectangle<int>(22, 82, w - 44, 76);
            g.setColour(juce::Colour(0xff292119));
            g.fillRoundedRectangle(display.toFloat(), 8);
            g.setColour(juce::Colour(0xffedc994));
            g.setFont(juce::FontOptions("Consolas", 10.0f, juce::Font::bold));
            g.drawText(added->templateInfo.category.toUpperCase(), display.getX(), display.getY() + 8,
                       display.getWidth(), 15, juce::Justification::centred);
            g.setFont(juce::FontOptions("Consolas", 8.0f, juce::Font::plain));
            g.drawFittedText(added->effect != nullptr ? added->pluginName : "ADD A VST3 PLUGIN",
                             display.getX() + 5, display.getY() + 30, display.getWidth() - 10, 15,
                             juce::Justification::centred, 1);
            for (int i = 0; i < 17; ++i)
            {
                const auto barHeight = 5 + ((i * 13 + 7) % 18);
                g.fillRect(display.getCentreX() - 67 + i * 8, display.getBottom() - 9 - barHeight, 3, barHeight);
            }
            for (std::size_t i = 0; i < added->templateInfo.knobs.size(); ++i)
            {
                const auto column = static_cast<int>(i % 2);
                const auto row = static_cast<int>(i / 2);
                const auto centre = juce::Point<float>(static_cast<float>(w / 2 + (column == 0 ? -82 : 82)),
                                                       static_cast<float>(h - 224 + row * 42));
                g.setColour(juce::Colour(0x900e0907));
                g.fillEllipse(juce::Rectangle<float>(32, 32).withCentre(centre + juce::Point<float>(0, 3)));
                g.setColour(juce::Colour(0xff745942));
                g.fillEllipse(juce::Rectangle<float>(32, 32).withCentre(centre));
                g.setColour(juce::Colour(0xff28211d));
                g.fillEllipse(juce::Rectangle<float>(26, 26).withCentre(centre));
                g.setColour(juce::Colour(0xffefe0c8));
                g.drawLine(centre.x, centre.y, centre.x + (i == 0 ? -3.0f : 3.0f), centre.y - 9.0f, 2.0f);
                g.setFont(juce::FontOptions(7.0f, juce::Font::bold));
                g.drawFittedText(added->templateInfo.knobs[i].toUpperCase(), static_cast<int>(centre.x) - 37,
                                 static_cast<int>(centre.y) + 18, 74, 11, juce::Justification::centred, 1);
            }
            g.setColour(added->effect == nullptr || added->effect->bypassed.load() ? muted.darker(.7f) : accent);
            g.fillEllipse(static_cast<float>(w / 2 - 4), static_cast<float>(h - 155), 8, 8);
        };
        chain.insert(chain.begin() + juce::jlimit(0, static_cast<int>(chain.size()), slot), &item->card);
        effectCards.push_back(std::move(item));
        layoutBoard();
        return added;
    }

    void addTemplate(const PedalTemplate& pedal, int slot)
    {
        if (createTemplateCard(pedal, slot) != nullptr)
        {
            errorText.clear();
            publishOrder();
            publishWebState();
        }
    }

    void browsePlugin(EffectCard& item)
    {
        const auto id = item.id;
        const auto pedalId = item.templateInfo.id;
        chooser = std::make_unique<juce::FileChooser>("Add a VST3 to " + item.templateInfo.name, juce::File {}, "*.vst3");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                             | juce::FileBrowserComponent::canSelectDirectories,
            [safe = juce::Component::SafePointer<LooperView>(this), id, pedalId](const auto& dialog)
            {
                if (safe == nullptr || dialog.getResult() == juce::File()) return;
                auto* target = safe->findEffect(id);
                const auto* pedal = findPedalTemplate(pedalId);
                if (target == nullptr || target->effect != nullptr || pedal == nullptr)
                {
                    safe->errorText = "That pedal is no longer waiting for a VST3.";
                    return;
                }
                safe->openPlugin(dialog.getResult(), safe->cardIndex(target->card), *pedal, id);
            });
    }

    static juce::File boardPresetDirectory()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Looper").getChildFile("Presets");
    }

    void refreshBoardPresets()
    {
        boardPresetNames.clear();
        const auto files = boardPresetDirectory().findChildFiles(juce::File::findFiles, false, "*.board");
        for (const auto& file : files) boardPresetNames.add(file.getFileNameWithoutExtension());
        boardPresetNames.sort(true);
    }

    void requestBoardPreset(const juce::String& rawName, bool saving)
    {
        const auto name = rawName.trim();
        if (name.isEmpty() || name.length() > 80 || name == "." || name == ".."
            || juce::File::createLegalFileName(name) != name || name.endsWithChar('.'))
        {
            errorText = "Enter a valid preset name without file-path characters.";
            publishWebState();
            return;
        }
        const auto file = boardPresetDirectory().getChildFile(name + ".board");
        if (!saving && !file.existsAsFile())
        {
            errorText = "That pedalboard preset is no longer available.";
            publishWebState();
            return;
        }
        const auto proceed = [safe = juce::Component::SafePointer<LooperView>(this), file, name, saving]
        {
            if (safe == nullptr) return;
            if (saving) safe->saveBoard(file, name);
            else safe->loadBoard(file, name);
            safe->publishWebState();
        };
        if (saving && !file.existsAsFile()) { proceed(); return; }
        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,
            saving ? "Replace preset?" : "Load pedalboard preset?",
            saving ? "Replace the saved preset \"" + name + "\"?"
                   : "Loading replaces the current pedalboard and its settings. Recorded audio is kept.",
            saving ? "Replace" : "Load", "Cancel", nullptr,
            juce::ModalCallbackFunction::create([safe = juce::Component::SafePointer<LooperView>(this), proceed](int result)
            {
                if (result != 0) proceed();
                else if (safe != nullptr) safe->publishWebState();
            }));
    }

    bool boardPresetTransportReady()
    {
        const auto state = engine.getState();
        if (state == LooperEngine::State::empty || state == LooperEngine::State::stopped) return true;
        errorText = "Stop the loop before saving or loading a pedalboard preset.";
        return false;
    }

    void saveBoard(const juce::File& file, const juce::String& name)
    {
        if (!boardPresetTransportReady()) return;
        BoardPreset board;
        board.deckIndex = deckIndex();
        board.levels = { engine.inputGain.load(), engine.liveGain.load(), engine.loopGain.load(), engine.outputGain.load() };
        board.footer = { host.utilities.inputDb.load(), host.utilities.gateDb.load(), host.utilities.transpose.load(),
                         host.utilities.doublerMs.load(), host.utilities.outputDb.load() };
        board.gate = host.utilities.gate.load();
        board.doubler = host.utilities.doubler.load();
        board.tuner = host.utilities.tuner.load();
        board.layered = engine.isLayeredLoop();
        board.metronome = engine.metronome.load();
        board.bpm = static_cast<int>(std::round(engine.bpm.load()));
        const auto signature = engine.getTimeSignature();
        board.beats = signature.beats;
        board.unit = signature.unit;
        board.countIn = engine.countInBars.load();
        board.autoStop = engine.autoStopBars.load();
        {
            const juce::ScopedLock guard(devices.getAudioCallbackLock());
            for (auto* card : chain)
                for (const auto& item : effectCards)
                    if (&item->card == card)
                    {
                        BoardPreset::Pedal pedal;
                        pedal.templateId = item->templateInfo.id;
                        pedal.loaded = item->effect != nullptr;
                        if (pedal.loaded)
                        {
                            pedal.plugin.plugin = item->effect->instance->getPluginDescription();
                            pedal.plugin.templateId = pedal.templateId;
                            pedal.plugin.mappings = item->mappings;
                            pedal.plugin.bypassed = item->effect->bypassed.load();
                            item->effect->instance->getStateInformation(pedal.plugin.state);
                        }
                        board.pedals.push_back(std::move(pedal));
                    }
        }
        const auto directoryResult = file.getParentDirectory().createDirectory();
        if (directoryResult.failed()) { errorText = directoryResult.getErrorMessage(); return; }
        const auto result = saveBoardPreset(file, board);
        if (result.failed()) { errorText = result.getErrorMessage(); return; }
        refreshBoardPresets();
        selectedBoardPreset = name;
        errorText = "Saved pedalboard preset: " + name;
    }

    void loadBoard(const juce::File& file, const juce::String& name)
    {
        if (!boardPresetTransportReady()) return;
        BoardPreset board;
        const auto result = loadBoardPreset(file, board);
        if (result.failed()) { errorText = result.getErrorMessage(); return; }
        if ((engine.getState() != LooperEngine::State::empty || engine.getLength() != 0) && (board.layered != engine.isLayeredLoop()
            || board.countIn != engine.countInBars.load() || board.autoStop != engine.autoStopBars.load()))
        {
            errorText = "This preset changes locked loop settings. Save the loop and clear it before loading this preset.";
            return;
        }
        std::vector<std::unique_ptr<PedalHost::Effect>> loaded;
        for (const auto& saved : board.pedals)
        {
            const auto* pedal = saved.templateId == "custom" ? &customPedalTemplate() : findPedalTemplate(saved.templateId);
            if (pedal == nullptr || (saved.loaded && saved.plugin.mappings.size() != static_cast<int>(pedal->knobs.size())))
            {
                errorText = "The preset contains an unknown pedal or invalid knob assignments. The current board was kept.";
                return;
            }
            if (!saved.loaded) { loaded.push_back(nullptr); continue; }
            auto description = saved.plugin.plugin;
            if (!PedalHost::isPlugin(juce::File(description.fileOrIdentifier)))
            {
                const auto match = std::find_if(discoveredVsts.begin(), discoveredVsts.end(), [&description](const auto& candidate)
                {
                    return candidate.uniqueId == description.uniqueId && candidate.name == description.name
                        && candidate.manufacturerName == description.manufacturerName;
                });
                if (match == discoveredVsts.end())
                {
                    errorText = "A preset VST3 was not found: " + description.name + ". The current board was kept.";
                    return;
                }
                description = *match;
            }
            juce::String loadError;
            auto effect = host.load(description, loadError);
            if (effect == nullptr) { errorText = "Could not load " + description.name + ": " + loadError; return; }
            effect->instance->setStateInformation(saved.plugin.state.getData(), static_cast<int>(saved.plugin.state.getSize()));
            for (const auto& mapping : saved.plugin.mappings)
                if (mapping.isNotEmpty() && findPedalParameter(*effect->instance, mapping) == nullptr)
                {
                    errorText = "A saved plugin parameter is unavailable. The current board was kept.";
                    return;
                }
            effect->bypassed = saved.plugin.bypassed;
            loaded.push_back(std::move(effect));
        }
        if (!boardPresetTransportReady()) return;
        chain = { &deck };
        publishOrder();
        for (const auto& item : effectCards)
            juce::Desktop::getInstance().getAnimator().cancelAnimation(&item->card, false);
        effectCards.clear();
        for (std::size_t i = 0; i < board.pedals.size(); ++i)
        {
            const auto& saved = board.pedals[i];
            const auto* pedal = saved.templateId == "custom" ? &customPedalTemplate() : findPedalTemplate(saved.templateId);
            auto* item = createTemplateCard(*pedal, static_cast<int>(chain.size()));
            item->effect = std::move(loaded[i]);
            if (item->effect != nullptr)
            {
                item->pluginName = item->effect->instance->getPluginDescription().name;
                item->mappings = saved.plugin.mappings;
                updatePedalParameters(*item);
                item->bypass.setEnabled(true);
                item->bypass.setToggleState(saved.plugin.bypassed, juce::dontSendNotification);
                item->bypass.setButtonText(saved.plugin.bypassed ? "ENGAGE" : "BYPASS");
                item->vst.setButtonText("OPEN VST3");
            }
        }
        chain.erase(chain.begin());
        chain.insert(chain.begin() + board.deckIndex, &deck);
        engine.inputGain = static_cast<float>(board.levels[0]);
        engine.liveGain = static_cast<float>(board.levels[1]);
        engine.loopGain = static_cast<float>(board.levels[2]);
        engine.outputGain = static_cast<float>(board.levels[3]);
        host.utilities.inputDb = static_cast<float>(board.footer[0]);
        host.utilities.gateDb = static_cast<float>(board.footer[1]);
        host.utilities.transpose = static_cast<float>(board.footer[2]);
        host.utilities.doublerMs = static_cast<float>(board.footer[3]);
        host.utilities.outputDb = static_cast<float>(board.footer[4]);
        host.utilities.gate = board.gate;
        host.utilities.doubler = board.doubler;
        host.utilities.tuner = board.tuner;
        engine.metronome = board.metronome;
        engine.bpm = board.bpm;
        engine.setTimeSignature(board.beats, board.unit);
        if (engine.getState() == LooperEngine::State::empty && engine.getLength() == 0)
        {
            engine.setLayeredLoop(board.layered);
            engine.countInBars = board.countIn;
            engine.setAutoStopBars(board.autoStop);
        }
        publishOrder();
        selectedBoardPreset = name;
        errorText = "Loaded pedalboard preset: " + name;
        layoutBoard();
    }

    void choosePedalPreset(const juce::String& id, bool saving)
    {
        auto* item = findEffect(id);
        if (pedalChooserOpen || item == nullptr || (saving && item->effect == nullptr))
        {
            errorText = "Load a VST3 on this pedal before saving its preset.";
            return;
        }
        pedalChooserOpen = true;
        pedalChooser = std::make_unique<juce::FileChooser>(
            saving ? "Save VST pedal preset" : "Load VST pedal preset", juce::File {}, "*.pedal");
        pedalChooser->launchAsync(juce::FileBrowserComponent::canSelectFiles
            | (saving ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting
                      : juce::FileBrowserComponent::openMode),
            [safe = juce::Component::SafePointer<LooperView>(this), id, saving](const auto& dialog)
            {
                if (safe == nullptr) return;
                safe->pedalChooserOpen = false;
                auto file = dialog.getResult();
                if (file == juce::File()) return;
                if (saving && file.getFileExtension().isEmpty())
                {
                    file = file.withFileExtension(".pedal");
                    if (file.exists())
                    {
                        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Replace file?",
                            file.getFileName() + " already exists.", "Replace", "Cancel", nullptr,
                            juce::ModalCallbackFunction::create([safe, id, file](int result)
                            {
                                if (safe != nullptr && result != 0) safe->savePedal(id, file);
                            }));
                        return;
                    }
                }
                if (saving) safe->savePedal(id, file);
                else safe->loadPedal(id, file);
            });
    }

    void savePedal(const juce::String& id, const juce::File& file)
    {
        auto* item = findEffect(id);
        if (item == nullptr || item->effect == nullptr)
        {
            errorText = "That pedal no longer has a VST3 to save.";
            return;
        }
        PedalPreset preset;
        preset.plugin = item->effect->instance->getPluginDescription();
        preset.templateId = item->templateInfo.id;
        preset.mappings = item->mappings;
        preset.bypassed = item->effect->bypassed.load();
        {
            const juce::ScopedLock guard(devices.getAudioCallbackLock());
            item->effect->instance->getStateInformation(preset.state);
        }
        const auto result = savePedalPreset(file, preset);
        errorText = result.failed() ? result.getErrorMessage() : "Saved pedal preset: " + file.getFileName();
    }

    void loadPedal(const juce::String& id, const juce::File& file)
    {
        auto preset = std::make_shared<PedalPreset>();
        const auto result = loadPedalPreset(file, *preset);
        if (result.failed()) { errorText = result.getErrorMessage(); return; }
        const auto* pedal = preset->templateId == "custom" ? &customPedalTemplate() : findPedalTemplate(preset->templateId);
        if (pedal == nullptr || preset->mappings.size() != static_cast<int>(pedal->knobs.size()))
        {
            errorText = "This pedal preset has an unknown template or invalid knob assignments.";
            return;
        }
        auto* item = findEffect(id);
        if (item == nullptr) { errorText = "That pedal is no longer available."; return; }
        const auto apply = [safe = juce::Component::SafePointer<LooperView>(this), id, preset]
        {
            if (safe != nullptr) safe->applyPedalPreset(id, *preset);
        };
        if (item->effect == nullptr) { apply(); return; }
        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Replace VST pedal?",
            "Loading this preset replaces the current plugin and its settings. Save the current pedal first if needed.",
            "Replace", "Cancel", nullptr, juce::ModalCallbackFunction::create([apply](int resultCode)
            {
                if (resultCode != 0) apply();
            }));
    }

    void applyPedalPreset(const juce::String& id, const PedalPreset& preset)
    {
        auto* item = findEffect(id);
        const auto* pedal = preset.templateId == "custom" ? &customPedalTemplate() : findPedalTemplate(preset.templateId);
        if (item == nullptr || pedal == nullptr) { errorText = "That pedal is no longer available."; return; }
        auto description = preset.plugin;
        if (!PedalHost::isPlugin(juce::File(description.fileOrIdentifier)))
        {
            const auto match = std::find_if(discoveredVsts.begin(), discoveredVsts.end(), [&description](const auto& candidate)
            {
                return candidate.uniqueId == description.uniqueId && candidate.name == description.name
                    && candidate.manufacturerName == description.manufacturerName;
            });
            if (match == discoveredVsts.end())
            {
                errorText = "The VST3 used by this preset was not found. Install it or rescan its folder.";
                return;
            }
            description = *match;
        }
        juce::String loadError;
        auto effect = host.load(description, loadError);
        if (effect == nullptr) { errorText = "Could not load preset VST3: " + loadError; return; }
        effect->instance->setStateInformation(preset.state.getData(), static_cast<int>(preset.state.getSize()));
        for (const auto& mapping : preset.mappings)
            if (mapping.isNotEmpty() && findPedalParameter(*effect->instance, mapping) == nullptr)
            {
                errorText = "A saved knob parameter is unavailable in this plugin version; the current pedal was kept.";
                return;
            }
        effect->bypassed = preset.bypassed;
        auto previous = std::move(item->effect);
        item->effect = std::move(effect);
        updatePedalParameters(*item);
        item->templateInfo = *pedal;
        item->mappings = preset.mappings;
        item->pluginName = description.name;
        item->bypass.setEnabled(true);
        item->bypass.setToggleState(preset.bypassed, juce::dontSendNotification);
        item->bypass.setButtonText(preset.bypassed ? "ENGAGE" : "BYPASS");
        item->vst.setButtonText("OPEN VST3");
        publishOrder();
        previous.reset();
        errorText.clear();
        rememberPluginDirectories(juce::StringArray { description.fileOrIdentifier });
        layoutBoard();
        publishWebState();
    }

    bool addPlugin(const juce::PluginDescription& description, int slot, const PedalTemplate& pedal,
                   const juce::String& targetId = {}, bool replaceExisting = false)
    {
        auto* target = targetId.isNotEmpty() ? findEffect(targetId) : nullptr;
        if (targetId.isNotEmpty() && (target == nullptr
                                      || (replaceExisting ? target->effect == nullptr : target->effect != nullptr)
                                      || target->templateInfo.id != pedal.id))
        {
            errorText = "That pedal's VST3 state changed before selection.";
            return false;
        }
        if (target == nullptr && effectCards.size() >= PedalHost::maxEffects)
        {
            errorText = "The board already has 16 pedals.";
            return false;
        }
        auto effect = host.load(description, errorText);
        if (effect == nullptr)
        {
            errorText = "Could not load " + description.name + ": " + errorText;
            return false;
        }
        if (target == nullptr)
            target = createTemplateCard(pedal, slot);
        if (target == nullptr) return false;
        errorText.clear();
        rememberPluginDirectories(juce::StringArray { description.fileOrIdentifier });
        auto previousEffect = std::move(target->effect);
        target->effect = std::move(effect);
        updatePedalParameters(*target);
        target->mappings.clear();
        for (std::size_t i = 0; i < target->templateInfo.knobs.size(); ++i) target->mappings.add({});
        target->pluginName = description.name;
        target->bypass.setEnabled(true);
        target->bypass.setToggleState(false, juce::dontSendNotification);
        target->bypass.setButtonText("BYPASS");
        target->vst.setButtonText("OPEN VST3");
        target->card.repaint();
        publishOrder();
        previousEffect.reset();
        layoutBoard();
        publishWebState();
        if (!boardViewport.getViewArea().contains(target->card.getBounds().getCentre()))
            boardViewport.setViewPosition(target->card.getX() - 15, 0);
        return true;
    }

    void removeVst(EffectCard* item)
    {
        if (item == nullptr || item->effect == nullptr) return;
        auto previousEffect = std::move(item->effect);
        updatePedalParameters(*item);
        item->pluginName.clear();
        item->bypass.setEnabled(false);
        item->bypass.setToggleState(false, juce::dontSendNotification);
        item->bypass.setButtonText("BYPASS");
        item->vst.setButtonText("ADD VST3");
        publishOrder();
        previousEffect.reset();
        publishWebState();
    }

    void removePlugin(EffectCard* item)
    {
        const auto found = std::find_if(effectCards.begin(), effectCards.end(), [item](const auto& candidate) { return candidate.get() == item; });
        if (found == effectCards.end()) return;
        chain.erase(std::find(chain.begin(), chain.end(), &item->card));
        publishOrder();
        juce::Desktop::getInstance().getAnimator().cancelAnimation(&item->card, false);
        effectCards.erase(found);
        layoutBoard();
    }

    void openEditor(PedalHost::Effect& effect)
    {
        struct EditorWindow final : juce::DocumentWindow
        {
            explicit EditorWindow(const juce::String& title) : DocumentWindow(title, background, closeButton) { setUsingNativeTitleBar(true); }
            void closeButtonPressed() override { setVisible(false); }
        };
        if (!effect.editor)
        {
            auto* pluginEditor = effect.instance->hasEditor() ? effect.instance->createEditorIfNeeded() : nullptr;
            if (pluginEditor == nullptr) pluginEditor = new juce::GenericAudioProcessorEditor(*effect.instance);
            effect.editor = std::make_unique<EditorWindow>(effect.instance->getName());
            effect.editor->setContentOwned(pluginEditor, true);
            effect.editor->setResizable(pluginEditor->isResizable(), false);
            effect.editor->centreWithSize(effect.editor->getWidth(), effect.editor->getHeight());
        }
        effect.editor->setVisible(true);
        effect.editor->toFront(true);
    }

    void updateTuner()
    {
        const auto hz = host.utilities.readPitch();
        tunerNote.clear();
        tunerCents = 0;
        if (!host.utilities.tuner.load()) { tuner.setButtonText("TUNER / OFF"); return; }
        if (!deviceReady.load() || hz <= 0) { tuner.setButtonText("TUNER / PLAY A NOTE"); return; }
        const auto pitchNumber = 69.0 + 12.0 * std::log2(static_cast<double>(hz) / 440.0);
        const auto note = static_cast<int>(std::round(pitchNumber));
        const auto cents = static_cast<int>(std::round((pitchNumber - note) * 100.0));
        static const char* names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        tunerNote = juce::String(names[note % 12]) + juce::String(note / 12 - 1);
        tunerCents = cents;
        tuner.setButtonText(tunerNote + "   "
            + (std::abs(cents) <= 3 ? juce::String("IN TUNE") : (cents > 0 ? "+" : "") + juce::String(cents) + " cents"));
    }

    void send(LooperEngine::Command command, int layer = -1)
    {
        if (!deviceReady.load())
        {
            errorText = "Audio device unavailable. Open Settings > Audio to reconnect.";
            return;
        }
        if (settingsWindow != nullptr)
        {
            errorText = "Close Settings before using the looper.";
            return;
        }
        if (!engine.enqueue(command, layer))
            errorText = "Too many commands; please try again.";
    }

    bool loopFilesAvailable(bool saving)
    {
        const auto state = engine.getState();
        if (state != LooperEngine::State::empty && state != LooperEngine::State::stopped)
        {
            errorText = "Stop the loop before saving, opening, importing, or exporting.";
            return false;
        }
        if (engine.getSampleRate() <= 0.0 || (saving && engine.getLayerCount() == 0))
        {
            errorText = saving ? "Record a loop before saving or exporting." : "Connect an audio device before opening a loop.";
            return false;
        }
        return true;
    }

    void chooseLoopFile(const juce::String& action)
    {
        const bool saving = action == "save-project" || action == "export-wav";
        const bool wav = action == "import-wav" || action == "export-wav";
        if (loopFileDialogOpen || !loopFilesAvailable(saving)) return;
        loopFileDialogOpen = true;
        loopFileStatus.clear();
        loopChooser = std::make_unique<juce::FileChooser>(
            action == "save-project" ? "Save layered loop" : action == "load-project" ? "Open layered loop"
                : wav && saving ? "Export loop mix as WAV" : "Import WAV as a new loop",
            juce::File {}, wav ? "*.wav" : "*.loop");
        loopChooser->launchAsync(juce::FileBrowserComponent::canSelectFiles
            | (saving ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting
                      : juce::FileBrowserComponent::openMode),
            [safe = juce::Component::SafePointer<LooperView>(this), action, saving, wav](const auto& dialog)
            {
                if (safe == nullptr) return;
                safe->loopFileDialogOpen = false;
                auto file = dialog.getResult();
                if (file == juce::File()) return;
                if (saving && file.getFileExtension().isEmpty())
                {
                    file = file.withFileExtension(wav ? ".wav" : ".loop");
                    if (file.exists())
                    {
                        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Replace file?",
                            file.getFileName() + " already exists.", "Replace", "Cancel", nullptr,
                            juce::ModalCallbackFunction::create([safe, action, file](int result)
                            {
                                if (safe != nullptr && result != 0) safe->processLoopFile(action, file);
                            }));
                        return;
                    }
                }
                safe->processLoopFile(action, file);
            });
        publishWebState();
    }

    void processLoopFile(const juce::String& action, const juce::File& file)
    {
        const bool saving = action == "save-project" || action == "export-wav";
        auto project = std::make_shared<LooperEngine::Project>();
        {
            const juce::ScopedLock guard(devices.getAudioCallbackLock());
            engine.process(nullptr, 0, nullptr, 0, 0);
            if (!loopFilesAvailable(saving)) return;
            if (saving && !engine.captureProject(*project))
            {
                errorText = "Could not capture the stopped loop.";
                return;
            }
        }
        if (saving)
        {
            const auto result = action == "save-project" ? LoopAudioFile::saveProject(file, *project)
                                                        : LoopAudioFile::exportWav(file, *project);
            if (result.failed()) errorText = result.getErrorMessage();
            else { errorText.clear(); loopFileStatus = "Saved " + file.getFileName(); }
            return;
        }
        auto result = action == "load-project" ? LoopAudioFile::loadProject(file, *project)
            : LoopAudioFile::importWav(file, engine.getSampleRate(), *project);
        if (result.wasOk()) result = LoopAudioFile::resample(*project, engine.getSampleRate());
        if (result.failed()) { errorText = result.getErrorMessage(); return; }
        const auto apply = [safe = juce::Component::SafePointer<LooperView>(this), project, file]
        {
            if (safe == nullptr) return;
            const juce::ScopedLock guard(safe->devices.getAudioCallbackLock());
            safe->engine.process(nullptr, 0, nullptr, 0, 0);
            if (!safe->engine.restoreProject(*project))
                safe->errorText = "Could not open the loop. Stop transport and check the device sample rate; the current loop is unchanged.";
            else
            {
                safe->errorText.clear();
                safe->loopFileStatus = "Opened " + file.getFileName();
            }
        };
        if (!hasLoop()) { apply(); return; }
        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Replace current loop?",
            "Opening this file replaces all current layers. Save them first if you want to keep them.",
            "Replace", "Cancel", nullptr, juce::ModalCallbackFunction::create([apply](int resultCode)
            {
                if (resultCode != 0) apply();
            }));
    }

    void confirmClear()
    {
        if (!hasLoop()) return;
        juce::Component::SafePointer<LooperView> safe(this);
        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Clear loop?",
            "This permanently removes the current loop.", "Clear", "Cancel", nullptr,
            juce::ModalCallbackFunction::create([safe](int result)
            {
                if (safe != nullptr && result != 0)
                    safe->clearLoop();
            }));
    }

    void confirmDeleteLayer(int layer)
    {
        juce::Component::SafePointer<LooperView> safe(this);
        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Delete layer?",
            "Permanently delete Layer " + juce::String(layer + 1)
                + "? Other layers are kept. This cannot be undone.",
            "Delete", "Cancel", nullptr,
            juce::ModalCallbackFunction::create([safe, layer](int result)
            {
                if (safe != nullptr && result != 0)
                    safe->send(LooperEngine::Command::removeLayer, layer);
            }));
    }

    void clearLoop()
    {
        if (deviceReady.load())
        {
            send(LooperEngine::Command::clear);
            return;
        }
        devices.removeAudioCallback(this);
        if (engine.enqueue(LooperEngine::Command::clear))
            engine.process(nullptr, 0, nullptr, 0, 0);
        else
            errorText = "Could not clear the loop; please try again.";
        devices.addAudioCallback(this);
    }

    void showSettings(bool showAudio = false)
    {
        if (settingsWindow != nullptr)
        {
            settingsWindow->setVisible(true);
            settingsWindow->toFront(true);
            return;
        }
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(new SettingsPanel(engine, devices,
            [safe = juce::Component::SafePointer<LooperView>(this)](bool deviceChanged)
            {
                if (safe == nullptr) return;
                auto* device = safe->devices.getCurrentAudioDevice();
                if (deviceChanged && device != nullptr && device->isOpen()) safe->preserveRememberedAudioDevice = false;
                safe->persistAudioSettings();
            }, showAudio));
        options.dialogTitle = "Settings";
        options.dialogBackgroundColour = cream;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        settingsWindow = options.launchAsync();
    }

    void timerCallback() override
    {
        const auto state = engine.getState();
        if (auto* device = devices.getCurrentAudioDevice())
            deviceText = devices.getCurrentAudioDeviceType() + " / " + device->getName()
                + " / " + juce::String(device->getCurrentSampleRate(), 0) + " Hz / "
                + juce::String(device->getCurrentBufferSizeSamples()) + " samples"
                + "    |    SPACE record    S stop";
        if (deviceReady.load() && errorText.startsWith("Audio device unavailable"))
            errorText.clear();
        record.setButtonText(state == LooperEngine::State::empty ? "RECORD"
            : state == LooperEngine::State::countingIn ? "CANCEL COUNT-IN"
            : state == LooperEngine::State::recording ? "FINISH LOOP"
            : state == LooperEngine::State::stopped ? "PLAY"
            : state == LooperEngine::State::playing ? engine.isLayeredLoop() ? "ADD LAYER" : "OVERDUB" : "CANCEL TAKE");
        record.setEnabled(settingsWindow == nullptr);
        clear.setEnabled(hasLoop());
        const auto count = engine.getLayerCount();
        layers.setButtonText("Layers (" + juce::String(count) + ")");
        if (visibleLayerCount != count)
        {
            visibleLayerCount = count;
            layerList.setSize(layerList.getWidth(), count * 58);
        }
        emptyMixer.setVisible(count == 0);
        for (auto& row : rows) row->refresh();
        switch (engine.takeNotice())
        {
            case LooperEngine::Notice::recordingLimit:
                errorText = "Recording limit reached; the first layer is preserved and playing."; break;
            case LooperEngine::Notice::layerLimit:
                errorText = "All 8 layers are used. Rerecord an existing layer or clear the loop."; break;
            case LooperEngine::Notice::invalidCommand:
                errorText = "Finish or cancel the current take before modifying layers."; break;
            case LooperEngine::Notice::invalidSetting:
                errorText = "Setting unavailable: finish the count-in or clear the loop before changing recording mode."; break;
            case LooperEngine::Notice::none: break;
        }
        if (!deviceReady.load() && errorText.isEmpty())
            errorText = hasLoop()
                ? "Audio device unavailable or sample rate changed. Clear the loop to change settings."
                : "Audio device unavailable. Open Settings > Audio to reconnect.";
        if (++tunerTick % 3 == 0) updateTuner();
        deck.repaint();
        repaint();
        publishWebState();
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        const auto ok = engine.prepare(device->getCurrentSampleRate());
        deviceReady.store(false);
        if (ok)
        {
            const auto in = device->getActiveInputChannels().countNumberOfSetBits();
            const auto out = device->getActiveOutputChannels().countNumberOfSetBits();
            if (in > 0 && out > 0)
            {
                host.prepare(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
                deviceReady.store(true);
            }
        }
    }

    void audioDeviceStopped() override { deviceReady.store(false); host.stop(); }

    void audioDeviceIOCallbackWithContext(const float* const* inputs, int inputChannels,
        float* const* outputs, int outputChannels, int frames, const juce::AudioIODeviceCallbackContext&) override
    {
        if (deviceReady.load())
            host.process(engine, inputs, inputChannels, outputs, outputChannels, frames);
        else
            for (int ch = 0; ch < outputChannels; ++ch)
                if (outputs[ch] != nullptr) juce::FloatVectorOperations::clear(outputs[ch], frames);
    }

    VintageLook vintage;
    LooperEngine engine;
    PedalHost host;
    juce::AudioDeviceManager devices;
    std::atomic<bool> deviceReady { false };
    juce::String errorText, audioError, deviceText { "SPACE  record / overdub     S  stop" };
    bool audioPanelOpen = false;
    BoardRail rail;
    juce::Viewport boardViewport;
    VintageCard deck { "Loop Deck", true };
    std::vector<VintageCard*> chain;
    std::vector<juce::Rectangle<int>> slots;
    std::vector<std::unique_ptr<EffectCard>> effectCards;
    int dragScroll = 0, tunerTick = 0, tunerCents = 0;
    juce::String tunerNote;
    juce::TextButton record { "RECORD" }, stop { "STOP" }, clear { "CLEAR" }, settings { "Settings ↗" },
        layers { "Layers (0)" }, effects { "Effects" }, tuner { "TUNER / PLAY A NOTE" }, gateToggle { "OFF" }, doubleToggle { "OFF" };
    juce::Slider input, live, loop, output;
    juce::Slider inTrim, gateDb, pitch, delay, outTrim;
    FloatingPanel layerPopup { "Layers" }, effectDrawer { "Make some space." };
    juce::TextButton closeLayers { "CLOSE" }, closeEffects { "X" }, browse { "OPEN VST3 FILE / BUNDLE" },
                     refreshEffects { "RESCAN" }, addPre { "+ PRE-LOOP" }, addPost { "+ POST-LOOP" };
    EffectLibrary effectLibrary;
    juce::ListBox effectList { "Installed VST3 effects", &effectLibrary };
    juce::TextEditor effectSearch;
    juce::StringArray discovered;
    juce::StringArray pluginDirectories;
    std::vector<juce::PluginDescription> discoveredVsts;
    juce::String vstTargetId, vstPickerError;
    juce::String selectedBoardPreset;
    juce::StringArray boardPresetNames;
    bool scanned = false, scanInProgress = false, vstReplacing = false;
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<juce::FileChooser> loopChooser;
    std::unique_ptr<juce::FileChooser> pedalChooser;
    bool pedalChooserOpen = false;
    std::unique_ptr<juce::XmlElement> rememberedAudioDevice;
    bool preserveRememberedAudioDevice = false;
    bool loopFileDialogOpen = false;
    juce::String loopFileStatus;
    juce::Label emptyMixer;
    juce::Component layerList;
    juce::Viewport layerViewport;
    std::array<std::unique_ptr<LayerRow>, LooperEngine::maxLayers> rows;
    int visibleLayerCount = -1, nextEffectId = 1;
    juce::Component::SafePointer<juce::DialogWindow> settingsWindow;
    juce::WebBrowserComponent webView;
    juce::ThreadPool vstScanPool { 1 };
};

class MainWindow final : public juce::DocumentWindow
{
public:
    MainWindow() : DocumentWindow("Looper", background, DocumentWindow::closeButton | DocumentWindow::minimiseButton)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new LooperView(), true);
        setResizable(false, false);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }
    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
    LooperView* view() const { return static_cast<LooperView*>(getContentComponent()); }
};

class LooperApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Looper"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    void initialise(const juce::String&) override { window = std::make_unique<MainWindow>(); }
    void shutdown() override { window.reset(); }
    void systemRequestedQuit() override
    {
        if (window == nullptr || !window->view()->hasLoop()) { quit(); return; }
        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Close Looper?",
            "Any loop changes since your last project save will be lost.", "Close", "Cancel", nullptr,
            juce::ModalCallbackFunction::create([](int result)
            {
                if (result != 0) juce::JUCEApplication::getInstance()->quit();
            }));
    }
private:
    std::unique_ptr<MainWindow> window;
};
}

START_JUCE_APPLICATION(LooperApplication)
