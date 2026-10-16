# Disc selection frontend

Lagi's disc selection is a platform frontend that runs **before** native Azel initialization. It does not reproduce Saturn disc-swap tasks or replace Azel's save/load logic.

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
