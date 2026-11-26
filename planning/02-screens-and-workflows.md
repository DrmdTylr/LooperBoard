# Screens and Workflows

## Main performance screen

The supplied video was reviewed using frames sampled every four seconds across its 11-minute runtime and the full available auto-generated transcript. It presents an effects-pedalboard workflow, freely ordered effects including multiple instances, a tuner, whole-board and individual-pedal presets, a utility strip, modulation, and a frequency-split example. See the timestamped [video feature map](07-video-feature-map.md). The reference does not establish an integrated looper or third-party VST hosting, and sampled frames do not show every gesture.

Use those layout and workflow principles with original artwork, not the reference product's graphics. Proposed desktop layout:

```text
 [Session / Save]       [Name *]       [Device / latency] [MIDI] [Settings]
 ┌────────────────────── PANNABLE PEDALBOARD ────────────────────────────┐
 │ PRE-LOOP                  LOOPER                     POST-LOOP        │
 │ [+] -> [VST card] -> [VST card] -> [state/progress] -> [VST card]   │
 │                                     [rec/dub] [stop]    [open/bypass] │
 │                                     [undo] [clear]                   │
 └───────────────────────────────────────────────────────────────────────┘
 [Input gain / meter]      [Live level] [Loop level]    [Output / mute]
```

The illustration is the default automatic layout; users may place cards anywhere in a pannable canvas. Signal lines, arrows and order indicators convey actual processing order independent of screen position. The looper's current state and transport remain accessible through pinned compact controls when it is off-screen. Keep the top and bottom strips visible. The example is a concept, not a pixel-perfect reconstruction.

### Board interaction

- The top strip holds the session name and unsaved indicator, save/open, device status, MIDI status and settings. Avoid implying that the reference app's preset bar is a loop-session manager.
- The main area displays the ordered signal path, including the looper as a movable node. The effective pre/post position is derived from signal order and remains clear regardless of canvas placement. Empty chains offer an add button.
- Drag a scanned VST from the searchable browser to any location on the canvas, or drop a supported local VST3 bundle (or VST2 file if supported). Validate/scan file drops before loading; report incompatible, untrusted or failed plug-ins. Never move or delete the original plug-in file. A card can also be added without drag-and-drop.
- When a new card is dropped on a highlighted insertion point or connection, insert it there. When dropped on empty canvas, place it there visually and append it to the signal chain, showing its connection and order. Moving a card on empty canvas changes its visual location only; dropping it on an insertion point changes the processing order. Invalid/cancelled drops leave the graph unchanged. Auto-scroll/pan at edges, provide a reset-layout action, and preserve manual locations.
- The looper is a prominent original pedal-style module with readable state, progress, record/overdub, stop, undo/redo and protected clear. It may be moved on the canvas or inserted at a different point in the chain; effects before it are recorded, effects after it process live and loop audio together.
- Each hosted VST appears as an original host-owned card showing name, order, pre/post position, bypass, open editor and an explicit menu for move/remove. Permit as many instances and any sequence as the system and plug-in license allow. Supply buttons/menu and keyboard alternatives to all drag actions. A cross-boundary move must visibly show the resulting routing before committing.
- Open the VST's own editor in a window; its graphics and control layout are supplied by the plug-in. Do not promise that arbitrary VST interfaces can be resized, embedded into pedal artwork, or exposed as host knobs. An unavailable plug-in remains a labeled card with retry/remove options.
- The bottom strip keeps input gain/meter, live and loop levels, master output/meter and mute in view. The video's gate, noise reduction and dry/wet controls are visual inspiration, not implicit requirements to add those processors or a global wet mix.
- Use text plus color and shape for state; the record control must show the next action. Provide a compact empty state, resizable window, tooltips, keyboard focus and readable controls at common display scales.
- Plan whole-board and per-effect presets plus a collapsible macro/modulation strip inspired by the video. A board preset changes effects/parameters, not recorded loop audio; warn before replacing unsaved effect settings. Macro/LFO mappings depend on parameters actually exposed by the plug-in.

### Main controls

- Session name, new/open/save/save as, unsaved marker and export.
- Audio device status and settings shortcut; input/output meters and clip indicators.
- Pre/post chains: add, remove, bypass, reorder, open editor and indicate load failure.
- Looper: record/finish first pass, overdub, play/stop, undo/redo and clear; progress, state, time, loop level and live level.
- Clear requires a confirmation for mouse/keyboard; a deliberate configurable hold gesture for MIDI can avoid an unintentional destructive press. Never erase on a simple unconfirmed press.
- Show available controller and command feedback. Keep critical controls accessible without opening a settings panel.

## Other screens and dialogs

| Screen | Purpose and essential contents | Phase |
|---|---|---|
| First-run/audio setup | Driver/backend, device and channel choices, sample rate, buffer, reported latency, test meters, monitoring and safe output level | MVP |
| Plug-in browser | Search scanned VST3 effects, drag into any board location, scan paths, rescan, format, load errors and blacklist/quarantine status | MVP |
| Plug-in editor window | The plug-in's own UI; close without removing, bypass and identify position in chain | MVP |
| MIDI/keyboard mapping | Device selection, learn/assign, conflict warning, commands, test feedback, clear mapping, saved bindings | MVP |
| Preset browser | Search/save whole boards and individual plug-in states; share metadata/state without bundling plug-ins; do not replace loop audio | Later |
| Macro/modulation panel | Map macros, LFO, envelope, sequencer and noise sources to exposed parameters, with range and source visualization | Later |
| Tuner | Persistent compact tuning indication and larger view with explicit output mute | Later |
| Session browser/save dialog | New/open/save-as, recent sessions, missing file/plug-in warnings and unsaved-change decisions | MVP |
| Export dialog | Destination, name, loop WAV format and progress/error notification | MVP |
| Diagnostics/recovery | Device disconnection, failed scan, unavailable plug-in, safe startup, recovery candidate and explicit actions | MVP |
| Track/song view | Tracks, independent controls, series/parallel modes, active/queued song section and sync relationships | Later |
| Tempo/rhythm panel | BPM, tap, meter, click/count-in, quantize, rhythms, fills and MIDI clock status | Later |
| Loop library/editor | Import, audition, name, duration, optional trim and metadata | Later |
| Global preferences/about | Defaults, paths, keyboard, accessibility, version and third-party licenses | MVP (minimal) |

## Core workflows

1. **First launch:** select interface/backend and input/output channels; choose buffer; play and check meters; set safe master level; save settings.
2. **Build a board:** scan plug-ins, drag an amp/effect from the browser to the canvas or an insertion point; add another instance if desired. Position effects before and after the looper, open their editors and hear the live signal. Move a card or looper to an insertion point to change processing order, or into free space to change appearance only; check the visible connections. The looper remains accessible via pinned controls.
3. **Record:** press record, play, press again to close the first pass and play it; press overdub, play another part, return to playback; undo/redo last overdub.
4. **Foot control:** select a MIDI input, learn record/overdub, stop, undo/redo and a protected clear action; test feedback before performing.
5. **Save and resume:** save a named session, close and reopen it; restore audio, routing and plug-in states or clearly report what is missing.
6. **Export:** choose a filename and render the loop to WAV; report success or a specific failure without changing the session.
7. **Recover:** on a device or plug-in problem, stop safely or mute, explain the problem, preserve the session and offer a retry or safe-mode reopen.

Later workflows add synchronized tracks, switching verse/chorus, rhythm selection, and MIDI clock. Exact switch gestures and track timing require the decisions in [sources and decisions](06-sources-and-decisions.md).
