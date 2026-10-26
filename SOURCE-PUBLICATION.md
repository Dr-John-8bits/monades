# Browsable source publication - 26 October 2026

Mutable Dreams 1.1 is now available as an ordinary source tree on the repository's default branch, following The Firmware Hub's request. Application/DSP, selected upstream modules, platform/BSP, build tools, tests, presets, web sources and licence notices can be inspected directly on GitHub.

The import comes only from the previously published `Mutable-Dreams-1.1-Source.zip`:

```text
SHA-256: 90fef68214bd1ff6674db2c80852cc2291b31321e9c26cb20cf59d45ee04af40
Files:   807
```

Every archived file is preserved byte for byte. The archive README is stored as `docs/release-1.1/README-from-source-archive.md` to keep the public repository README and its newer manual links. The root GPL notice now also includes the archive's existing 1.1 addendum; all original licence text remains intact. Additional repository documentation and the comparison helper are separate from the archived source.

Run `python3 tools/verify_published_source.py` to repeat the comparison. See [BUILDING.md](BUILDING.md) for clone/build commands and dependencies. The historical archive manifest remains [SOURCE-SHA256.txt](SOURCE-SHA256.txt).

The newly extracted tree was compiled as a native host build with AddressSanitizer and UndefinedBehaviorSanitizer. The existing editor/storage-transfer tests and DSP suite passed. This used a compatible macOS 15.4 SDK after the locally selected newer SDK failed to link with the available linker. All archived application and build sources remain unchanged.

No firmware code, presets, storage formats, binary release assets or release tags are changed by this publication. The published 1.1 package still has identity `FM-1_710010100` and SHA-256 `8e039bb0e9681fbbb0504e9b7d251895958fd88ae97f11ff0342b4aef38143d2`. Published 1.0 remains unchanged. There is no new firmware version and no need to reflash.

Current-branch GitHub ZIP downloads include the expanded source tree. The unchanged historical release tags retain their previous repository layout; their named source assets remain the frozen release snapshots. This documentation does not claim a new device test or an independent byte-identical firmware rebuild.
