# The Looper Board Archetype

Vintage Deck is the standalone Windows looper interface. It combines the eight-layer looper, tuner, and board controls with a movable serial chain of VST3 effects. **Settings > Visual** selects Default, Fixed deck, Moveable deck, or Three pages and toggles patch cables. Default is the original full-size board; the chosen view is remembered. The looper timing controls remain in the top toolbar.

## Run

Open `build\Looper_artefacts\Release\The Looper Board Archetype.exe` after building. Connect an audio interface before launch, select its input and output in **Settings > Audio**, start with a low output level, and use headphones. Disable hardware direct monitoring if it doubles the software-monitored signal.

The Vintage Deck window has a fixed 1500 × 980 size so the complete pedalboard layout remains visible.

### ASIO and audio settings

In **Settings > Audio**, choose **ASIO** under audio device type, then select your installed driver, such as **Focusrite USB ASIO**. The app discovers registered Windows ASIO drivers; it does not install drivers. Use your interface manufacturer's 64-bit driver. Use **Refresh** to rescan after connecting a device.

Select one input for a mono instrument or two inputs for stereo, and select both headphone output channels (normally outputs 1 and 2). The channel lists show the interface's actual channel names, including loopback inputs where provided. Select a supported sample rate and buffer size (try 128 samples), use **Test** at a safe output volume, and open **Control Panel** or **Reset Device** when the driver supports them. Windows Audio modes and DirectSound remain available.

MIDI input devices can also be enabled here; MIDI looper control mappings are not implemented yet. Sample-rate and buffer choices come from the active driver, so not every setting is available on every device.

Audio device, channel, sample-rate, buffer, MIDI input, and input-mode choices are saved in `%APPDATA%\Looper\audio-settings.xml` and restored at launch. If a saved device is unavailable, choose an available one in Audio settings; its remembered preference is not replaced just by changing input mode.

### Vintage Deck and effects

- **Effects** opens folders for Dynamics, Gain, Filter, Pitch, Modulation, Delay, Reverb, and Amp & Cab. Drag a pedal template onto the board, then use **Add VST3** inside its display and search by plugin or manufacturer. The display also contains **Open VST3**, **Change**, and **Remove VST** for a loaded plugin. Use the selector under each pedal knob to assign an available plugin parameter, then drag the knob or use arrow keys to adjust it. Unassigned knobs are disabled.
- The header preset selector and save icon store the whole pedalboard in `%APPDATA%\Looper\Presets`: pedal order and templates, VST3 states, bypass and knob assignments, deck and board controls, and looper timing settings. Stop transport before saving or loading. Loading confirms before replacing the board; if a plugin or saved parameter is unavailable, the current board is kept. The matching VST3s must be installed. Existing recordings are not part of a preset; save them separately as a loop project. If a recorded loop locks layered mode, count-in or auto-stop and the preset changes those settings, save the loop and clear it before loading. Capturing plugin state may briefly interrupt audio.
- Older individual `.pedal` preset files are not changed, but this version no longer provides a control to load them.
- Click outside the Effects drawer, its X, or press Escape to close it. VST3 discovery starts in the background on every launch; **Rescan** refreshes the installed-plugin list. Standard VST3 folders and remembered folders are searched. Discovered plugin directories, including those of manually loaded plugins, are stored in `%APPDATA%\Looper\vst-directories.txt` for future launches. Plugins are scanned in-process; only install/load plugins you trust.
- **Visual > Default** shows the original full-size pedalboard. **01 / Fixed deck** keeps the Loop Deck centered between compact pre- and post-effects. **02 / Moveable deck** places a compact Loop Deck in the chain, with quick looper controls in the footer. **03 / Three pages** switches between pre-effects, the Loop Deck, and post-effects; drag an effect to the board edge to move it across the Loop Deck. Open a compact pedal's lower half to access its real controls.
- Drag a pedal template from **Effects** onto the board to add it. Drag a pedal header to pick it up and drop it into a new signal-chain slot. This works for the Loop Deck in view 02 and effects in all views. Release outside the board to cancel. Keyboard users can focus a movable header and use Left/Right. **Bypass** skips a loaded plugin and turns off its pedal light without dimming the pedal; **Remove** unloads that instance.
- You can still drop a `.vst3` directly from Explorer onto the board. Only load plugins from publishers you trust: they run in Looper's process.
- **Before** the deck, effects are recorded into new takes. **After** the deck, effects change what you hear without altering the recorded layers. The metronome is added after the effects and doubler.
- The top-left **tuner** listens to the live input before gate, transpose, and effects. Play one note at a time; it highlights the nearest note, displays its octave and cents, and lights the flat, in-tune, or sharp indicator. Click to turn it off.
- Tuner dots represent the sharp notes A#, C#, D#, F#, and G# and highlight independently. There are no dots between B/C or E/F.
- The footer contains **Input** gain (dB), **Gate** threshold (dB), and **Transpose** (semitones) on the left, **Doubler** delay (ms) and **Output** gain (dB) on the right. Gate and Doubler have independent on/off switches. These are separate from the four deck dials. Drag a dial vertically or edit its numeric field; double-click the dial to reset it.

