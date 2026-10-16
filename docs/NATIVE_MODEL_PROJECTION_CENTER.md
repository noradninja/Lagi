# Native model projection center

Hardware acceptance, 2026-10-16: the user confirms Dragon positioning is
correct after the preview-root capture fix. This correction is retained when
rolling back the separate tile cache, which regressed menu frame rate.

## Follow-up: missing preview-root capture (2026-10-16)

The latest hardware screenshot still shows the preview right of its circle.
The previous correction only runs when a native scene view was captured.
`dragonMenuDragonDraw` had no view-capture scope, so an otherwise valid local
origin could reach the fallback camera path without that correction. The menu
draw now opens an explicit native view-relative scope at its actual root,
before submitting model and shadow, and publishes that root even if a prior
scene view was captured. Normal actor scopes retain their existing behavior.
This supplies the missing integration dependency; it is not a guessed extra
screen-space shift. Host checks now execute the scope with absent/stale views,
explicit replacement, ordinary scope retention and null matrices in addition
to the324 projection cases. Generated preview and bridge Vita syntax pass.
The new root-scope change still requires hardware centering verification.

## Report and source boundary

The Dragon ability preview appeared down/right of its circular backdrop.
Azel `menu_dragonMorph.cpp` sets VDP1 local coordinates to `(120,112)`;
the normal center is `(176,112)` in its 352x224 canvas.
`3dEngine_flush.cpp` snapshots this per object and applies an NDC translation
of `2*(localX-176)/352`, `-2*(localY-112)/224` after projection.
Lagi previously retained the model matrix but omitted this local origin.

## Implementation

- Capture the local origin at every model submission through the scene bridge,
  which includes the actual Azel structure definition. No mirrored layout or
  end-of-frame global is used.
- Preserve default centered submissions and existing static caching. Mark
  non-default origins dynamic because the projection correction depends on
  the camera. This prevents a cached world mesh retaining an old camera shear.
- Convert the NDC offset into a view-space shear proportional to vertex Z,
  then invert the view basis to apply it in Neptune's existing world-space
  vertex batch. Use the same horizontal 4:3 presentation factor as projection.
  The inverse handles fixed-point basis rounding rather than assuming an
  exactly orthonormal matrix.
- This is algebraically the original post-projection translation: view Z,
  clipping depth and native lighting inputs remain unchanged. It introduces
  no additional shader, texture lookup, draw call or fragment work.
- No Dragon/menu-ID-specific positional guess; other native model origins
  use the same service. Native-view-less diagnostic fallback remains unchanged.
- Radar/UI sprites, Cinepak and texture filtering paths are not modified.

## Validation and remaining gate

Host `tests/projection_offset_test.ps1` executes the actual coordinate capture
and checks 324 projection combinations: three screen ratios, centered and
shifted origins, three depths, and four camera bases including rounding.
It checks NDC displacement and preserved depth. Source guards check submission
capture, cache bypass and renderer integration; these guards are not GPU tests.
Vita syntax checks cover both bridge files and the renderer.

No VPK build, deployment, performance measurement or hardware visual result
is claimed. Check the Dragon ability preview against its circle and verify
normal gameplay framing after returning. The newer branch documentation
(`ed92d5c`, `docs/NEPTUNE_RENDERER.md`) records maintainer hardware acceptance
of the earlier point-light position and color corrections. This projection
change does not replace that evidence or claim a new hardware lighting test.

PowerShell build on the user's checkout:

```powershell
cd E:\dev\Lagi
git switch feature/menu-save-load-restoration
git pull --ff-only
cmake --build build-fullres
```

The newer Map entry/exit child is preserved; the actual map viewport and
navigation remain unfinished (`docs/FLIGHT_MAP_MENU_BRINGUP.md`). Books page
integration, menu animation timing and other outstanding menu acceptance
checks also remain separate unfinished work.
