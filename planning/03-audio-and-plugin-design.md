# Audio and Plug-in Design

## Proposed stack and scope

- Native Windows x64 standalone executable, C++ and JUCE as the initial framework candidate; CMake and a reproducible Windows build. Validate framework license terms before adoption or distribution.
- JUCE is a candidate, not an installed dependency or a guarantee of VST2/ASIO availability. Prototype device I/O and VST3 hosting before committing to the architecture.
- Prefer the user's interface's native ASIO driver when available and properly licensed; provide a supported Windows fallback, with explicit latency/feature limitations. Hardware and driver determine actual performance.
- Host compatible 64-bit VST3 effects in both positions. VST2 is a separate requested deliverable: check lawful SDK access, hosting framework support and distribution terms first. Do not fetch unofficial SDK copies, assume VST3 licensing covers VST2, or promise a 32-bit bridge.

## Routing and monitoring

```text
physical input -> selected channels -> gain/meter -> PRE effects
                                                   |
                                                   v
                                    [record / overdub into loop]
                                                   |
                              processed live + loop playback mix
                                                   |
                                      POST effects -> master -> output
```

- The selected input is monitored once in software. Warn users that interface direct monitoring can double the signal or produce comb filtering; offer clear guidance, not an assumption that software can control the interface's mixer.
- The pre-chain affects audio as it is recorded; later pre-chain changes affect new playing and overdubs, not already recorded material. Post-chain changes affect the combined live and loop signals.
- Define mono/stereo channel mapping, unsupported plug-in channel layouts, plug-in latency reporting and bypass behavior explicitly. Changing or removing a plug-in must not destroy recorded loops.
- The master meter measures the output after post effects and gain; handle clipping visibly. Muting must be readily accessible; do not silently add an effect or limiter.
- Delay compensation: record the actual input path reaching the loop. Measure input/output and plug-in latency and test timing against monitored playback; do not silently shift recordings based solely on a driver's reported figures. Provide measured/manual compensation only after its behavior is validated.

## Looper engine

- A sample-indexed transport runs inside audio blocks. Queue control events from UI/MIDI; apply them at a deterministic sample boundary. Specify how timestamped MIDI events and untimestamped UI events are aligned.
- Proposed state model: `Empty -> Recording -> Playing <-> Overdubbing`, plus `Stopped` with loop retained. Clear returns to Empty only after the protected action; stopping during first recording needs an explicit rule in the implementation specification.
- The first pass establishes loop length; overdubs align to the existing cycle. Repeated overdubs must not add an extra live-monitor path. Wrap at the exact sample boundary even when loop length is not divisible by buffer size.
- Keep at least one last-overdub undo/redo snapshot; a new overdub after undo invalidates redo. Define bounded memory and clear error/warning behavior. Saving while recording must use a consistent snapshot or be disabled with explanation.
- Use an intentional, tested boundary/crossfade strategy to control clicks without changing loop length; avoid promising that arbitrary discontinuous recordings are naturally seamless.
- Allocate buffers and prepare plug-ins away from the real-time thread; no disk I/O, blocking locks, unbounded allocation, UI access or plug-in scanning in the audio callback. Exchange state safely at buffer boundaries.
- Specify maximum recording length from an explicit memory budget and sample rate/channel count, show remaining capacity and preserve captured audio if a limit is reached.

## Plug-in lifecycle and failures

- Discover files in configured directories on a worker; identify format/architecture, cache results and report individual failures. Use a separate scan process if feasible so a bad plug-in cannot kill the main app during scanning; persist the identity being scanned to quarantine a plug-in after a crash.
- Only show compatible audio effects as loadable; reject instruments, unsupported channel layouts and mismatched architectures with an actionable explanation. Plug-ins may have their own licensing or require activation.
- Instantiate, prepare, edit, bypass, remove, reorder and serialize plug-in state off the audio callback, coordinating a safe graph swap and plug-in editor lifetime.
- The pedalboard uses host-owned cards; the actual third-party editor remains plug-in-owned and may have its own size, threading and window behavior.
- Keep independent visual coordinates and an explicit ordered signal graph. A free-space drag moves only the card; a drop on an insertion point changes graph order. Moving the looper in the graph updates pre/post classification without changing existing recorded audio. Render connections and order from the graph, never infer processing order solely from card coordinates.
- Library/file drops validate supported format, architecture, audio buses and licensing/activation; scan on a worker and instantiate safely. Multiple instances have independent state. Reject a bad drop with an actionable error and preserve the existing graph; never modify the original plug-in file.
- For any graph change, prepare the replacement away from the real-time thread and switch safely at an audio boundary; handle plugin preparation failure by retaining the prior graph and reporting it. Test for loud discontinuities, tails, plug-in latency changes and behavior during recording.
- On missing or failed plug-in, retain its identity and saved opaque state, mark the slot unavailable, and let the user locate/retry or explicitly remove it. Never overwrite saved state with an empty replacement.
- Running plug-ins in the main process means a faulty plug-in can still crash the app even if scanning is isolated. Runtime sandboxing is a later enhancement, not guaranteed MVP protection.
- Test latency-introducing and tail-producing effects; specify safe bypass and transport/playhead information to hosted effects. Do not assume all third-party plug-ins behave correctly.

## Sessions and data safety

- Use a versioned session directory containing a manifest and referenced audio and plug-in state; store metadata such as channels, rate, loop length, settings, slot order and mappings. Keep global hardware settings separate from portable sessions.
- Make a consistent snapshot, write to temporary files, verify write results, and replace the saved version only on success. Preserve the previous valid session on failure; avoid assuming a multi-file directory replacement is atomic on every filesystem.
- Warn on unsaved close/open/clear; report disk-full, permission, missing audio and incompatible format errors. Offer recoverable snapshot(s) separately from the last deliberate save.
- WAV export starts with the loop's defined cycle and avoids unexpectedly including live input; specify whether it is dry loop audio or rendered through the post-chain, including tail handling. First release defaults to raw stored loop audio; rendered effects export is a later explicit option.
- Never save actual commercial plug-in binaries into sessions or ship them. Persist plug-in identity and its own state only.
- If effect-board presets are added later, store chain order and plug-in states separately from a full session; loading a board preset must not replace loop audio. Warn before discarding unsaved effect changes.
- Store board coordinates separately from ordered node identifiers; restoring a session must restore both the visual arrangement and processing order.

## Video-inspired extensions

- Per-effect presets use the plug-in's own preset mechanisms where available, or a host-saved opaque state snapshot; don't assume all plug-ins expose named programs. Board-preset sharing stores references/state, never plug-in binaries; warn when recipients lack plug-ins or licenses.
- Macro, LFO, envelope, sequencer and noise modulation targets only parameters the plug-in exposes to the host. Define scaling, smoothing, range, multiple-source combination, tempo and save/restore before implementation. Third-party custom editor controls cannot be assumed to accept direct drag targets.
- Optional frequency-split/parallel routing needs a graph with explicit split, crossover, levels and recombination, latency compensation and phase tests. It is not part of the MVP serial chain.
- Optional global dry/wet and tuner mute require explicit routing: specify whether live and loop audio are affected, account for plug-in latency, and avoid a second unintended monitor path. Do not silently add gates, noise reduction or a limiter.