### Input and layers

- Input mode is in **Settings > Audio**. **Mono: first input (centered)** is the default. It sends the first selected input to both ears, even if two inputs are enabled but your guitar is plugged into only the first. Choose **Mono: second input** for the second selected input (or the sole input if only one is selected). Choose **Stereo** only for an actual stereo source.
- The **Layer** control in the top toolbar defaults to on. Turn it off before recording for classic single-layer overdubs: **Overdub** adds the incoming phrase into Layer 1 rather than creating a separate layer. Each take starts at the next loop boundary and completes after one cycle. **Rerecord** replaces rather than adds. The mode is locked while a loop or count-in exists; Clear first to change it.
- Click **Record**, play a phrase, and click **Finish Loop** to set its length and create Layer 1. **Add Layer** starts a new, full-length take at the next loop boundary and finishes automatically after one cycle. A cancelled take does not become a layer.
- **Layers** opens a mixer popup that shows only completed recorded layers, never empty slots or unfinished new takes. An existing layer remains visible while it is rerecorded.
- Each layer has **Volume**, **Pan** (-1 left, 0 center, +1 right), **Solo**, **On/Off**, **Rerecord**, and **Delete**. For stereo material, pan behaves as a left/right balance. Double-click pan to center it.
- Solo isolates the selected active layer(s) among recorded layers; it does not mute live monitoring or the metronome. Disabled layers are silent and do not impose solo on the other layers. Volume/pan changes do not alter recorded audio.
- **Rerecord** captures one full cycle at the next loop boundary and replaces only that layer. The old take continues playing until the replacement completes; **Cancel Take** or **Stop** preserves it.
- **Delete** removes the selected layer after confirmation, keeping the remaining audio and mixer settings. Remaining layers renumber in order; deleting the last layer returns the looper to empty. Deletion is unavailable during a take and cannot be undone.
- **Stop** retains recorded layers; **Play** restarts at the beginning. Stopping the first recording preserves the captured phrase. **Clear** deletes all layers after confirmation. Space performs the main action and S stops when the board has keyboard focus. Vintage Deck has no undo/redo control.
- The progress ring tracks the established loop's playback position, including silent portions, and stays still during the initial recording or when stopped.

### Saving, opening, and WAV files

Stop transport, open **Layers**, then use the file buttons at the bottom:

- **Save loop** writes a `.loop` project with every completed stereo layer, its volume, pan, on/off and solo settings, layered/classic mode, tempo, time signature, metronome, count-in, auto-stop, and the four deck gains.
- **Open loop** restores a project, stopped and ready to play. Opening a project or importing WAV asks before replacing an existing loop; save it first to keep it. Invalid or incomplete files leave the current loop unchanged.
- **Import WAV** opens a mono or stereo WAV as a new single-layer loop. Mono is centered. Timing uses the default 120 BPM / 4/4; WAV files do not contain this app's layer/timing metadata.
- **Export WAV** writes one cycle of the layer mix as a stereo 32-bit floating-point WAV. It respects layer volume, pan, active/solo, and deck Loop/Output gains. It excludes live monitoring, the metronome, board utility processing, and VST effects after the deck. Effects recorded before the deck are already part of each layer.

Projects and WAV imports are resampled to the current audio-device rate and must fit the current recording duration/memory limit. Files are written through a temporary file before replacing a destination. Projects do not save plugin instances, effect-board layout, device configuration, or board utility settings. Save again after editing; there is no autosave. The browser design demos cannot save or import actual audio; these controls are in the desktop app.

