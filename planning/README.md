# Standalone Looper - Project Plan

## Goal

Build a Windows desktop app that runs without a DAW: plug in an instrument, set up an audio interface, place effects before and after a looper, and perform hands-free. The requested visual inspiration is the [Rob Scallon Pedalboard video](https://www.youtube.com/watch?v=WRYLAAqUYWY); the audio setup should feel as approachable as the tagged Neural DSP standalone app.

This folder describes the broader plan. The Windows application now uses Vintage Deck with the existing layered looper, ASIO and metronome, plus real board utilities and serial pre/post VST3 hosting. VST2, persistence and the broader roadmap remain open. See the [project README](../README.md) for current behavior and limitations.

## Read in order

1. [Pedals and features](01-pedals-and-features.md): research, basic capabilities, and extras.
2. [Screens and workflows](02-screens-and-workflows.md): the proposed app layout and user journeys.
3. [Audio and plug-in design](03-audio-and-plugin-design.md): routing, recording, real-time safety, compatibility, and persistence.
4. [Roadmap](04-roadmap.md): start-to-finish phases, dependencies, and release gates.
5. [Testing and acceptance](05-testing-and-acceptance.md): concrete tests and targets.
6. [Sources and decisions](06-sources-and-decisions.md): research links, uncertainties, and decisions to confirm.
7. [Video feature map](07-video-feature-map.md): the reference's demonstrated features and how each fits the app.

## Proposed first release

- Windows 64-bit standalone application; mono or stereo input and stereo output.
- One stereo loop with record, playback, repeated overdubs, last-overdub undo/redo, stop, clear, and loop position/status display.
- Drag VST3 effects from a browser onto a freely arranged pedalboard, insert or reorder them anywhere in the signal chain, and place them on either side of the movable looper; open plug-in windows and save their state.
- Device selection, ASIO where licensed and available, another Windows audio backend where supported, level meters, and master volume.
- Save/load a session with loop audio and effect configuration, WAV export, keyboard and MIDI foot-controller mapping.
- Preflight, error handling, crash-safe saves, and actual audio-interface testing before a usable release.

This is deliberately a first milestone, not the complete pedal-inspired feature set. Multiple loops, rhythm tracks, reverse, and other features remain in the roadmap. VST2 is a requested compatibility goal with an explicit licensing/SDK feasibility gate, not a promise that it will be available.

## Default signal path

`Audio input -> input gain/meter -> pre-loop VST chain -> looper (live + loop mix) -> post-loop VST chain -> master gain/meter -> audio output`

Pre-loop effects are recorded into the loop. Post-loop effects process both the live signal and loop playback, and changing them affects both. More routing options are later features, not implied by this default.

## Proposed design principles

- Make the record/play/overdub state obvious at a glance and easy to use from a foot controller.
- Present a pedalboard with a persistent top bar and bottom control strip, inspired by the supplied video. Allow free visual arrangement while making the actual processing order and movable looper position unmistakable; the video reference does not establish a looper workflow.
- Third-party VSTs get host-owned pedal-style cards that open the plug-in's own editor; their original interfaces and capabilities vary.
- Keep the first loop immediate and unquantized; rhythm and synchronization are opt-in.
- Protect recordings and the audio callback even when devices or third-party plug-ins fail.
- Confirm important decisions in [sources and decisions](06-sources-and-decisions.md) before freezing the scope.
