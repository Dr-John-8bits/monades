# Mutable Dreams 1.0

A playable FM-1 instrument for generative ambient music and rhythms. Six freely assignable tracks, eight sound engines, 64 editable factory Biomes plus a blank starting point, and 128 sound presets. Every engine can occupy every track.

## Play

Open `Mutable-Dreams.html`, choose **Start instrument**, then **Play**. The offline file contains the actual firmware application and sound engine compiled to WebAssembly. Browser drivers replace the hardware drivers; browser speed is not an FM-1 performance measurement. Browser saves stay local and separate from the device. Export/import complete patch files from the browser editor. Enable the typing-key option to play F through E with A W S E D R F G Y H U J, and use OCT to shift the range. All 27 on-screen chromatic keys are available.

On the FM-1, use SELECT to choose a track and ALGORITHM to change its engine. PRESETS selects a complete Biome on HOME/SEQ/FX/GLO, or a sound preset on any EDIT page. A sound preset preserves notes, rhythm, mix, send and mute. A Biome replaces the complete patch and clears the old cloud. Stop and SAVE before changing Biomes if you want to keep your work.

| Control | Function |
| --- | --- |
| HOME | Density, melodic Motion, Colour, granular Space |
| EDIT | Sound → Filter → Envelope → Mix |
| SEQ | Rhythm → Groove: swing, repeats, accent, clock division |
| FX | Granular → Tape: pitch, reverse, duck source and depth |
| GLO | Harmony → System → Look: display motion, startup sound, replay intro, reset CPU peak |
| SEL | Mute the selected track |
| ARP | Lock / evolve the random seed |
| REC | Make a variation; on SYSTEM, panic and clear the tape |
| ENV | Restore the patch before the last variation |
| LFO | Freeze / resume granular recording |
| PLAY | Start / stop; stop lets tails decay |
| SAVE | Store the patch and preferences while stopped |
| OCT −/+ | Change the live keyboard octave |
| Hold a function button for about 0.85 s | Explain that button and its current four-knob mapping |

Release a held button, then tap a function button to close help. That first tap only closes help. The browser includes all 27 chromatic keys and both octave buttons. The 27 keyboard keys remain musical triggers. MIDI notes on channels 1–6 play the corresponding tracks; other channels play the selected track. Notes trigger decaying voices, without a sustain-pedal or note-off envelope mode.

## Shape a sound

FILTER provides cutoff, resonance, OFF/LP12/LP24/LPG, and slow per-track filter movement. The nominal cutoff runs from 60 Hz to 18 kHz. Settings through 90% cover the warm range; the final tenth opens further. COLOUR also influences brightness. Use OFF for the most open dry signal, or LP12 with a high cutoff for gentle warmth. LP24 is deliberately darker. Resonance is neutral at zero and rises gently through the lower control range. Warm factory sounds start without filter resonance. Harmonic TONE selects a spectral region: high values can emphasise individual upper partials; choose a low value for a strong fundamental. Rings naturally rings: reducing its decay and tone shortens and softens that character.

On SOUND, K3 is SHAPE for melodic engines and DECAY for drums/Rings. FM at SHAPE 50 avoids strong internal feedback. Choir HARMONICS selects minor, major, suspended or fifth chords, quantised to the shared scale. SHAPE adds up to ±20 cents of detuning. Envelope times are independent: attack up to 8 seconds and melodic decay time constant up to 16.06 seconds.

## Biomes, memory and startup

The atlas has eight families and a Blank Biome with six keyboard-playable Choir tracks and no sequencer events. The factory catalogue is deterministic and lives in flash. `tools/generate_presets.py` generates `presets/catalog.json` and `app/dsp/preset_bank.inc`; regenerate, then rebuild device and browser after editing it.

SAVE holds **one personal complete patch**, loaded on restart. Factory Biomes remain available. Granular audio is not stored. The browser editor exports and imports complete patch files, so you can keep a personal file library. Stop before import. Files do not include the live granular recording or display preferences, and do not transfer to hardware. The device diagnostic export remains a read-only backup, without a general upload interface. Patch layout version 4 is 652 bytes; the firmware storage reader also supports migrations from versions 1–3.

The animated flower-logo intro lasts 3.2 seconds and any control dismisses it. Its quiet two-second chime follows the master volume. GLO → GLO → GLO opens LOOK: K1 switches live-view motion, K2 switches the startup sound, K3 replays the intro. Stop + SAVE retains the two preferences. Live-view motion and startup animation are distinct. Preferences occupy a separate flash object from panel calibration.

## Audio architecture

Selected original Plaits bass drum, snare, hi-hat, FM, virtual analogue and additive algorithms; the original Rings modal resonator; and our own three-sine Choir. These are not complete Plaits or Rings ports. The granular bus is our own implementation, not a Clouds or Beads port.

