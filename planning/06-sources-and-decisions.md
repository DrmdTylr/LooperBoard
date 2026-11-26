# Sources and Open Decisions

Research accessed 2026-11-23. Pedal features are inspiration, not a promise of exact behavior or bundled sounds.

## Sources

- [Rob Scallon video: How I use my signature plugin](https://www.youtube.com/watch?v=WRYLAAqUYWY) - inspected frames every four seconds across the supplied video and reviewed YouTube's auto-generated transcript from beginning to end. This is not continuous visual/audio playback; transcription errors and short gestures may be missed. See [video feature map](07-video-feature-map.md).
- [Fine Classics Rob Scallon Pedalboard](https://fineclassicsplugins.com/products/rob-scallon-pedalboard) - its product description confirms a standalone pedalboard-style app. Our layout adapts the observed visual structure rather than copying graphics or asserting feature parity.
- [Pigtronix Infinity Looper](https://www.pigtronix.com/pedals/infinity-looper/) - original Infinity reference; not a confirmed match to “Infinity O.”
- [MXR Clone Looper](https://www.jimdunlop.com/mxr-clone-looper-pedal/).
- [EHX 720](https://www.ehx.com/products/720/) and [EHX 1440](https://www.ehx.com/products/1440/).
- [Boomerang III Phrase Sampler](https://www.boomeranglooper.com/products/phrase-iii-sample).
- [Line 6 DL4 MkII](https://origin.line6.com/effects-pedals/dl4-mkii/).
- [BOSS RC-1](https://www.boss.info/se/products/rc-1/specifications/), [RC-5](https://in.boss.info/products/rc-5/) and [RC-10R](https://in.boss.info/products/rc-10r/).
- [Steinberg: VST2 discontinued](https://helpcenter.steinberg.de/hc/en-us/articles/4409561018258-VST-2-Discontinued) and [current VST SDK licensing](https://www.steinberg.net/developers/vstsdk/) - do not infer legacy VST2 rights from current VST3 licensing.
- [JUCE 8 license](https://juce.com/legal/juce-8-licence/) and [Steinberg ASIO licensing FAQ](https://forums.steinberg.net/t/asio-sdk-proprietary-licensing-faq/1044024) - check current terms and the intended distribution before implementation.

## Decisions to confirm before implementation

1. **Pedals:** Which exact Pigtronix Infinity model is intended? Are there other specific pedals to add?
2. **Reference:** Which video-inspired features should follow the first playable release? The reference demonstrates flexible chain order, but gestures and third-party plug-in hosting are not assumed to work identically.
3. **Hardware:** Which Windows version, audio interface/driver, input/output setup and MIDI foot controller will be used? Is there a headphone-only use case?
4. **Plug-ins:** Which exact VST3 and VST2 plug-ins, versions and architectures must work? Is the Neural DSP VST version installed and licensed? VST2 is required by the request but subject to the SDK/rights gate.
5. **Looper behavior:** Choose `record -> play -> overdub` or `record -> overdub -> play` as default, what stop does while recording the first loop, how stopped loops restart, and whether a press or hold clears.
6. **Priority:** Is a stable single loop first acceptable, or are synchronized tracks, song sections or drum rhythms mandatory for the first usable release?
7. **Scale:** Set a maximum loop duration, undo memory budget and desired number of tracks based on available RAM and performance tests.
8. **Distribution:** Personal use, open-source distribution or commercial release? This affects licensing, packaging and support obligations.
9. **Monitoring:** Should the default post-effects chain process live input and loop together as proposed, or should loop-only post effects be required?

Until confirmed, the defaults in this plan are proposals and should not be mistaken for completed behavior.
