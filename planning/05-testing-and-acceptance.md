# Testing and Acceptance

Targets below are proposed release criteria, not measurements already achieved. Record OS, CPU, interface/driver, plug-in versions, sample rate and buffer size with every performance result. Never claim zero latency.

## Automated loop tests

- Feed known sample sequences through the engine at several buffer sizes, including a loop length not divisible by the buffer size. After the first pass, each repeated cycle must contain exactly the recorded sample count, with no added or dropped samples.
- Overdub a known signal over multiple wraps; verify mixing, last-overdub undo and redo, and redo invalidation after a new overdub. Live monitoring must appear exactly once.
- Test empty/recording/playing/overdubbing/stopped transitions, stop/restart, deliberate clear, and commands at and across a wrap. Define and test stop during first recording before implementation.
- Test memory limits, zero-length/too-short recordings, channel mapping, device rate changes and buffer-size changes; preserve recordings or explicitly block unsafe changes.
- Test deterministic MIDI/UI event ordering, including two commands in one block; no race may corrupt the loop.

## Routing and plug-ins

- With known gain effects before and after the loop, confirm pre effects are baked into recorded audio; changing pre effects later does not rewrite it. Confirm post effects affect both live and existing loop playback.
- Confirm add/reorder/bypass/remove changes the intended chain, preserves loop audio, and does not double monitor. Test mono/stereo mapping and explicit rejection of incompatible layouts.
- Drag an effect from the browser to the start, between nodes and end of both sides of the looper; drop another instance of the same effect and confirm independent state. Test supported file drop, invalid/cancelled drop and loading failure without changing the existing graph or source file.
- Move any card and the looper in free space, then onto insertion points within/across the loop boundary by mouse and keyboard. Free placement must not alter audio order; insertion must alter it. The displayed connections, order and actual audio must match, pre/post labels stay clear, and transport remains accessible when the looper is off-screen.
- Save/reopen a manually arranged board; independently verify its coordinates, graph order, repeated-instance state and loop audio. Stress graph changes during playback/recording for dropouts, unsafe level jumps and lost audio.
- Open and close different third-party editor windows without losing the host card, chain order or session state. Verify an unavailable plug-in is visibly identified.
- Test scan crashes/failures, missing plug-ins, missing licenses, disabled plug-ins and saved opaque state recovery. A broken plug-in may still crash an in-process host; verify recovery, not an unrealistic guarantee of isolation.
- Test latency- and tail-producing VST3s and, only if the VST2 gate passes, a representative licensed VST2. Test the actual desired Neural DSP VST format if installed and licensed.

## Persistence and controller checks

- Save and reopen a session: compare loop sample count/content, chain order, plug-in identity/state, levels and mappings. Verify a missing plug-in retains its saved state.
- Interrupt/fail a save, simulate full disk or denied permission, and reopen the prior valid session. The app must report failure; the last valid save must remain usable.
- Attempt close/open/clear with unsaved audio and confirm each prompt/gesture protects it. Check WAV export has the recorded cycle and excludes live input.
- Test MIDI learn, conflicts, keyboard focus, protected clear, device removal/reconnection and explicit feedback; ensure regular MIDI input is not mistaken for a clear.
- When implemented, verify board versus single-effect presets, missing plug-in warnings, exposed parameter modulation and saved mappings; board presets must never replace loop audio.

## Real hardware and performance

- Initial target: at 48 kHz with the target interface's ASIO driver and a 128-sample buffer, run live monitoring, one loop and representative pre/post effects for 30 minutes with **zero observed audio dropouts**; log configuration and failure counts. This is a target, not a guarantee for every interface or plug-in.
- Measure round-trip latency using a physical loopback where safe and available, at 64/128/256 samples; report actual results separately from driver-reported latency and plug-in latency. Agree an acceptable threshold with the user rather than inventing a universal one.
- Record a loop, leave it playing for 30 minutes and compare loop boundaries to the initial sample count: no accumulated sample drift in the internal transport.
- Listen to loop joins, overdub transitions, bypass and device changes with headphones at safe levels. Test quiet and loud signals and verify clipping indication.
- Test at least one supported fallback Windows backend and a real MIDI foot controller; record device-specific limitations and minimum usable buffer sizes.

## Release checklist

- [ ] Clean-machine install, launch, setup, save/open, export and uninstall work without a DAW.
- [ ] On-screen/keyboard/MIDI interaction, visible state, scaling and accessible labels pass manual checks.
- [ ] Disconnecting the interface or controller produces a visible error, avoids uncontrolled sound and keeps recoverable session data.
- [ ] Audio callback is reviewed for blocking, allocation and unsafe cross-thread access; stress tests produce no reproducible glitches on the stated reference setup.
- [ ] Build, tests, help, legal notices, supported formats and known incompatibilities are documented and consistent with the shipped build.
- [ ] A user can complete the end-to-end workflows in [screens and workflows](02-screens-and-workflows.md).
