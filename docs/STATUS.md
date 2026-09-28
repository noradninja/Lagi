# Lagi Development Status

Last updated: 2026-09-28

Lagi is currently in native PlayStation Vita bring-up. The project is executing reconstructed Panzer Dragoon Saga runtime/data paths directly on ARMv7 and is not using Saturn CPU emulation for the primary runtime.

## Proven on real Vita hardware

The following milestones have been verified on hardware:

- Vita process/platform startup.
- Double-buffered 960x544 debug framebuffer.
- Azel fixed-point runtime.
- Azel heap allocator.
- Azel hierarchical task scheduler.
- Root task creation plus Update/Draw execution.
- Big-endian Saturn memory readers and `sSaturnPtr` traversal.
- Direct Disc 1 CUE/BIN parsing from `ux0:data/lagi/Disc 1/`.
- ISO9660 filesystem traversal inside the MODE1 BIN track.
- `COMMON.DAT` loaded directly from the disc image.
- 9 dragon-level stat tables parsed.
- 27 battle overlay descriptors parsed.
- 27 battle activation entries parsed.
- 79 sound configuration records parsed.
- Dragon COMMON morph/model and animation metadata parsed.
- `DRAGON0.MCB` model hierarchy correlated against COMMON.DAT.
- Basic Wing hierarchy: 31 bones.
- Basic Wing hotpoint data: 6 decoded hotpoints.
- Basic Wing geometry: 31 model nodes, 300 vertices, 212 Saturn polygons.
- Vita SELECT toggles the debug/status screen.

## Current renderer milestone

The current branch is bringing up the first native SceGxm model viewer. Its minimal color shaders are vendored as precompiled GXP byte arrays so the project builds with standard VitaSDK and does not require psp2cgc.

Target behavior:

- Load the validated Basic Wing hierarchy and default pose.
- Transform hierarchy-local geometry into a usable debug mesh.
- Preserve each original Saturn quad as one debug-colored polygon, even though GXM receives two triangles.
- Render through native SceGxm.
- Left analog stick rotates the viewer.
- Triangle resets the view.
- L/R switches debug render mode.
- SELECT switches between runtime status and the 3D viewer.

The renderer is intentionally untextured at this stage. Its purpose is geometry and hierarchy verification before adding Saturn texture and shading semantics.

## Debug screen

The runtime status console records each completed hardware milestone rather than reusing a single screen color. Current PASS lines include Saturn memory, Disc 1 mount, COMMON.DAT tables, sound table, dragon COMMON data, DRAGON0 hierarchy/hotpoints, decoded geometry, and task runtime status.

SELECT is reserved for Lagi's debug UI because the Sega Saturn controller has no SELECT button.

## Rendering direction

Lagi is moving toward a native SceGxm renderer rather than VitaGL. The long-term renderer should translate reconstructed VDP1/VDP2 intent directly to the Vita GPU while preserving important Saturn presentation behavior. Resolution is the intentional exception: final rendering targets the Vita's native 960x544 framebuffer rather than the Saturn's lower native resolutions. Preserved behaviors include:

- low color precision / RGB555-style quantization,
- Gouraud shading semantics,
- nearest-neighbor texture behavior,
- mesh transparency,
- half-transparency behavior,
- VDP2 layer/color calculation,
- native Vita 960x544 presentation rather than Saturn-resolution rendering.

## Next milestones

1. First visible Basic Wing native GXM render.
2. Confirm hierarchy pose/orientation visually.
3. Add per-model/per-polygon/wireframe debug modes.
4. Validate winding and culling.
5. Decode Saturn textures and palettes from DRAGON0 data.
6. Add VDP1-compatible texture and color behavior.
7. Expand renderer from the Basic Wing test path into reusable PDS model submission.

## Compatibility notes

- VitaSDK GXM API compatibility: GPU mapping helpers now use `SceGxmMemoryAttribFlags` directly, and the default clip/viewport call supplies explicit `959, 543` bounds for the 960x544 render target.

- C++20/VitaSDK enum compatibility: the GPU allocation helper accepts combined GXM memory attribute masks as an integer and performs the enum cast only at `sceGxmMapMemory()`, avoiding strict-enum failures from bitwise OR expressions.

- Linkage fix: vendored GXP symbols are defined with explicit external linkage so the renderer can reference the shader byte arrays across translation units.

- GXM bring-up is now non-fatal: shader/patcher failures remain on the existing status console and report the exact failing stage instead of closing the application. SELECT only enters the 3D viewer after both the GXM shader pipeline and Basic Wing mesh are ready.

## GXM startup isolation

A hardware regression caused the application to exit immediately after native GXM initialization was introduced. The current isolation build restores the previously proven double-buffered CPU framebuffer renderer and deliberately executes no GXM functions during startup or the frame loop. The GXM library and vendored shader data remain linked, but the Basic Wing viewer is temporarily disabled. If this build boots normally, the regression is isolated to executed GXM initialization/state setup rather than disc parsing, dragon geometry reconstruction, shader data linkage, or the existing platform/task runtime.

## GXM staged hardware probe

The framebuffer isolation build booted successfully, confirming the immediate-exit regression is caused by executed GXM bring-up rather than the existing platform/runtime or Basic Wing data path. GXM is now being reintroduced incrementally. Stage 1 binds SELECT to a one-shot `sceGxmInitialize()` probe only. No GXM context, render target, shader patcher, shader program, or draw submission is created. A successful probe adds `[PASS] GXM INITIALIZE` to the existing framebuffer console; a returned error code is displayed as `[FAIL] GXM INIT 0x........`.