### Metronome and count-in

The top toolbar contains **Layer**, **Metronome**, **BPM**, **Time signature**, and **First recording count-in**. The metronome defaults to **120 BPM**, **on**, and **4/4**, and runs during count-in, recording and playback (not while empty or stopped). Turn **Click** off to stop the audible metronome. BPM is adjustable from 30 to 300 and always measures quarter notes.

The time signature can have **1-16 beats per bar**, with a beat unit of **1, 2, 4, 8 or 16**. Each click follows that beat unit, with the first beat of each bar accented. For example, at 120 BPM, 3/4 has three clicks over 1.5 seconds and 6/8 has six clicks over 1.5 seconds. Changing the signature restarts the click's bar clock but does not change recorded audio; tempo/signature changes never stretch or quantize the loop.

**Count-in** defaults to off and can be set to **1, 2 or 4 bars** before the first recording. Its duration follows the selected signature: at 120 BPM in 4/4 the options wait 2, 4 or 8 seconds. The screen shows beats remaining; recording starts automatically afterwards. Tempo and signature are locked during count-in. Count-in applies only to the first recording, not new layers, classic overdubs or rerecords. Turning the audible click off does not remove the selected countdown. Close Settings to use **Cancel Count-in** or **Stop**. The app's click is added only to playback, never baked into a layer; external hardware loopback inputs can still record output, so select your instrument input rather than a loopback channel.

The first phrase can last up to 60 seconds at normal sample rates. A fixed 256 MiB audio-buffer budget can shorten that limit at higher rates; the displayed maximum reflects the current rate. Reaching the limit preserves the first layer and starts playback. All eight layers share that phrase's length and transport; recordings are not automatically beat-quantized.
## Build and test

Install Visual Studio with C++ desktop tools, the Windows SDK, CMake and Git. From this folder:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For the browser-side layout regression checks, run `python -m http.server` from this folder and open `http://localhost:8000/tests/NativeLayoutTests.html`. The test uses a simulated native bridge and does not access audio devices.
For the browser mockup popup checks, open `http://localhost:8000/tests/PopupTests.html` using the same server.
In the Windows app, the default board uses the full pedal faces; the other visual views open those controls from the compact pedals. Assign VST parameters using the labels beneath the dials or mode switches. Older pedal and board presets retain their existing assignments.

