# Disc selection frontend

Lagi's disc selection is a platform frontend that runs **before** native Azel initialization. It does not reproduce Saturn disc-swap tasks or replace Azel's save/load logic.

## Current status (2026-10-16)

The existing layout, four medallions, controls, looping music, 0.5-second fade
and Disc 1 boot are user-accepted at approximately 60 FPS in full resolution.
The new authentic ASCII.CGZ frontend font preserves that layout and uses the
same implementation for selector labels and initial Loading text. The user
confirmed that the font works and accepted the subsequent label/plaque layout
adjustments. See [Frontend PDS font](FRONTEND_PDS_FONT.md).
The font sections below describe historical experiments, not the current path.

Selection changes were subsequently reported at 13 FPS despite 60 FPS idle.
The performance branch adds GXM presentation of immutable prepared states;
see [GXM presentation](FRONTEND_GXM_PRESENTATION.md) for resource ownership,
startup preparation cost and the pending hardware acceptance gate.

## Disc image layout

The disc loader looks in `ux0:data/lagi/Disc 1` through `Disc 4`. Each directory contains a CUE sheet and its referenced BIN file. Folder numbering, rather than arbitrary filename ordering, determines the disc number. The runtime uses the existing lowercase `ux0:data/lagi` root; place the folders there exactly as shown.

The ISO9660 adapter exposes:

- `disc::disc_available(1..4)`: checks whether a CUE selects a readable MODE1 data track with a valid ISO9660 primary volume descriptor, without changing the current active disc.
- `disc::mount_disc(1..4)`: mounts a specific disc for subsequent file access.
- `disc::selected_disc()`: returns the active disc number, or zero when no image is mounted.
- `disc::init()`: retains the legacy Disc 1 default for existing callers.

CUE support is limited to the formats handled by the existing parser (MODE1/2352 or MODE1/2048 with an INDEX 01). Audio-only tracks and other CUE layouts are not recognized as data sources.

## Planned pre-game menu

The selected concept is the **radial mechanical disc menu**, with disc entries displayed beside the circular mechanism. Disc 1-4 are always visible; a disc that fails probing is dimmed and skipped by directional navigation. The player confirms a valid disc before `azel::runtime_init()`. Cancellation must not silently boot an arbitrary disc. Menu artwork uses the Saga-inspired stone/metal motif, original patterned backdrop, pale typographic treatment, and segmented green selection arc.

The illustrative mockup is not a finished texture atlas or source asset. Production UI work requires separated texture assets, correctly sized/dithered text and a native GXM frontend presentation path. Vita's physical display is **960x544**. The user confirmed that 980x544 was a typo.

At launch: initialize the platform, scan all four slots, draw the selector, process pad input, call `mount_disc(selected)`, then initialize Azel exactly once. Preserve the selected mount throughout gameplay; reuse Azel's native save/load path for transitions.

## Status

The four-disc adapter, pre-Azel selector loop, D-pad navigation, Cross confirmation, mount handoff, and a procedural **960x544 radial-layout prototype** are implemented. The pregame prototype is drawn into Neptune's existing startup scanout framebuffer, not via a new GPU shader. It deliberately does not touch Azel's renderer or game logic. It is not the finished textured Saga-style artwork; converting the radial mockup to actual GXM-backed atlas assets remains a separate art/rendering task. **Building and Vita hardware verification remain pending.**


## Textured launcher hardware test (October 2026)

The launcher now supports layered visual assets before Azel startup. The native
resolution is 960x544. The source implementation is committed, but the actual
binary art pack is delivered separately because the connected repository write
interface does not accept binary uploads from this workspace.

Extract `lagi_disc_selector_vita_assets.zip` into `ux0:data/lagi` to create:

```
ux0:data/lagi/frontend/background.rgba
ux0:data/lagi/frontend/outer_frame.rgba
ux0:data/lagi/frontend/radial.rgba
ux0:data/lagi/frontend/gold.rgba
ux0:data/lagi/frontend/ivory.rgba
ux0:data/lagi/frontend/missing.rgba
ux0:data/lagi/frontend/highlight.rgba
ux0:data/lagi/frontend/row.rgba
ux0:data/lagi/frontend/info.rgba
ux0:data/lagi/frontend/legend.rgba
```

The files use a simple test-only format: two little-endian unsigned 32-bit
integers for width and height, followed by width*height tightly packed RGBA8888
pixels. The launcher loads and caches all ten assets on first presentation.
If one is missing or invalid, it retains the proven procedural prototype.
The composed artwork is blitted into the existing startup scanout buffer; this
is **not yet a Neptune GXM textured path**, and is not the long-term renderer
architecture. It does not affect the gameplay Neptune renderer or Azel tasks.

