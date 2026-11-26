# Pedals and Feature Inventory

## Reference pedals

These are feature inspirations, not claims that the app will clone a pedal or include its sounds. Device storage limits are not proposed software limits. Sources are linked in [sources and decisions](06-sources-and-decisions.md).

| Reference | Relevant capabilities | Candidate app feature |
|---|---|---|
| Pigtronix Infinity Looper | Two stereo loops; parallel, series, length multiplier, MIDI clock, input split, variable decay, reverse | Multi-track modes, tempo sync, independent input routing, loop aging |
| MXR Clone Looper | Record/overdub; undo; loop-start indication; half/double speed and reverse | Clear cycle indicator, speed and reverse controls |
| EHX 720 | Stereo loops and memories; undo/redo, reverse, half speed, fade stop | Saved loops, quick creative effects, selectable stop behavior |
| EHX 1440 | Stereo memories, MIDI clock, reverse/octave and variable speed, one-shot/retrigger, fade, import/export, selectable record order | Clock sync, playback modes, sound-file exchange, configurable action order |
| Boomerang III | Series, series with master, synced and free parallel loops; decay, reverse, octave, one-shot, live copy | Song parts, synchronized or independent tracks, resampling, ambient decay |
| Line 6 DL4 MkII | One- and four-switch looper; mono/stereo; position before or after effects; play once, half speed, reverse | Pedal-style layouts and configurable pre/post effects |
| BOSS RC-1 | Simple record/play/overdub/undo; stereo; circular status/progress; external switches | Large single-loop performance control and readable status |
| BOSS RC-5 | Stereo, reverse, saved phrases, MIDI control/sync, rhythms and A/B variations | Phrase library, rhythm and controller support |
| BOSS RC-10R | Two serial song sections, rhythms with variations/fills/intros/endings, MIDI control | Verse/chorus mode and a coordinated rhythm section |

**Model uncertainty:** “Pigtronix Infinity O” was not independently identified. The table uses the original Infinity Looper as a provisional reference; confirm the exact model before claiming model-specific parity. “etc.” leaves the list open for more pedals.

## Basic features

### First playable release (MVP)

- Live input monitoring through the software, with mono-to-stereo routing and stereo input where available.
- Single stereo loop: record the first pass, close it into seamless playback, add repeated overdubs, undo/redo the latest overdub, stop, restart and clear.
- Independent loop level and live-monitor level; input and output meters; clipping indication; master output level and mute.
- Obvious empty, recording, playing, overdubbing and stopped states, loop progress and elapsed/total time.
- On-screen controls, keyboard shortcuts and assignable MIDI notes/CC for core commands; show controller connection status.
- Pre- and post-looper VST3 chains: scan configured folders, drag effects from the browser or a supported plug-in file onto the board, insert anywhere, move/reorder/bypass/remove, open the editor, and show unavailable plug-ins.
- A freely arranged, pannable/scrollable pedalboard with a movable looper, clear signal-order links, and persistent top and bottom controls. Support multiple instances of the same VST subject to system resources and plug-in licensing. Use generic host cards for third-party plug-ins rather than assuming their interfaces can be embedded as pedals.
- Select audio driver, input/output device and channels, sample rate and buffer size; show reported latency and connection errors.
- Named session save/open, loop audio and plug-in state recall, save-as, and WAV export; warn before losing unsaved changes.

### Essential follow-on pedal functionality

- Multiple tracks with independent level, mute, stop, clear and status; synchronized parallel, free parallel and serial song-section modes.
- Optional tempo, metronome, tap tempo, count-in and beat/bar quantization; loop length multiples; MIDI clock sync.
- Reverse, half/double speed, one-shot, retrigger, stop at boundary and fade-out.
- Multiple named loops/sessions and WAV import; intuitive recall without unexpectedly replacing unsaved audio.
- Configurable single-switch action order, dedicated control mappings, and expression control of useful parameters.
- Rhythm accompaniment with A/B variations and fills; clear separation of rhythm, live and loop levels.

## Optional extras

- Loop decay/feedback and sound-on-sound aging; reverse solo and live resampling.
- Track-specific pre/post effect chains, parallel effects, sends, aux/monitor outputs and multiple instrument input routing.
- More advanced undo history, track duplication, loop trimming, waveform editing and track stems export.
- Variable speed with pitch changing, or independent time-stretch/pitch-shift; these are different features and require separate quality/latency decisions.
- Drum kits, intros/endings, user rhythm import, auto-record on input, preset changes and setlists.
- External clock output, advanced MIDI bindings, controller feedback LEDs, expression curves and OSC.
- Video-inspired board and individual-effect presets, sharing, assignable macros, LFOs, envelope followers, sequencers and noise modulation, subject to reliable plug-in parameter support. These are separate from basic MIDI command mapping.
- Input EQ, noise gate/reduction, transposition, global dry/wet, high/low-pass filters and an optional limiter; define their signal paths and latency before adding them.
- Creative VST effects for amp/cabinet models, convolution/reverb with freeze, delay, fuzz, compression, overdrive, doubling, feedback, lo-fi tape and pitch/frequency splitting; built-in versions are separate future projects, not assumed to be bundled.
- Tuner, presets, metronome accents, pedalboard themes, accessibility options and full-screen performance mode.
- Plug-in sandboxing, CPU/latency diagnostics, recovery snapshots and offline export enhancements.
- macOS support or a plug-in version of the looper itself, only after the standalone Windows app is stable.

## Boundaries

- No supplied commercial plug-in, Neural DSP executable, branded pedal artwork, rhythm library, or copyrighted sounds are bundled or copied.
- A standalone `.exe` is not automatically a hostable VST. The user must install a compatible VST3 (or, if the gate succeeds, VST2) version and have any required license.
- “Unlimited” overdubs means repeated overdubbing subject to practical memory, storage, and undo-history limits; the app must communicate limits rather than silently discard audio.
