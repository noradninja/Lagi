# Authentic pre-Azel PDS Latin font

The selector and both early `Loading...` draws share `frontend::LatinFont`.
This is a frontend service, not an initialization of Azel's native text system:
**Azel decides. Lagi services. Neptune renders.** No upstream Azel sources,
VDP2 VRAM, text tasks or native string state are changed.

## Source and mapping

Upstream `AzelLib/VDP2.cpp::loadFont()` loads `ASCII.CGZ` through
`unpackGraphicsToVDP2()`. Ordinary `printVdp2String()` uses printable ASCII
0x20..0x7E directly: top tile `0x63 + (character - 0x20) * 2`, bottom tile
one later. Each glyph is 8x16, two consecutive 8x8 4bpp tiles, high nibble
first, with an 8-pixel advance. The rejected `resetVdp2StringsData` path and
the extended-character `loadCharacterToVdp2()` generator are not used.

The bounded decoder preserves the native interleaved, LSB-first control-bit
stream, literal, short/long back-reference and overlapping-copy behavior.
It rejects truncated streams, backward references before the output and
capacity overflow. Compressed input is capped at 64 KiB, output at 32 KiB;
all 95 printable glyphs must be present. The tested US Disc 1 resource was
9,297 compressed bytes and decoded to 19,648 bytes.

Default native white palette bank 12 is copied from Azel's existing initialized
palette data without mutating it. Original tile outline/shading indices are
preserved; existing frontend label colors tint that palette. Index zero is
transparent. Glyphs are drawn at native pixel size without filtering.
Non-ASCII input uses the authentic question-mark glyph. Measurement and draw
share the same advance and newline handling.

## Resource ownership and layout

Initialization tries `<data root>/ASCII.CGZ`, then the user's Disc 1..4 images.
The scoped disc reader restores every mount field and selected-disc value on
success or failure and closes its file before restoring the mount. Missing or
invalid resources retain the diagnostic font. Initialization runs once before
the first Loading frame; repeated calls, including failure, perform no I/O.
No compressed font, decoded glyph atlas or game-derived preview is committed
or packaged. Users must supply their own game data.

Artwork, medallion positions, row positions, input, music, fade and boot flow
are unchanged. The existing selector frame cache still handles idle frames;
no per-frame font loading or allocations are introduced. Label anchors remain
unchanged; the selected-disc digit follows the measured prefix without overlap
(the diagnostic fallback retains its old digit position). Loading uses the same
measured font with a 16-pixel right margin and 14-pixel bottom margin.

## Validation

- Host decoder output matches the extracted original decoder byte-for-byte
  using the user's real ASCII.CGZ; all 9,297 truncated prefixes are rejected.
- Synthetic tests cover both copy forms, control-byte boundaries, overlap,
  invalid references, output bounds, clipping, pitch padding and failure reset.
- All 12,160 decoded glyph pixels match the native tile addressing; measurement,
  unsupported-character handling and blank space are checked.
- Actual selector/Loading composition helpers are exercised with real local
  resources, all four selections and both cached framebuffers. Host previews
  were visually checked for readable glyphs, alignment and clipping.
- Scoped-mount restoration and actual loader caching/fallback pass host tests
  for loose, disc-backed, missing and malformed resources.
- Vita syntax checks and full VPK build pass with all three flags below ON.

These are not Vita execution results. Confirm font appearance, startup,
approximately 60 FPS idle selection, music, navigation, fade and boot on device.
Successful loading logs `[FrontendFont]` with compressed/decoded sizes and
`nativeTextUntouched=1`; missing/invalid data logs the diagnostic fallback.

## PowerShell build

```powershell
cd E:\dev\Lagi
git switch feature/menu-save-load-restoration
git pull --ff-only
cmake -S . -B build-fullres -DLAGI_FULLRES=ON -DLAGI_STOCHASTIC_FILTER=ON -DLAGI_DITHER_LOOKUP=ON
cmake --build build-fullres
```

Host tests require an MSVC developer PowerShell environment:
`tests/frontend_font_test.ps1` accepts a local disc image path (see its parameter
block); `tests/frontend_font_io_test.ps1` needs no game data. Test previews and
temporary fixtures stay outside the repository by default.