The graphics are newly generated design assets intended for testing. Right
list labels, missing-disc behavior, and the green selected-medallion overlays
remain live. The art pack includes a background, frame, radial mechanism,
available and missing medallions, selection ring, row plaque, information panel,
and legend panel. The text uses the existing startup font for now.

Suggested acceptance tests: verify all assets load; inspect alignment and
visual legibility; test only Disc 1 present; test multiple discs and selection;
confirm Cross mounts the chosen disc and transitions to native Azel; record
launcher FPS for a later GXM optimization pass.


## 2026-10-16 visual and handoff checkpoint

The first textured hardware run showed correct image loading and successful Disc 1 boot, but
around 3 FPS in the CPU alpha-composited selector. The right-side plaque icons were
redundant, the radial selected medallion did not reliably turn gold, and the legend and
info text need further optical alignment.

The next source revision:
- moves the golden medallion and green ring exclusively to the selected **radial** node;
- keeps right-hand rows as informational text, with bright selected labels and muted
  missing labels;
- corrects the navigation wrap formula;
- caches the static composed launcher until selection or availability changes, reducing
  idle CPU work;
- introduces a 30-vblank fade of the last presented launcher image before Azel starts;
- adds line-separated selection and fade logs.

**Not yet complete:** moving the launcher to native GXM textured quads, replacing the
startup/Loading... text with the authentic PDS font, and repairing the bottom plaque
source geometry rather than only adjusting placement. These are independent follow-up
integration steps. The current CPU caching is a temporary performance mitigation, **not**
a substitute for native GXM. Hardware testing is required for this checkpoint.


## Radial layout checkpoint after hardware run (2026-10-16)

Hardware measurements show ~59 FPS with static framebuffer caching and the
30-vblank fade works and mounts Disc 1 correctly. The renderer is still
CPU-backed (cached), not native GXM.

The four radial directions are now physically mapped: Disc 1 top, Disc 2
right, Disc 3 bottom, Disc 4 left. D-pad and right stick both target the
corresponding disc directly; missing discs reject selection. The right stick
requires a return to center before a new input edge is registered.

The left legend plaque and its text have been removed. Right rows show only
available discs, with highlighted text for the selected disc. Hollow plaque
icon apertures receive a dark fill. Radial medallions and selection rings are
centered using their visible alpha bounds instead of the source image canvas,
to counter transparent padding discrepancies.

Unique artwork requires four independent sprite images; only generic gold
and ivory sprites were in the initial pack. The menu can optionally load
`ux0:data/lagi/frontend/disc_1.rgba` through `disc_4.rgba`, preserving the
same RGBA header/payload format as the existing assets. Missing optional
icons use the previously provided sprites. A selected ivory emblem is tinted
gold rather than replaced with another emblem. This is a temporary asset
fallback, and does not itself create new unique symbols. These optional
assets are not required to boot or pass initial visual tests.

Pending: authentic PDS font for launcher and Loading...; native SGX/GXM
launcher composition; hardware-check of the new alpha-centered alignment.


## Optional selection-screen background music

Place `bgm.mp3` in `ux0:data/lagi/frontend/`. The track is optional and
intentionally not bundled inside the VPK or mounted game disc. The frontend
MP3 worker decodes frames through the public-domain `minimp3` decoder and
streams PCM through Lagi's existing SceAudio output service. The track loops
while the disc selector is active. The worker and audio port are stopped and
released after disc selection **before** Azel audio initialization, so
Cinepak/SCSP retain their normal audio ownership. A missing track simply
leaves the selector silent. Diagnostics start with `[FrontendMusic]`.

Repository copy convention: `assets/frontend/bgm.mp3` (user-supplied;
optional). VPK rebuild is necessary for the new decoder code, but changing
only the MP3 later requires no rebuild. Looping audio has since been confirmed
on hardware by the user.


## Historical font investigation (superseded)

Azel's upstream `AzelLib/VDP2.cpp` implements `loadFont()`, which reads
`ASCII.CGZ` from the mounted disc and calls `unpackGraphicsToVDP2()` to
populate source character graphics in VDP2 VRAM. `loadCharacterToVdp2()`
then expands those graphics into a 16×16 VDP2 character with edges/shading
determined by adjacent source pixels.

The current launcher, however, renders before `azel::runtime_init()`.
Do not initialize Azel just to get the menu font. Keep the following work
separate from native VDP2 glyph rendering:
1. Build a bounded read/decompression path for `ASCII.CGZ` sourced from the
   user's mounted disc, using the same decoder semantics as upstream Azel.
