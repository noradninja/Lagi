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
