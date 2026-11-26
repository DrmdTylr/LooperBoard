# Roadmap: Start to Finish

Status: a loop-only Windows build supports eight same-length layers, per-layer volume/pan/solo/on-off/rerecord, ASIO discovery, and a metronome with first-recording count-in. Focused engine tests exist; see the [project README](../README.md). Hardware testing, persistence, VST hosting, MIDI and other planned features remain open. Each phase should end with its gate met before the next dependent phase. Time estimates are intentionally omitted until hardware, experience and final scope are known.

## 0. Confirm scope and feasibility

- [ ] Confirm the Pigtronix model, intended interface/controllers, target Windows versions, app distribution and preferred looping gestures.
- [x] Review sampled visuals across the supplied video and its complete available auto-generated transcript; map demonstrated features and limitations.
- [ ] Agree on an original pedalboard layout and which specific interactions matter; the video does not establish an integrated looper or third-party VST hosting.
- [ ] Check JUCE, VST3, ASIO and VST2 SDK/license and redistribution terms for the intended build. Document decisions; VST2 remains blocked until lawful access and compatibility are verified.
- [ ] List target plug-ins and architectures, including whether the tagged Neural DSP product has an installed VST3.
- [ ] Establish repository/build conventions, dependencies and a manual test matrix. Do not modify the parent home-directory repository outside this project.
**Gate:** approved MVP, platform/driver choices, and written VST2 feasibility outcome.

## 1. Build a running audio shell

- [ ] Set up the application framework, Windows build, test target and executable packaging basics.
- [ ] Add audio device settings, mono/stereo channel mapping, software monitoring, gain, mute, meters and errors.
- [ ] Test using the target audio interface and headphones at safe volume; measure round-trip latency.
**Gate:** executable starts without a DAW and stable, single-path live monitoring works on a real interface.

## 2. Host pre/post effects

- [ ] Prototype VST3 discovery, scan failure handling, plugin editor, load/unload, reorder and bypass.
- [ ] Place a simple pass-through placeholder at the future looper position and demonstrate ordered pre and post processing.
- [ ] Test real instrument, gain, delay and amp-sim effects, including their channel layouts, latency and activation requirements.
- [ ] Implement VST2 only if Phase 0's license/SDK and technical gates pass; otherwise record the blocker and retain VST3 functionality.
**Gate:** multiple compatible effects operate on either side, with failures visible and no lost audio configuration.

## 3. Implement the loop engine

- [ ] Implement sample-accurate first-pass record, fixed-length playback, overdub, stop/restart, protected clear, last-layer undo/redo and level control.
- [ ] Add bounded audio storage, safe state changes and exact wrap behavior, including non-block-aligned lengths.
- [ ] Add focused automated signal/state tests and real-hardware listening tests.
**Gate:** the MVP loop acceptance tests pass and the engine holds time without audible buffer-boundary artifacts.

## 4. Make it playable

- [ ] Build a pannable, freely arranged pedalboard with persistent top/bottom strips, visible signal connections, a movable looper, host-owned plug-in cards and looper transport/state/progress feedback.
- [ ] Implement library-to-board and supported file-to-board drag/drop, insert at any connection, visual free positioning and drag reordering across the looper; support repeated instances, error recovery and safe graph swaps.
- [ ] Support non-drag alternatives and editor windows, with pinned looper controls when off-screen. Add keyboard shortcuts and accessible controls.
- [ ] Add MIDI device selection and learnable commands; define clear/hold, lost-device behavior and conflicts.
- [ ] Test while playing: a user can operate record, overdub, stop and undo without a mouse.
**Gate:** complete real-time workflow works with a MIDI controller and state is readable at a glance.

## 5. Persistence, recovery and release candidate

- [ ] Implement versioned sessions, audio and plug-in state save/restore, unsaved prompts, missing-plug-in handling and WAV export.
- [ ] Implement safe save, recoverable snapshots and safe startup after scan failure; validate disk-full and interrupted-save behavior.
- [ ] Run compatibility, reliability, latency, accessibility and security/privacy checks; document setup, troubleshooting and third-party licenses.
- [ ] Produce a Windows installer or clearly documented portable build, test install/uninstall and a clean-machine first run.
**Gate:** all MVP checks in [testing and acceptance](05-testing-and-acceptance.md) pass or documented blockers prevent release.

## 6. Pedal-inspired expansion

Prioritize with the user after MVP; each item includes its own design, tests and documentation.

1. Multiple tracks, synchronized/free parallel and serial sections; loop multipliers and optional master track.
2. Reverse, half/double speed, one-shot, retrigger, stop-at-end, fades and decay.
3. Tap tempo, metronome, quantization and MIDI clock; test drift, start/stop and loss of clock.
4. Rhythm engine with A/B parts and fills; obtain or create properly licensed sounds and patterns.
5. Loop library/import, advanced routing, live resampling, effects export and other selected extras.
6. Video-inspired board/per-effect presets and sharing; tuner; input/output utility controls; macro/modulation panel with LFO, envelope, sequencer and noise sources. Validate actual plug-in parameter support and define global utility routing before implementation.
7. Frequency split and parallel routing with latency/phase handling; explore the demonstrated creative sounds using compatible installed plug-ins or separately scoped original built-in processors. See [video feature map](07-video-feature-map.md).

## 7. Maintain and distribute

- [ ] Publish clear installation and interface/MIDI/plugin setup instructions, a compatibility matrix, versioned release notes and license notices.
- [ ] Test upgrades and old-session migration, collect opt-in diagnostics only if added, and provide a safe backup/restore procedure.
- [ ] Triage audio dropouts and plug-in compatibility issues; maintain regression tests and release only after re-running the relevant gates.

## Dependencies and risks

- Plug-in hosting, MIDI and persistence depend on the audio shell; track/rhythm synchronization depends on a working transport and loop engine.
- VST2 licensing and legacy SDK access may block that format independently of VST3; x86 plug-ins require a separate bridge and are not assumed supported.
- Third-party plug-ins can crash, block or add latency; driver behavior and real-world round-trip latency vary.
- Long loops and undo layers consume significant RAM/disk; crashes and power loss require explicit data protection.
- Adding multi-track sync, rhythm libraries or time-stretch substantially increases scope and should not delay the first useful app.