- Stereo output at 44.1 kHz, 128-frame hardware callbacks, internal blocks up to 32 frames. No dynamic allocation in audio rendering.
- Grain scheduling follows fragment duration, waits for free slots and finishes existing grains through quality changes. Hann windows give smooth beginnings and endings.
- The 256 KiB tape holds 131,072 mono PCM16 samples at **44.1 kHz / 2.97 seconds**. Four-point interpolation, safe trajectories around the recording seam and boundary fades soften transitions.
- Individual voices remain linear. A protective soft knee acts at the tape input and final mix. Low-frequency DC removal retains bass detail. The tape records at the full sample rate. Feedback remains bounded and filtered.
- ECO: two grains, eight Rings modes, half-rate FM/VA/Harmonic with compensated pitch and linear reconstruction. FULL: four grains, 24 Rings modes and full-rate melodic engines. Dense FULL combinations automatically enter ECO. This is a deliberate hardware limit; not all six-voice combinations run in FULL.
- Individual Euclidean lengths 1–32, hit count, rotation, chance and division; shared swing up to 65:35, 1–4 repeats, accents, eight scales, root and transposition. Trigger-based ducking accepts any source track; it is not an audio compressor.

Start with a modest master level. Model/Biome changes still reset voices and are not seamless scene crossfades. Pitch-shifted granular audio can retain texture and aliasing; the tape is PCM16, not an unlimited-bandwidth studio processor. Objective tests cover defined cases, not every subjective timbre.

## Validation and recovery

The first public release is **1.0**, device identity **FM-1_710010000**. The exact 1.0 build has passed physical installation, six-voice stress, all Biomes and sound recalls, Grain Size checks, a ten-minute soak, saved-state persistence and a return to original V15 followed by reinstallation. Its measured results carry the release package identity and SHA-256. Earlier comparative evidence retains its original identifiers.

See `docs/RELEASE-VALIDATION.md`, `docs/release-evidence/` and `docs/Recovery-Guide.txt`. Device results identify the exact firmware package by SHA-256. Sound-quality comparisons identify their separate baseline rather than presenting it as a release measurement.

A return to the original manufacturer V15 and back was physically verified on the development device before the final release audit. The unchanged update path also has package integrity, cancellation and boot tests with the release payload. That V15 round trip retained the Mutable Dreams patch but reset motion/start-sound preferences. The ROM rescue path is prepared, not exercised by deliberate damage. Keep a separate backup of your own work before changing firmware.

## Build and test

Python 3.11+, Pillow and a C/C++ compiler are required locally. WebAssembly uses WASI SDK 34.0 via `WASI_SDK`.

- `python3 tools/build_local.py host`: native application simulator.
- `python3 tools/build_local.py dsp --sanitize`: build the general audio tests; run `app/build/san/dsp-tests`.
- `python3 tools/build_local.py quality --sanitize`: build controlled granular/linearity regressions; run `app/build/san/audio-quality-tests --enforce`.
- `python3 tools/build_local.py web`: shared application compiled to WebAssembly.
- `python3 tools/package_web.py Mutable-Dreams.html`: self-contained offline browser instrument.
- `tools/build_device.sh`: device build using JieLi pi32v2 and AC79 SDK. Set `JIELI_TOOLCHAIN`, `AC79_SDK`, optionally `PYTHON` and a platform runner. Compiler and SDK are not bundled.

The hardware tools require `tools/requirements-bench.txt`. `project_backup.py` is read-only; hardware control/library tests intentionally change the current patch and some save test state. Back up the user's work first. USB audio tools explicitly select the Mutable Dreams input and never default to a microphone.

No external audio input, freely wired audio patch graph, whole Mutable module collection, MIDI clock slave, Bluetooth/WLAN, sample import or song automation is implemented.

## With gratitude

Mutable Dreams owes its foundation to **Émilie Gillet and Mutable Instruments**. Their original algorithms, clear musical ideas and open source make this exploration possible. We preserve their source credits and MIT notices. Our work adds an FM-1 instrument around selected algorithms: Biomes, sequencing, a custom granular bus and a new interface.

[Original Eurorack source](https://github.com/pichenettes/eurorack), [Plaits documentation](https://pichenettes.github.io/mutable-instruments-documentation/modules/plaits/), [Rings documentation](https://pichenettes.github.io/mutable-instruments-documentation/modules/rings/).

New application and integration: MIT. Platform: fm1-base and felucca-bsp, MPL-2.0; see `base/LICENSE`, `base/LICENSING.md`, `base/CREDITS.md`. Fonts: SIL Open Font License, see `base/assets/fonts/`. This is an independent project, unaffiliated with and not endorsed by Mutable Instruments or M-VAVE. Mutable Dreams is the project name chosen for this work.

## Musical starting points and accessibility

Factory Biomes use consonant oscillator ratios, quieter secondary voices, shared chord tones, lower melodic motion and deliberate rhythm anchors. Harmonic presets favour the fundamental; Rings defaults use shorter decay. All controls remain editable, including more metallic settings. The complete tutorial is 3 minutes 56 seconds, with English narration and subtitles.

Held-button help is available directly on the display and in the browser. The optional external screen-reader protocol sends readable text without an unhosted dictionary URL; no external speech dictionary service is required or advertised.

## Installation

Use the release browser installer in desktop Chrome or Edge. It contains the verified 1.0 package, requests MIDI access only on an explicit installation action, and confirms the firmware identity after restart. It installs from original M-VAVE V15, updates Mutable Dreams, and accepts the official V15 file for returning to stock. The separate Python installer kit is an alternative. Read the Recovery Guide first.

## Additional GPL distribution for Firmware Hub

The combined program is additionally distributed under GPL-3.0-only. See `LICENSING-GPL.txt`, `LICENSE` and the retained `LICENSE-MIT.txt`. Upstream MIT, MPL-2.0, Apache-2.0 and OFL notices remain valid. This licensing addition changes no firmware behaviour. The original 1.0 firmware hash remains unchanged.
