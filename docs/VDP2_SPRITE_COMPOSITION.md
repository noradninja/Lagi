# Experimental VDP1 world-image / VDP2 composition

Enable with `-DLAGI_VDP2_SPRITE_COMPOSITE=ON`. Default remains OFF until
Vita verification. This is a first supported composition route, not all VDP2
effects or a completed general Saturn compositor.

## Implemented route

Native Full/Textured, full-resolution scenes with type-3/mixed-RGB SPCTL,
sprite CC enabled, no color-calculation window, no NBG0/2 world layer and no
destination-alpha mode render world geometry/effects into a separate RGBA
surface. RBG0 is not prepainted into this surface. A new GPU pass samples the
completed world image and resolved RBG0 (or the existing black clear), applies
native priority selection and CCRS ratio/add mode, then leaves the established
NBG1/3 text/backing and foreground-UI ordering in place. Menus, diagnostic views,
half-resolution and unsupported modes keep the previous renderer.

Type-3 indexed colors retain their priority and ratio selector bits after CRAM
lookup, encoded in unused opaque texture alpha values 129..156. Direct RGB
retains alpha 255. Existing VDP1 half-transparent RGB results use reserved 128;
half-blend programs write this new result selector rather than preserving the
destination selector. Mesh holes remain absent pixels. Point sampling of the
completed surface prevents interpolation of selectors. Native material keys
already contain the complete CMDCOLR and PMOD; register ratios are resolved at
composition, not baked into cached texture RGB. No effect-specific tints or
new water geometry are introduced.

The shader performs ratio arithmetic in RGB555 with denominator 32 and clamps
additive output. Existing VDP1 half blending still uses GXM's 8-bit alpha;
this does not claim pixel-exact VDP1 framebuffer emulation.

## Explicit limitations

- Only the completed **world** VDP1 contribution is in this pass. Foreground
  HUD/cursors remain in the existing late UI path; not all sprite priorities
  and VDP2 layers are interleaved by a general top/second-source resolver yet.
- NBG0 clouds, NBG2 world layers, color-calculation windows, destination-alpha,
  line-color screen selection, sprite shadows and non-type-3 layouts are not
  enabled by this change. RBG0's own color calculation and extended color
  calculation also fall back rather than treating a translucent top RBG0 as
  opaque. Unsupported world configurations retain the previous route.
- BACK uses the renderer's existing black clear, not a new native BKTA decoder.
- Allocation is about 2.13 MiB for the extra 1024x544 surface. An additional
  render-target transition and full-screen GPU pass require hardware profiling.
- VDP1 blending against empty/indexed destinations retains the existing GXM
  approximation; tests do not establish complete destination-MSB semantics.

## Validation and acceptance

`tests/vdp2_sprite_color_test.ps1` checks all priority conditions, ratio
arithmetic across all 32x32x32 channel combinations, every type-3 selector
encoding, and world-pass placement before NBG text. The Vita shader compiler
and VPK build are separate checks; neither proves appearance/performance.

Test normal combat effects, overlapping effects, stippled particles, camera
movement, HUD and return to flight. Logs report `VDP2SpriteComposite active=1`
only for eligible configurations and log successful GPU resource preparation.
Compare the Saturn reference without changing palette saturation globally.
Capture fresh ScenePerf timings, since the added pass has not been measured.

The experimental-ON battle VPK and default-OFF non-battle VPK both compile
with FULLRES, STOCHASTIC_FILTER and DITHER_LOOKUP ON. The two sprite color
tests pass on the host. These are build/math checks, not Vita acceptance.

For the existing Vita build directory:

```powershell
git pull --ff-only
cmake -S . -B build-battle-vita -DLAGI_VDP2_SPRITE_COMPOSITE=ON
cmake --build build-battle-vita -j 32
```

To compare the existing route, reconfigure the same option OFF and rebuild.

The A3 static-material disc audit still finds no half-transparent PMODs in
its visible outdoor model grids. This pass can restore eligible VDP2 blending
against RBG0, but does not prove the missing layered water contribution exists
or is submitted. NBG0 cloud work and the remaining water trace stay open.