The first configuration downloads JUCE 8.0.8, a pinned official [ASIO SDK](https://github.com/audiosdk/asio), and the Microsoft WebView2 SDK via CMake. On Windows, the app embeds the Vintage Deck HTML/CSS in WebView2 and bridges its controls to the JUCE audio engine and device settings. This development build uses the ASIO SDK's GPLv3 option, selected by the project owner. Distribution must meet its GPLv3 requirements, including corresponding source; a closed-source release needs an appropriate proprietary license. Review [JUCE licensing](https://juce.com/legal/juce-8-licence/) as well before use or distribution. The downloaded SDK's license text is in `build\_deps\asio_sdk-src\LICENSE.txt`. ASIO is a trademark of Steinberg Media Technologies GmbH.

## Pedalboard UI mockups

Open [the design lab](mockups/index.html) in a browser; no server or installation is needed. The top navigation offers:

- **Vintage deck:** a wood-backed board with dark rails, a leather-trimmed looper, tape-style reels, and a simulated tuner. The floating **Effects** drawer groups demo pedals into Dynamics, Gain, Filter, Pitch, Modulation, Delay, Reverb, and Amp & Cab folders. Drag a pedal from a folder onto the board, then use its move handle to reorder the chain. Pedals use the interactive popup designs directly on the board; VST selection, assignments, and the editor are simulated and do not load plugins. Open **Layers** for the mixer popup.
- **01 / Fixed deck:** a fixed central looper with two rows of compact pre- and post-loop pedals and patch cables. Click a compact pedal's bottom half to expand its full controls, then close it to return to the board.
- **02 / Moveable deck:** the looper is a compact, expandable pedal in the chain; the footer provides quick looper levels, transport and timing controls.
- **03 / Three pages:** the transparent page icons share a toolbar row with the tuner and Effects/Layers buttons. Switch between compact pre effects, the looper and post effects. Drop a pedal at the right edge of the pre page or left edge of the post page to move it to the other page.
- **Pedal gallery:** original pedal studies with family colors (blue dynamics, grayscale gain, orange filter, yellow pitch, green modulation, purple delay, red reverb, and brown amp/cab), larger reverb, delay, and amp/cab enclosures, and a brown loop station concept. The controls shown are design concepts and may differ from the current board templates; the gallery is static and does not process audio.
- **Pedal popups:** interact directly with the pedal controls, VST display, bypass footswitch and remove button. Choose an example VST to assign simulated parameters to each dial and the second footswitch on larger pedals; changing VST resets those assignments. The VST display can also preview an example editor. Reset restores the examples. These are fictional examples only; the browser cannot scan or load plugins.

Vintage deck has two transport footswitches and a BPM/time-signature display. Drag its dials up or down, or use arrow keys, Page Up/Down, or Home/End. The footer has Input (dB), Gate (dB), and Transpose (st) on the left, and Doubler (ms) and Output (dB) on the right. Gate and Doubler have on/off switches. These are separate from the deck's own level dials and do not process audio.

Try demo takes, undo/redo, layer level/pan/solo/on-off, retake/delete, Settings, and adding/reordering example effects. Search or open a category folder, add a pedal with its + button or drag it to the board, adjust its named dials, or toggle its footswitch. Pedal controls are visual only and do not process audio. Only completed demo layers appear in the mixer. Preview state resets on a full page reload.
The browser board studies start with example effects, support up to ten effects plus the looper, and maintain separate preview state. Pedal sizes expand to the available rail space, and a single rail is centered vertically. Dragging an effect template from the Effects drawer adds it at the end of the chain. Their black cables use connector images cropped from the supplied references. The browser remains a simulation; the corresponding standalone views operate on the real shared signal chain.
Click the bottom half of any compact pedal to open its full-control popup; there are no Edit/Delete overlay buttons. Drag the top half to reorder a compact pedal; align its center within the small center zone of another pedal (at most 18 pixels from its center) to change the destination. Passing over edges or gaps leaves the destination unchanged. Other pedals slide out of the way, and the dashed destination outline matches the dragged pedal's enclosure size and rounded corners. Keyboard users can focus the bottom half and press Enter or Space to open the popup, or focus either half and use the left and right arrow keys to reorder it.

In the browser board studies, effect popups use gallery-inspired enclosures and offer a remove button and a simulated VST3 display. The standalone app retains its existing real VST3 picker and pedal controls.

Compact-pedal moves use a floating copy that follows the pointer while the enclosure-shaped outline previews the destination. Release to commit, or press Escape or release outside the board to cancel. Reduced-motion settings disable the slide-aside animation.

The standalone executable uses the Vintage Deck design with real transport, levels, layers, tuner, audio settings, and VST3 hosting. Its Effects drawer uses the same categorized pedal templates; adding a template and loading a VST3 creates a real effect-chain slot. Settings and Layers use the Vintage Deck dialogs. The layer waveform decoration does not represent recorded audio. The browser design lab remains a visual simulation with no audio processing.

## Current limitations

- Unsaved loop changes are lost on closing or clearing after confirmation. Use **Layers > Save loop** to keep recordings and mixer settings, and save a pedalboard preset to preserve the effect board.
- This build hosts up to 16 compatible 64-bit mono or stereo VST3 effects. VST2, instruments, sidechain routing, plugin crash isolation, plugin latency compensation, MIDI control, independently timed tracks, tempo stretching/synchronization, and reverse are not implemented.
- Transpose uses a short crossfading delay, which adds latency and can color transients or chords, especially at larger shifts.
- ASIO is enabled on Windows; driver discovery does not guarantee a disconnected or busy device will open. Real-world latency depends on the device and driver.
- Settings can open while a loop exists, so you can adjust the metronome and timing in the Looper tab. Device settings in the Audio tab remain locked to prevent losing recordings on sample-rate changes. Close Settings to use transport controls. If a device disconnects or changes sample rate, clear the loop before changing devices.
- Input and output meters show the most recent audio block. Samples above full scale can clip; reduce levels if the meter turns red.
- This version has automated signal/state tests but has not yet been validated with the target audio interface or MIDI hardware.