2. Reproduce the character mapping and 16×16 output generated by
   `loadCharacterToVdp2()`, including the default character source map.
3. Make the result a shared preboot glyph atlas used by the disc selector and
   the early `Loading...` frame, with measured advances and baseline.
4. Retain the tiny diagnostic 5×7 font as a fallback only if extraction fails.
5. Verify visible glyphs and text metrics on Vita before replacing all labels.

No proprietary font data should be checked into Git. Preserve game-owned
font behavior after Azel initializes. Do not claim that a generic substitute
font is an authentic PDS extraction.


## Rejected glyph-source text checkpoint (2026-10-16)

The default glyph source data is already linked into Lagi from Azel's
`common_generated.cpp` as `resetVdp2StringsData[4106]`. Azel's
`resetVdp2StringsSub1()` chooses the 16-bit glyph rows at `pData + 8`
when the default font type is 5. Each of the 256 character entries contains
16 rows of 8 two-bit coverage values. The preboot renderer now reads these
rows directly, without asking Azel to initialize VDP2 or mount a new disc.
This preserves the **source character shapes** of the native font, but uses
a simple coverage presentation rather than Azel's full neighboring-pixel
outline expansion in `loadCharacterToVdp2()`.

The launcher labels and initial `Loading...` text use the new 8x16
glyph source path, with a 9-pixel advance. The existing debug 5x7 renderer
remains intact. The precise palette, spacing, and outline treatment require
Vita visual validation; do not describe this stage as byte-for-byte native
font rendering. All other VDP2 font ownership remains with Azel.

## Font validation failure and rollback (2026-10-16)

Hardware testing showed corrupted text from the experimental `resetVdp2StringsData` glyph path. That type-5 table represents extended character sprites, **not** the ordinary ASCII font. Azel loads the regular character graphics from `ASCII.CGZ` using `loadFont()` and `unpackGraphicsToVDP2()`. The incorrect preboot glyph renderer was removed, and the prior readable text plus its baseline coordinates were restored. Implement the real `ASCII.CGZ` decoding path and confirm mapping on Vita before enabling it for either selector or `Loading...`. The left and right portrait centers were independently adjusted 2 pixels left and 2 pixels up per hardware feedback.


## VPK-installed frontend resources (October 2026)

The shipping VPK now installs frontend RGBA assets at `app0:frontend/`.
The frontend renderer first opens these packaged assets, falling back to
`ux0:data/lagi/frontend/` for older installs. The optional looping music
uses the same preference order for `bgm.mp3`. BIN/CUE game discs are **not**
bundled and still belong in `ux0:data/lagi/Disc N/`.

On CMake configure, the packager includes all required PNG-converted RGBA
sprites from `assets/frontend/`. When present at configure time, it also
includes `bgm.mp3`, `navigate.wav`, and `confirm.wav`. Missing optional
audio files are not silently synthesized: add the files to the repo and
rerun `cmake -S . -B build-fullres -DLAGI_FULLRES=ON` before building.
The WAV menu cues now contain PCM rendered from the user's Disc 1 game data
through the same 68K/SCSP sound core used by Lagi. Native Azel's title menu
calls system effect 10 for movement (COMMON config: bank 1, sound 2) and
effect 0 for confirmation (bank 0, sound 0). The render loads the original
`SDDRVS.TSK`, `AREAMAP.SND`, `SYS_SE.SEQ`, `SYS_SE.BIN`, `DRG_SE.SEQ`,
`DRG1SE.BIN`, and `A5DSP.EXB`, then follows the native initialize/map/effect/
mixer commands and area-map enable sequence. Only the trailing digital silence
is removed from each 44.1-kHz stereo 16-bit WAV. No video-capture audio is
used in the package.

Disc-selection changes queue the movement cue only when the selected disc
actually changes. X queues the confirmation cue before mounting; the existing
music stream carries both cues through the 30-vblank fade and releases audio
before Azel initializes. Frontend music is held to a 4096-frame input queue
so the mixer does not hide cues behind the normal long PCM ring backlog.
Local PCM and build checks do not establish Vita
audibility or the final balance against BGM; device verification remains open.

The 960×544 fitted radial asset can be authored offline as a complete
full-resolution transparent atlas, which the renderer presents at 1:1 scale.
Legacy smaller radial art still uses the old fitted composition. Row art is
also presented 1:1. Avoid runtime enlargement or reduction for the new
assets. Existing image files must be replaced in `assets/frontend/` and
rebuilt to change the installed look; packaged assets have priority over
legacy ux0 overrides.
