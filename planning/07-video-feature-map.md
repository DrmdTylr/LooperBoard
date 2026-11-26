# Video Feature Map

Source: [How I use my signature plugin](https://www.youtube.com/watch?v=WRYLAAqUYWY), approximately 11:05. Review method: frames every four seconds from the supplied local video plus the available YouTube auto-generated transcript, from start to finish. This covers the full timeline but is not continuous audiovisual playback; short gestures and speech-recognition errors may be missed. The reference is an integrated effects product, not evidence that it hosts third-party VSTs or contains a looper.

| Time | Demonstrated or described | Planned adaptation |
|---|---|---|
| 00:42-01:20 | Pedalboard window, amp selection, flexible effects placement | First release: draggable VST browser, pedal-style cards, flexible chain order and a movable looper. Amps are user-installed effects, not built-in models by default. |
| 01:20-02:12 | Different reverb spaces and a freeze control | Later: support compatible reverb/freeze VSTs and their own presets/controls. Original built-in reverb and licensed impulse responses are separate projects. |
| 02:16-02:48 | Delay time in milliseconds or beat divisions; reverse, drive, flutter, width and pitch | Host compatible delay VSTs now; later provide a reliable tempo/playhead and optional own delay with these controls. Exact features depend on the plug-in. |
| 03:16-04:00 | Fuzz, compressor, overdrive, doubler, feedback | Host the user's compatible effect VSTs; separately prioritize original built-ins if wanted. |
| 04:12-04:26; 07:10 | Reorder effects and use multiple copies, including repeated delays or reverbs | First release: multiple independent VST instances, drag insertion at any chain point, visible order and resource limits. |
| 04:30-04:50; 07:33-08:00 | Tape-like degradation, warble and background textures | Compatible lo-fi/texture VSTs; optional original processors later. Do not copy the product's samples or assets. |
| 05:00-05:15 | Always-available tuner; enlarged tuner with mute | Later: compact tuner plus expanded display with clearly indicated, reversible mute and safe output behavior. |
| 05:16-05:30 | Amp cabinet selection, EQ and room mic adjustments | Use capabilities exposed by an installed amp/cab VST; consider host EQ/IR support later. Do not assume every VST exposes cabinets or microphone controls. |
| 05:36-05:44; 08:09 | Whole-board and individual-pedal presets; sharing | Later: board presets and per-instance state snapshots, separate from loop sessions; import/export preset metadata and state, never VST binaries. |
| 05:47-06:06 | Input gain/EQ, gate, noise removal, transpose, dry/wet, output, high/low-pass, limiter | First release: input/output gain and metering. Plan the remaining utility processors later, with defined signal order, bypass, latency and safety. |
| 06:10-07:02 | LFO assigned to a parameter, rate/depth, waveform, inversion and range | Later: modulation panel targeting compatible exposed VST parameters; visible target, range, rate, depth, smoothing, shape/invert and saved mapping. |
| 08:25-09:13 | Pitch and frequency split: low signal bypasses reverb while higher signal is processed | Later: explicit split/parallel routing, crossover, branch controls and latency/phase-safe recombination; also support compatible VSTs offering their own split. |
| 09:45-10:14 | Envelope follower and noise modulation applied to pitch; multiple modulation sources | Later: envelope and noise sources, multiple-source combination and assignable targets, subject to plug-in parameter support. A sequencer tab is visible; its detailed behavior is not established by this demonstration. |
| 10:41-11:05 | Emphasis on experimentation and quick playing | Keep board editing approachable, preserve existing audio during changes, and provide clear feedback and undo/safety choices. |

## Priorities and limitations

- **MVP:** freely positioned board cards, direct drag/drop, arbitrary serial chain insertion and order, multiple instances, movable looper, VST editors, safe audio graph changes, saved sessions.
- **Next:** whole-board/per-effect presets, tuner and useful utilities; choose exact ordering with the user.
- **Advanced:** modulation, split/parallel graph, creative processors and additional sound libraries. These are planned capabilities, not claims that the first release already has them.
- The video product's native pedals have integrated controls. A third-party VST may expose only its own editor, a limited parameter list, unusual channel layouts or licensing constraints; the host must degrade visibly rather than promise uniform access.
- The user-requested VST2 format is subject to the licensing/SDK gate in [audio and plug-in design](03-audio-and-plugin-design.md).
