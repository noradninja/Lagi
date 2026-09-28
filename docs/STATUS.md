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

- GXM staged probe result: Stage 1 `sceGxmInitialize()` passed on hardware. Stage 2 now tests allocation/mapping of the VDM, vertex, fragment, and fragment-USSE ring buffers only. No GXM context, render target, surfaces, shaders, or scene submission are created yet.

- GXM staged probe result: Stage 2 ring-buffer and fragment-USSE allocation/mapping passed on hardware. Stage 3 now creates only the `SceGxmContext` using those proven buffers. No render target, color/depth surfaces, shader patcher, programs, or scene submission are involved yet.

- Stage 3 build fix: added the standard `<cstdlib>` header required for `std::malloc` / `std::free` used by the GXM context host-memory probe.

- GXM staged probe result: Stage 3 `sceGxmCreateContext()` passed on hardware. Stage 4 now adds only `sceGxmCreateRenderTarget()` at 960x544, single-scene, no multisampling. Color/depth surfaces, shader patching, programs, and scene submission remain disabled.

- GXM staged probe result: Stage 4 render-target creation passed on hardware. Stage 5 now adds a dedicated 960x544 linear A8B8G8R8 color surface, sync object, and tiled S8D24 depth/stencil surface. Shader patching, shader programs, scene begin/end, and draw submission remain disabled.

- GXM staged probe result: Stage 5 color/depth surface setup passed on hardware. Stage 6 now allocates the shader patcher backing buffer plus vertex/fragment USSE pools and calls only `sceGxmShaderPatcherCreate()`. Shader program validation/registration, patched program creation, scene submission, and drawing remain disabled.

- GXM staged probe result: Stage 6 shader patcher creation passed on hardware. Stage 7 now validates the vendored vertex/fragment GXP blobs with `sceGxmProgramCheck()` and registers both with the shader patcher. Patched vertex/fragment program creation, scene submission, and drawing remain disabled.

- Debug console readability: reduced framebuffer status/body text scale by 50% and tightened line spacing, while keeping the title larger. This allows substantially more GXM bring-up diagnostics to remain visible on the 960x544 status screen.

- GXM staged probe result: Stage 7 precompiled GXP validation and shader-patcher registration passed on hardware. Stage 8 now resolves `aPosition`, `aColor`, and `wvp`, then creates the patched vertex and fragment programs using the Basic Wing debug vertex layout. Scene begin/end, render state, vertex/index submission, and drawing remain disabled.

- Debug console capacity: increased stored status lines from 20 to 64. The Stage 8 probe exceeded the previous cap after the earlier integration and GXM milestones, so later results could execute successfully but were not visible on-screen.

- GXM staged probe result: Stage 8 shader parameter resolution and patched vertex/fragment program creation passed on hardware. Stage 9 now begins and ends a single empty scene against the dedicated GXM color/depth surfaces, binds the patched programs and conservative fixed state, calls `sceGxmFinish()`, but submits no vertex/index buffers and issues no draw call.

- GXM staged probe result: Stage 9 empty scene begin/end passed on hardware. Stage 10 now allocates a three-vertex/three-index debug triangle, uploads an identity `wvp`, binds stream 0, and calls `sceGxmDraw()` into the dedicated off-screen GXM color surface. The GXM render target is still not sent to the display, keeping scanout/display-queue behavior out of this test.

- GXM staged probe result: Stage 10 off-screen triangle draw passed on hardware. Stage 11 now queues the dedicated 960x544 GXM color surface directly to `sceDisplaySetFrameBuf()` after the draw and waits for vblank. The normal CPU framebuffer flip is suppressed while this GXM surface is displayed. A successful test should visibly show the hard-coded RGB triangle.

- GXM staged probe result: Stage 11 displayed the hard-coded RGB triangle correctly on real hardware, proving the complete GXM draw-to-scanout path at 960x544. Stage 12 replaces only that triangle payload with the reconstructed/posed Basic Wing debug mesh. The model receives a fixed CPU-side presentation rotation, centering, and normalization into clip space; the already-proven identity `wvp`, color shader, depth surface, indexed triangle submission, and direct display scanout remain unchanged. Per-polygon debug colors are preserved. Interactive rotation and wireframe remain deferred until this fixed-view model draw is hardware-proven.

- Stage 12 hardware result: PASS. The reconstructed Basic Wing rendered correctly on real Vita hardware through the native GXM pipeline at 960x544. The fixed CPU-side pose/presentation transform produced a coherent dragon silhouette with correct-looking hierarchy placement and proportions, and per-polygon debug colors made the 212 Saturn polygon records visually inspectable. This validates the current Saturn-data -> hierarchy/pose reconstruction -> debug mesh -> GXM draw -> display scanout path end-to-end.

- Interactive Basic Wing viewer: the Stage 12 hardware-proven model render is now persistent and interactive. SELECT toggles between the CPU framebuffer status console and the native GXM viewer; left stick rotates yaw/pitch; Triangle resets to the initial presentation angle; L/R toggles filled triangles vs wireframe. The renderer keeps the proven identity-WVP shader path and rotates the small normalized debug mesh on the CPU before each GXM draw, minimizing new GPU-side variables.

- Interactive viewer hardware fix: video testing showed the Basic Wing disappearing into small fragments during free rotation. The mesh itself remained coherent; the issue was clip-space depth. Because the debug viewer deliberately keeps an identity WVP, CPU-rotated vertices were centered around z=0 and could cross the near clip boundary. Viewer depth is now remapped to a safe positive normalized interval around z=0.5 after rotation, preserving depth ordering while keeping the complete model inside the clip volume.

- Interactive viewer scanout fix: hardware video showed a horizontal partial-frame flicker that also corrupted the PSVshell FPS overlay, confirming a front-buffer scanout race rather than model geometry. The viewer had been clearing/redrawing the same single GXM color buffer that the display was actively scanning. The GXM viewer now owns two 960x544/pitch-1024 color surfaces and sync objects. It renders only into the back surface, finishes the GXM scene, queues that completed surface with `SCE_DISPLAY_SETBUF_NEXTFRAME`, waits for vblank, then flips the GXM draw index. Buffer 0 remains the initial proven model frame and buffer 1 is the first interactive back buffer.

- Basic Wing viewer camera: replaced the temporary identity-WVP/clip-space rotation path with a real perspective transform. The normalized dragon mesh now remains static in model space; yaw/pitch are applied in the WVP matrix, the model is translated 3 units forward in a left-handed view, and a 50-degree vertical-FOV perspective projection uses the native 960/544 aspect ratio with 0.1/100 near/far planes. This removes the intentionally orthographic-looking bring-up projection and avoids per-frame CPU vertex rewrites while preserving the hardware-proven GXM shaders, draw submission, and double-buffer scanout path.

- Wireframe stability: hardware testing showed some shared edges flickering in line mode. Adjacent triangles rasterize the same edge at effectively identical depth while carrying different per-polygon colors; with LESS_EQUAL both copies can pass and compete. Wireframe now uses strict `SCE_GXM_DEPTH_FUNC_LESS` with depth writes enabled so the first shared-edge fragment establishes depth and equal-depth duplicates are rejected deterministically. Filled mode remains LESS_EQUAL.

- Basic Wing viewer controls: right-stick Y now dollies the perspective camera in/out without changing FOV. Up moves closer, down moves farther away; distance is clamped to 1.4-8.0 units, and Triangle resets yaw, pitch, and camera distance to the default view. Left-stick rotation, L/R fill-wireframe toggle, SELECT console-viewer toggle, and START+SELECT exit remain unchanged.
