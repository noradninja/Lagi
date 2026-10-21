# Lighting color retention audit

Scope: shared town/cinematic, actor, field/environment, point-light and VDP1
effect color transport. No replacement tints or grayscale conversions added.

- Town/cinematic camera setup reads the native RGB descriptor and three separate
  RGB falloff endpoints. `townCamera_draw` passes the packed native color to
  `setupLight`; it is not replaced with white.
- `capture_current_light` copies all three native light color and falloff values.
  Removing the view transform rotates the direction, not the color.
- Dynamic model and cached/static renderer payloads retain those channels.
  Native array ordering is B,G,R: the shared Gouraud evaluator uses indices
  2,1,0 for output R,G,B, matching `ComputeColorFromNormal`.
- Point-light color remains distinct from normal directional/base lighting.
  The shared additive evaluator combines their per-channel contributions before
  native quantization/clamping; it does not reduce point color to intensity.
- Particle/effect Gouraud tables retain separate R/G/B corner values. The shader
  interpolates all three independently and adds them to the base texture.
- Lighting-only subdivision and scanline shader variants output RGB, despite
  their historical `gray` filenames / `GouraudGrayscale` enum. They consume the
  shade payload used by full mode and do not average it into monochrome.

The environment/actor host fixture now specifically checks unequal channels
`3,17,29` and three independent falloff values in cinematic-style capture, both
with and without camera rotation, through the actual renderer cached payload.
Existing point-bridge, status-light and 384-case particle color fixtures pass.
This validates the shared paths, not every authored effect or GPU screenshot.

Setup-only `[NativeCameraLight]` diagnostics report authored RGB and all three
falloff colors for each town/cinematic camera-light request. They do not log per
frame or alter the lighting. A hardware lighting-only/full-mode comparison is
still needed to verify each scene's authored appearance and any missing effect
tasks; preserved color transport alone cannot prove those tasks execute.

## 2026-10-20: cinematic actor light basis

Scoped cinematic actor geometry removes `g_viewScopeMatrix`, but its light
previously always removed `g_nativeSceneViewMatrix`. Those roots can differ
when an actor Draw precedes the scene camera capture. This rotates light relative
to otherwise correctly placed actors. The shared capture now uses the same root
as geometry, including the case where only the scoped root is valid. RGB,
falloff, native normals and light strengths are unchanged. The production
fixture tests a quarter-turn actor root against an identity scene root, with
and without a valid scene view. This establishes the coordinate-space correction,
not that every screenshot difference (including floor/sky palettes) is resolved.
