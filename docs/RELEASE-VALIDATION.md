# Mutable Dreams — Release validation

Firmware 1.0 · M-VAVE FM-1 · 18 October 2026

The release was installed on a physical FM-1 and checked through its control protocol and stereo USB audio. The browser runs the same application and sound engine compiled to WebAssembly. This report describes measured coverage; it is not a claim that every possible setting or combination has been exhaustively tested.

## Release identity

Identity: FM-1_710010000. Package: 610136 bytes. SHA-256: `75008881683cbeb3515e24cfd7830136b90eb293f0752dc66b203b7f2ee7cd9f`.

The installer confirmed the exact 1.0 identity after restart. The current and saved patch matched before the update, and both patch content and appearance preferences survived the update. A subsequent same-version reinstall verified saved-patch and preference persistence. A complete 1.0 → original V15 → 1.0 round trip confirmed the return and stock-install paths on the device. An independent package check verified container sizes, header/directory/file CRCs, decrypted application payload and padding. Loader tests covered successful updates, cancellation, missing-host and missing-success paths. Native tests used address and undefined-behaviour sanitizers.

## On the FM-1

Nineteen six-track configurations were exercised for five seconds each at 220 BPM, with maximum repeats, dense hits, granular send, feedback, pitched/reversed grains and filters. Six additional admission-boundary cases ran for fifteen seconds each. Maximum callback use across these tests: **82.49%**, leaving **17.51%** of the 2,902 µs callback budget. No audio deadline faults, overload events, invalid samples or dropped commands occurred in the final suite.

FULL was verified with four FM, four Harmonic or two Rings voices under the stress setup. The next higher counts entered ECO in advance. The six-track matrix also verifies six kicks, snares, hats, FM voices, VA voices, resonators, Harmonic voices and Choir voices, plus mixed ambient, techno and low-pass-gate configurations. ECO retains all six tracks. The admission guard also counts audible tails, so muting a voice does not immediately erase its processing cost.

| Configuration | Requested / actual | Duration | Callback peak | Result |
| --- | --- | --- | --- | --- |
| six-Kick-ECO | ECO / ECO | 5.1 s | 60.23% | Pass |
| six-Snare-ECO | ECO / ECO | 5.3 s | 82.36% | Pass |
| six-Hat-ECO | ECO / ECO | 5.1 s | 53.93% | Pass |
| six-FM-ECO | ECO / ECO | 5.2 s | 73.05% | Pass |
| six-VA-ECO | ECO / ECO | 5.1 s | 57.65% | Pass |
| six-Rings-ECO | ECO / ECO | 5.2 s | 82.49% | Pass |
| six-Harmonic-ECO | ECO / ECO | 5.2 s | 75.67% | Pass |
| six-Choir-ECO | ECO / ECO | 5.0 s | 52.10% | Pass |
| six-Kick-FULL | FULL / FULL | 5.2 s | 63.44% | Pass |
| six-Snare-FULL | FULL / ECO | 5.3 s | 82.36% | Pass |
| six-Hat-FULL | FULL / FULL | 5.1 s | 59.89% | Pass |
| six-FM-FULL | FULL / ECO | 5.2 s | 73.12% | Pass |
| six-VA-FULL | FULL / FULL | 5.1 s | 72.81% | Pass |
| six-Rings-FULL | FULL / ECO | 5.3 s | 82.49% | Pass |
| six-Harmonic-FULL | FULL / ECO | 5.2 s | 75.67% | Pass |
| six-Choir-FULL | FULL / FULL | 5.1 s | 57.93% | Pass |
| ambient-mixed | FULL / ECO | 5.1 s | 63.03% | Pass |
| techno-mixed | FULL / ECO | 5.2 s | 69.50% | Pass |
| lpg-mixed | FULL / ECO | 5.1 s | 64.71% | Pass |
| four-FM-FULL | FULL / FULL | 15.2 s | 77.19% | Pass |
| five-FM-requested-FULL | FULL / ECO | 15.1 s | 63.23% | Pass |
| four-Harmonic-FULL | FULL / FULL | 15.2 s | 73.85% | Pass |
| five-Harmonic-requested-FULL | FULL / ECO | 15.1 s | 65.30% | Pass |
| two-Rings-FULL | FULL / FULL | 15.1 s | 65.06% | Pass |
| three-Rings-requested-FULL | FULL / ECO | 15.1 s | 48.00% | Pass |

A **600-second** continuous mixed-voice stress run changed Grain Size, position, feedback, grain pitch/reverse/freeze, voice shape, filter settings, Colour, ducking and interface pages during playback. Its maximum callback use was **66.23%**. No reported audio faults, silent intervals or USB capture overflows occurred.

The library audit covered every factory entry: **65 Biomes × 6 seconds**, including the deliberately silent Blank, with exact parameter recall checked against the catalogue. Maximum callback use in that audit: **81.70%**. All **128 sound preset recalls** preserved notes, rhythm, mix and mute. All **14 function-button holds** opened help without invoking their short action; closing help consumed only the closing tap. The appearance settings and stopped SAVE operation were checked on the device.

The first eight Biomes also have twelve-second stereo recordings from the FM-1 in the Audio folder. They are unmastered device recordings at a fixed master setting, not browser renders or generated music; playback levels vary with each arrangement.

## Grain Size and sound purity

The previous fixed grain-start interval reused an occupied slot above Grain Size 20 in ECO. Scheduling now follows the fragment duration, waits for a free slot and finishes existing windows through quality changes. The controlled native regression checks **all 101 displayed values in both qualities**, plus live size/quality/position/pitch/reverse changes. It found zero interrupted grains; the largest constant-input sample step was 0.000321.

