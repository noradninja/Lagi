# Disc selector GXM presentation

## Accepted baseline and scope

The font, selector layout, music, fade and boot flow were accepted by the user.
Idle selection was approximately 60 FPS, but changing selection dropped to
13 FPS. The old CPU compositor reused only its most recent completed frame;
each change repeated alpha composition, portrait-bound scanning, resampling,
gold tint and text rendering across the large artwork.

This change is separate from in-game menu performance. Main includes the
accepted menu/frontend work through `3e20726`. Hardware confirms field-map
viewing, scrolling and exiting, but map/Dragon screens remain 15 FPS and other
menus 20 FPS. No native animation multiplier or Azel source change is included.

## Implementation

Neptune initializes its existing GXM backend before Azel starts. Once per disc
availability mask, it uses the accepted CPU composition and authentic shared
font to prepare each selectable state, then uploads immutable RGBA textures.
There are at most four full-resolution state textures (8,355,840 pixel bytes,
plus allocation alignment); with no discs only the no-selection state exists.
Startup preparation is still CPU work and may increase initial startup delay.
This is GPU presentation of prepared states, **not** a new GPU sprite/tile/font
compositor. It deliberately preserves accepted source pixels and fitting.

Every selector frame submits one full-screen GXM quad, point-sampling the
960x544 state texture 1:1. Selection changes only choose the texture: there is
no CPU composition, full-screen memcpy, allocation or texture upload on a normal
change. An exceptional mount failure changing availability rebuilds the states.
Filtering/dithering of game content is untouched; this native-pixel frontend
texture needs no interpolation. Initial `Loading...` retains its shared font
and existing CPU startup presentation.

The fade draws that selected texture plus a black alpha-blended GXM quad,
using the existing 0..30 brightness sequence. Each completed GPU present waits
one VBlank. `end_frame()` consumes a GPU-owned-present marker instead of queuing
the CPU framebuffer or adding another wait, including the final black frame.
GXM scanout uses its actual padded 1024-pixel pitch; CPU/font/state texture rows
remain 960 pixels. This distinction is required to avoid garbled scanout.

The GPU is explicitly finished before scanout and before freeing mapped
resources. State textures/geometry are released at fade completion, before
`runtime_init()`, or on shutdown/failure. The completed shared GXM backend is
reused by native title/movie/game presentation. CPU loading-screen ownership
is restored for the handoff. Allocation/render failures retain the CPU selector
and fade path. Audio/input/disc choice and native task ownership are unchanged.

## Checks and device gate

`tests/launcher_gxm_test.ps1` executes the actual preparation, presentation,
fade and end-frame functions against a mock driver. Checks cover uploaded
state texels, zero recomposition/allocation over repeated changes, availability
changes, no-disc state, padded scanout pitch, no duplicate CPU presents, all
31 fade frames, partial-allocation cleanup, render failure and resource release.
The font test separately exercises the actual accepted composition with local
game glyphs/art. Menu surface/cadence regression tests pass.
The final full Vita VPK build passes with full resolution and both existing
dithering flags enabled; existing native array-comparison/GCC ABI notes remain.

The user subsequently confirmed the selection menu stays at 60 FPS throughout
selection changes. The fresh log reports approximately 16,661..16,669 us per
selection render/present, including VBlank wait; one-time preparation took
1,879,937 us. That is hardware acceptance of the exercised selector path, not
an in-game menu improvement. Ordinary/save menus remain 20 FPS, Map/Dragon 15.

The host checks themselves do not execute GXM or establish device FPS. Validate all four
directions repeatedly, idle/selection FPS, artwork/font alignment, music, the
half-second fade and Disc 1 boot. `[DiscSelectorGXM] ready` reports preparation
time and `selectionCompose=0`; selection logs report render/present duration,
including VBlank wait. A fresh same-session log should also include several
seconds each in ordinary menus, Dragon and field Map for the next performance
pass. The new capture includes two CPU-heavy menu ScenePerf samples, detailed
in `MENU_TILE_DECODE_PERFORMANCE.md`.

```powershell
cd E:\dev\Lagi
git fetch origin
git switch feature/menu-render-performance
git pull --ff-only
cmake -S . -B build-fullres -DLAGI_FULLRES=ON -DLAGI_STOCHASTIC_FILTER=ON -DLAGI_DITHER_LOOKUP=ON
cmake --build build-fullres
```
