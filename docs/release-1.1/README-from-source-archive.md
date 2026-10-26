# Mutable Dreams 1.1

Release source prepared on 25 October 2026. See `docs/release-1.1/QUALIFICATION.txt` for the exact build, checks and remaining limits. Published 1.0 is unchanged.

Six assignable tracks and twelve engines: the original Plaits/Rings-based voices, Round Bass, Elements Strike, Mellow Drift and original Satin Morph. There are 224 factory sound presets, 76 musical Biomes plus Blank, sixteen named user Biomes and sixty-four user sound slots. Global mixer, granular bus, shared delay/room, editable sixteen-step phrases, USB/TRS MIDI input and Marbles/Tides/Warps-inspired functions are included. These are selected module functions, not full ports of every Mutable Instruments module.

The device interface has nineteen pages, translucent leaf cards, curved bud sliders, tapered mixer reservoirs, rasterized Nunito headings, organic backgrounds, cached animated contours, track meters, subpixel live blooms and contextual held-key help. The browser build runs the same application/DSP source. Browser storage is local and separate from hardware banks. Connect FM-1 enables a live Web MIDI editor; patch JSON exchanges one live Biome, while .mddb backups restore the complete saved project, preferences and all User Banks.

The Hub comparison and full request audit are in `docs/development/FIRMWARE-HUB-AUDIT.txt` and `REQUIREMENTS-AUDIT.txt`. Block optimizations preserve the reference audio; the separate Plectrum fix intentionally restores the main impulse during rapid retriggers. Audio callbacks now contain 256 frames (5.80 ms), while DSP tiles remain 64 frames. End-to-end latency has not been measured. The second core stays disabled.

## Building

Use `tools/build_local.py` with `host`, `dsp`, `quality`, `next`, or `web`; native DSP checks accept `--sanitize`. Set `WASI_SDK` for web. `tools/package_web.py` creates the standalone browser instrument. The device package is built with `tools/build_device.sh` after configuring the Jieli toolchain and AC79 SDK. Apple Silicon build repair and the Blink patch are documented in `docs/development/BUILD-ENVIRONMENT-FIX.txt`.

## Data and recovery

Patch version 6 is 1,292 bytes and imports older Mutable Dreams patches. `tools/device_editor.py export/import` exchanges the live patch without implicitly saving it. `tools/dreams_backup.py backup/restore/info` manages the saved project, preferences and all 16/64 User Banks in a CRC-checked `.mddb` image. Restore validates formats and parameter ranges, stages A/B records and requires physical SEL. It excludes calibration, transient granular audio and session MIDI settings. Save while stopped and using internal clock; occupied slots require confirmation.

`tools/project_backup.py --user-banks` remains a read-only legacy export. Read `docs/development/LIVE-EDITOR-PROTOCOL.txt` and `Recovery-1.1.txt`. The V15 roundtrip test erased User Banks and preferences: a separate `.mddb` backup is essential. Current release evidence is in `docs/release-1.1/QUALIFICATION.txt`; historical development evidence remains separately for provenance. No blanket claim covers every third-party firmware path.

Run `tools/test_transfer.py` after a sanitized host build. It checks app validation, bounded uploads, all bank slots and the actual 22-object flash map under simulated interrupted writes. Browser protocol and shared WASM tests are under `tests/`. Firmware and backups are separate; reinstalling firmware does not restore personal banks.

## Credits and licensing

The GPL combined distribution and original MIT/MPL-2.0 component notices are retained. Elements Strike uses original Mutable Instruments code; see `docs/development/ELEMENTS-PORT.txt`. Satin Morph is original code. No JIANT or OberVave implementation was copied into this snapshot. The second-core/reference assessment and the product-video brief are under `docs/development/`.

Historical 1.0 and earlier development reports remain for provenance and do not certify this candidate. The completed developer handoff archive is separate and unchanged.