The physical-device test freezes a controlled FM tone, mutes the dry source and records Grain Size 10, 19, 20, 21, 30, 50 and 100 over USB. The comparison below uses high-frequency energy above 5 kHz normalized to total signal energy. This is a targeted discontinuity test, not a general music-fidelity rating or a claim that granular textures are always smooth.

| Grain Size | Reduction in normalized high-frequency residue |
| --- | --- |
| 10 | 5.55 dB |
| 19 | 4.66 dB |
| 20 | 3.93 dB |
| 21 | 18.51 dB |
| 30 | 34.04 dB |
| 50 | 39.19 dB |
| 100 | 39.20 dB |

The tape now records at 44.1 kHz, with four-point interpolation and safe read trajectories. The same 256 KiB holds **2.97 seconds of mono PCM16**, panned into stereo grains. Normal voices are linear; tape and final-output peak protection remain. A controlled dry FM tone measured approximately **0.25% harmonic distortion**, compared with 3.07% with the removed per-voice saturation. The regression requires below 0.5% for this defined tone. This is not a specification for every intentionally complex synthesis model.

Filter resonance is neutral at zero and rises gently in the lower range. At 20%, the 1 kHz impulse-response test measured about **0.16 dB** peak boost, versus 1.87 dB with the previous mapping. Warm pitched factory sounds start at zero resonance. In a controlled comparison of Velvet Mist's isolated Harmonic track, the strongest spectral component moved from about 784 Hz to its 196 Hz fundamental, at similar RMS level. These measurements support the intended tonal change; listening remains subjective.

## Native and browser coverage

The unchanged DSP source was previously checked under address and undefined-behaviour sanitizers; its original evidence identifiers are preserved. Native UI/help and browser tests were rerun for 1.0. The native DSP suite checks all 64 sounding Biomes for twelve seconds each, exact silence for Blank, every sound preset for eight seconds, sixteen sixty-second six-voice stress cases, 24 filter/engine extremes for ten seconds each, 10,000 boundary commands, all eight scales and twelve roots, deterministic sequencing, swing and repeat counts, freeze/reverse/pitch, ducking, envelope timing, corrupted saves and patch-version migrations. The original auxiliary-output optimizations were checked for bit-identical main outputs. There were no sanitizer failures.

The browser editor passed patch-file export/import round trips, invalid-file rejection without modifying the current patch, stopped-only import, local SAVE/reload and typing-key tests. Its files contain complete patch settings, but no granular recording or hardware settings. The browser audit checks all twelve pages, the 65-entry atlas, 128 sounds, free track assignment, animated splash, audio output at 44.1 kHz, Blank silence, local saving/reload, 27 chromatic keys, octave shifts, held PLAY/OCT help and key suppression while help is open. The release page was checked at desktop and 390-pixel mobile widths without horizontal overflow or browser errors. The embedded tutorial is 1920 × 1080, approximately 3 minutes 56 seconds, with 55 English subtitle cues.

Compilation and device layout checks passed. The extra warning audit found no warnings in the owned DSP implementation; five unused-parameter warnings remain in upstream API headers. Removed unused owned state is not allocated. The placeholder external speech-dictionary URL was removed. The optional reader protocol sends short status messages and help titles as readable text; it does not provide a complete screen description. The inherited command-line screen query produced no description, so it is not counted as a passed display-readback test.

## Practical limits

This is an independent instrument built around selected original Plaits and Rings algorithms, not a port of every Mutable Instruments module. Its granular bus and Choir are our own. ECO uses two grains, eight Rings modes and half-rate FM/VA/Harmonic with compensated pitch; FULL uses four grains, 24 modes and full-rate melodic engines. Not every six-track combination fits FULL. Pitch-shifted granular audio can alias, and model or Biome changes reset voices rather than crossfade seamlessly.

There is one personal saved patch. Granular audio is not saved. Browser storage is separate from the FM-1 and does not transfer patches over MIDI. External audio input, a freely wired patch graph, MIDI clock slave, sample import, song automation and Bluetooth/WLAN are not implemented. The original manufacturer's DX7 interface and factory bank are replaced.

## Installation and recovery

See the accompanying Recovery Guide for installation and the manufacturer download. The original V15 reference is FM-1_015, 699,956 bytes, SHA-256 `db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`. A physical 1.0 → V15 → 1.0 round trip was verified in this release audit, with the expected identity observed after each reboot. Its saved patch survived the round trip. V15 reset the separate display/start-sound preferences to their defaults; same-version 1.0 updates preserved both preferences. The preferences were restored to the owner’s original settings before handover. The prepared ROM rescue route was not verified by deliberately damaging firmware. The Python USB updater was used for these physical installations. The browser installer shares the protocol implementation concepts and was checked in simulation and file-validation tests; a browser-initiated hardware flash was not performed. The public package contains no private owner backups.

At handover, the FM-1 is stopped on Velvet Mist, on HOME, with animation and the optional startup sound enabled. Its live patch matches the saved patch byte for byte, and physical master control is restored.

## With gratitude

Émilie Gillet and Mutable Instruments created the original Plaits, Rings and stmlib algorithms used here, under their preserved MIT notices. fm1-base and felucca-bsp provide the device platform and simulator under MPL-2.0. Mutable Dreams is an independent community project, not affiliated with or endorsed by Mutable Instruments.
