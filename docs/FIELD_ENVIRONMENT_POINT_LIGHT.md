# Field collection point light on environment meshes

On2026-10-16 the user accepted the dragon/rider light appearance and requested
extending that light to the environment. This intentionally extends the original
collection effect's scope; it is not a claim that Saturn used this wider scope.
The older instruction not to extend the collection light is superseded by this
explicit request, not by a reinterpretation of the original executable.

## Implementation

After Azel sets the special point position, range and color, the field
dragon adapter captures those values by value. Native selector push/pop and
ordinary directional restoration remain unchanged. The environment does not
read stale point parameters after that cleanup, and the accepted actor path is
not changed.

The existing FLD_A3 native visibility-grid submission explicitly marks its
environment context. At the normal model draw boundary, that context uses the
frame-owned point snapshot and the **actual native view/model matrix**, through
the same fixed-point `native_point_light` evaluator as the accepted actor path.
Within its existing range, the point vector/color feed a separate additional
model-light payload. The Gouraud consumer adds its positive diffuse term to
the ordinary directional contribution. Ambient/depth falloff and authored
material colors are applied once, and the combined result is clamped and
quantized once. Outside its range, ordinary environment lighting is retained.
There is no arbitrary radius multiplier, per-item tint, inverse-square
replacement, texture-ID classification or forced smooth-normal generation.

This follows the native evaluator's model-origin distance and model-space
normal convention; it is not a newly introduced per-fragment/per-vertex distance
solver. Authored flat polygons remain flat; authored corner normals retain
Gouraud interpolation. Following the user's combined-lighting request, the
environment point light is additive, not a replacement for directional light.
`AdditionalModelLight` and `accumulate_additional_model_light` provide the shared
contribution path for future extra lights; only this field source is wired now.
The accepted native dragon/rider lighting behavior is unchanged.

The frame snapshot is cleared in `begin_frame`. Menus, billboards, task actors,
town geometry and unmarked contexts are not opted in. This covers the current
FLD_A3 field-grid environment, not all future town lighting sources or arbitrary
moving scenery. The native task ordering draws this field source before the
grid; future differently ordered sources require an explicit integration.

Neptune already refreshes static polygon lighting each frame independently of
the geometry/material cache. The extension therefore uses the cached-mesh
lighting path without forcing geometry rebuilds or new GPU uploads/draw passes.
It adds a native point evaluation per submitted marked model while active;
performance is **not yet measured on Vita**. Existing full and lighting-only
Gouraud shaders consume the same colors; no shader variant is added.

## Checks and hardware gate

`tests/environment_point_light_test.ps1` executes extracted production snapshot,
application and frame-reset code, and the actual renderer cached-light payload
consumer. It verifies copied parameters surviving native cleanup, preserved base lighting,
native versus world matrix, rotated normals, outside-range preservation,
frame-to-frame restoration, actor/UI exclusion and null matrices. Shared helper
checks cover RGB addition, back-facing rejection and overflow saturation. It also
generates the real source adapters and checks source/target/cleanup hooks.
The existing point capture/math/selector regressions and Vita syntax checks
are separate checks, not device appearance or performance evidence.

On-device: collect beside a wall/floor; confirm nearby marked environment
models receive the colored effect, the accepted dragon/rider appearance stays
unchanged, distant geometry keeps ordinary lighting, and all lighting returns
to normal when the effect ends. Repeat while moving/rotating the camera, then
enter/exit a menu and traverse a tunnel. Compare the same route's frame times.
The first eight eligible models log `[EnvironmentPointLight] applied=0/1`,
native distance and resulting vector; this is diagnostic, not a visual PASS.

PowerShell build:

```powershell
cd E:\dev\Lagi
git switch feature/menu-save-load-restoration
git pull --ff-only
cmake --build build-fullres
```

No full VPK build, deployment or fresh device test was performed in this
workspace. The user-accepted actor appearance does not automatically accept
this new environment extension.
