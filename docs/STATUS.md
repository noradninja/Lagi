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

- Basic Wing VDP1 metadata capture: the DRAGON0.MCB polygon parser now preserves each polygon's original four indices plus lightingControl, CMDCTRL, CMDPMOD, CMDCOLR, CMDSRCA, and CMDSIZE in a structured SaturnPolygonRecord. Derived inspection helpers expose texture byte address (CMDSRCA << 3), width/height from CMDSIZE, color mode from CMDPMOD, and texture flip from CMDCTRL using the same bit definitions as upstream Azel. The mesh build now requires one preserved record per polygon (212 expected), prints unique texture-descriptor/color-mode/address diagnostics, and reports [PASS] DRAGON0 VDP1 212 RECORDS on the Vita status console. Rendering is intentionally unchanged in this milestone.

- Persistent Vita logging: Lagi now opens `ux0:data/lagi/lagi.log` at platform startup and truncates it for each new run. The logging layer mirrors messages to stdout and the file, flushing after every write so diagnostics survive abnormal exits when possible. Startup/platform messages, COMMON.DAT loader diagnostics, dragon hierarchy/geometry diagnostics, and the new Basic Wing VDP1 descriptor report are persisted. Logging failure is non-fatal so the runtime can still boot with stdout-only diagnostics.

- Persistent logging follow-up: the initial stdio/fopen implementation did not create a file on hardware. The logger now uses Vita-native sceIoMkdir/sceIoOpen/sceIoWrite/sceIoClose, still targeting `ux0:data/lagi/lagi.log`. Startup now shows either `[PASS] LOG ux0:data/lagi/lagi.log` or a `[FAIL] LOG OPEN 0x...` status line on-screen, making filesystem/open failures immediately visible during hardware testing.

- Basic Wing texture-source trace: upstream Azel's `loadDragonFiles()` loads `DRAGON0.MCB` as a file bundle with VDP1 relocation `0x2400`, then loads `DRAGON0.CGB` directly into VDP1 byte address `0x12000`. These are the same base because `0x2400 << 3 == 0x12000`. Azel's bundle loader applies the relocation to each processed model's CMDCOLR and CMDSRCA. Therefore Lagi's preserved pre-relocation CMDSRCA/CMDCOLR values are direct offsets into DRAGON0.CGB when multiplied by 8. Lagi now reads DRAGON0.CGB from the mounted disc and verifies every Basic Wing texture byte range and mode-1 16-entry LUT range lies within the CGB. Hardware status reports `[PASS] DRAGON0 CGB REFERENCES` when all references validate; detailed byte counts/range ends are written to lagi.log.

- Basic Wing mode-1 decoder probe: Lagi now walks each unique Basic Wing VDP1 texture descriptor directly against DRAGON0.CGB using Azel's mode-1 semantics: two 4bpp texels per byte, CMDSRCA<<3 character address, CMDCOLR<<3 16-entry LUT address, SPD/end-code handling from CMDPMOD, and direct Saturn RGB555 expansion to RGBA8888. A real temporary RGBA buffer is produced for every unique texture, although it is not uploaded to GXM yet. The probe logs decoded texture/pixel counts, transparent/end-code pixels, direct RGB555 pixels, and any LUT entries that instead reference VDP2 CRAM. Hardware reports PASS when every unique texture decodes and no CRAM resolution is required; otherwise it reports that CRAM resolution is the remaining dependency. Rendering remains unchanged for this milestone.

- First native textured Basic Wing path: the mode-1 decoder now retains all 71 unique RGBA8888 texture payloads plus a polygon-to-texture mapping. The GXM viewer has a separate textured vertex format (position + UV), preserves Azel's four CMDCTRL flip orientations, uses half-texel UV endpoints, groups the 212 Saturn polygons into one indexed draw batch per unique texture, uploads each DRAGON0 texture as a linear A8B8G8R8 GXM texture, and forces point filtering/no mipmaps. Textured rendering is viewer mode 0; mode 1 retains the proven per-polygon debug colors; mode 2 is wireframe. Camera, depth surface, double-buffered scanout, and the proven color shader path remain intact. Transparent decoded texels are discarded in the first-pass fragment shader. The textured Cg shaders are compiled by psp2cgc and embedded as GXP objects at build time; CMake searches PSP2CGC/SCE_PSP2_SDK_DIR and the current development path E:/PSVITA/sdk/host_tools/bin.

- First textured Basic Wing hardware result: PASS. On real PS Vita hardware, viewer mode 0 renders the Basic Wing with its original DRAGON0.CGB textures and is visually correct. Texture addressing, 4bpp mode-1 decode, 16-entry LUT resolution, RGB555 expansion, per-polygon texture selection, CMDCTRL flip handling, UV corner order, nearest-neighbor sampling, GXM texture upload, grouped textured draws, perspective camera, depth, and double-buffered scanout all work together end-to-end. The existing polygon-color and wireframe debug modes remain available for comparison.

- Basic Wing lighting-data preservation milestone: Lagi now retains the raw per-polygon lighting payloads that were previously skipped after the six VDP1 command words. This mirrors pinned Azel's `processModel.cpp`: mode 0 has no extra data; mode 1 stores one signed 16-bit XYZ normal plus two padding bytes; mode 2 stores four corner records containing signed XYZ normals plus three unsigned 16-bit color words; mode 3 stores four signed XYZ normals. No lighting is applied to the textured GXM output yet. The mesh builder validates payload counts/with-color semantics, logs the Basic Wing mode 0/1/2/3 distribution plus extra/color-record totals, and reports `[PASS] DRAGON0 LIGHTING DATA` on hardware when the preserved records are structurally consistent.

- Gouraud grayscale diagnostic viewer: Basic Wing's preserved mode-3 corner normals are transformed through the same reconstructed bone hierarchy rotation as the model geometry, normalized, and evaluated against a fixed diagnostic light direction. The result is emitted as a separate grayscale color-vertex stream so the proven textured path remains untouched. Viewer modes are now: 0 textured baseline, 1 Gouraud grayscale diagnostic, 2 polygon debug colors, 3 wireframe. This diagnostic intentionally does not yet use PDS's live light vector, light color, or distance-falloff state; it exists to validate normal orientation, corner association, hierarchy transforms, and interpolation on hardware before texture modulation.

- Gouraud diagnostic refinement after hardware video review: the initial one-sided Lambert preview made back-facing wing normals nearly black against the black viewer background, visually shrinking the dragon and obscuring whether the wing issue was a real normal/corner problem. Mode 1 now maps the signed normal/light dot product across a 35%-100% grayscale range. This keeps the complete silhouette visible while retaining normal-direction information; it remains a diagnostic view and does not change the textured baseline or final Saturn lighting implementation.

- Viewer scale investigation: hardware showed a persistent apparent/actual scale mismatch when switching from textured mode 0 to grayscale mode 1. The debug modes previously used an older vendored color GXP binary while textured mode used a freshly compiled psp2cgc shader. To eliminate that unverified binary/toolchain difference, the color vertex/fragment shaders are now compiled from `shaders/lagi_color_v.cg` and `shaders/lagi_color_f.cg` with the exact same psp2cgc/objcopy pipeline and flags as the texture shaders. The old embedded `gxm_color_shaders.cpp` path is no longer linked. This does not intentionally change projection math; it makes the two vertex paths directly comparable on hardware.

- Viewer scale isolation pass: Mode 1 now uses the exact same `texture_v.cg` vertex program, `DebugTextureVertex` stride/attributes, WVP uniform parameter, and XYZ position data as textured Mode 0. Its grayscale intensity is carried in TEXCOORD0.x and a dedicated fragment shader emits grayscale. This removes the color-vertex pipeline entirely from the Mode 0 vs Mode 1 scale comparison. If hardware still changes scale between those two modes, the remaining suspect is camera/input state rather than vertex projection or stream layout.


- Viewer scale isolation hardware result: PASS. After moving the grayscale diagnostic onto the same `texture_v.cg` vertex path and `DebugTextureVertex` layout as the textured baseline, switching between modes no longer changes model scale. The previous scale mismatch was therefore isolated to the separate color-vertex path rather than camera state or the Basic Wing geometry.

- Combined textured-lighting milestone: a new native GXM path now carries position, UV, and one interpolated lighting scalar per vertex through `DebugTexturedLitVertex`. The fragment shader samples the already hardware-proven DRAGON0 texture, preserves alpha discard, and multiplies sampled RGB by the interpolated lighting value. The lighting value currently comes from the same fixed-light, preserved mode-3 normal diagnostic used by the grayscale view; this is intentionally a validation step before wiring PDS's live scene light vector/color/falloff. Viewer modes are now: 0 textured baseline, 1 textured + diagnostic Gouraud lighting, 2 Gouraud grayscale diagnostic, 3 polygon debug colors, 4 wireframe. Modes 0 and 1 share the same camera/WVP geometry and texture batching, so the hardware comparison should isolate lighting modulation itself.


- Wing-lighting winding/culling diagnostic: hardware video of the combined textured-lighting mode showed triangle-shaped lighting inversions concentrated on the thin wing membranes. Pinned Azel uses the same 0-1-2 / 0-2-3 quad split and maps lighting extra-data corner j to geometry corner j, so the current split/corner numbering matches upstream. One material difference is that Azel's object path enables back-face culling while Lagi's viewer had been rendering both sides with SCE_GXM_CULL_NONE. A new diagnostic viewer mode renders the same textured-lit stream with CCW culling enabled. Mode order is now: 0 textured baseline, 1 textured + lighting two-sided, 2 textured + lighting culled, 3 Gouraud grayscale, 4 polygon colors, 5 wireframe. The next hardware comparison is mode 1 vs mode 2 while rotating across the wing surfaces.

- Vita presentation assets: Lagi now packages a 128x128 indexed PNG app icon plus an 840x500 LiveArea background and 280x158 startup/gate image derived from the supplied Lagi artwork. The VPK includes these under `sce_sys/` with the matching LiveArea template. The assets were quantized/pixel-reduced intentionally to keep the Vita PNG payloads compact while preserving the supplied composition and low-resolution aesthetic.


- Saturn quad lighting reconstruction: the textured lighting path now follows Azel's VDP1 strategy instead of interpolating one scalar light value independently across each generated GXM triangle. Each original Saturn quad retains four RGB Gouraud corner offsets. The renderer projects the four original corners into screen space and the fragment shader reconstructs quad-local (s,t) with the same inverse-bilinear formulation used by Azel, then bilinearly evaluates corner 0-1 / 3-2 lighting across the complete quad. This removes the generated 0-1-2 / 0-2-3 diagonal from the lighting interpolation itself.

- RGB555 Gouraud quantization: the viewer now uses Azel's Saturn-accurate representation for the Gouraud payload: clamp to 0..31, treat 16 as neutral, and convert each 5-bit channel to signed additive modulation with `(gouraud5 - 16) / 31`. The fragment shader applies this as `clamp(texture.rgb + gouraudRGB, 0, 1)`, matching Azel's additive VDP1 shading model instead of the earlier modern multiplicative-light approximation. The standalone viewer still uses a fixed white diagnostic light and provisional fixed falloff/ambient floor because live PDS scene light color and distance-falloff state are not wired into Lagi yet; the 5-bit quantization and additive application are now correct.


- Wing winding follow-up: hardware video suggests the previous CCW culling diagnostic was preserving the opposite side of the thin wing membranes. The culled textured-lighting diagnostic now uses `SCE_GXM_CULL_CW` instead. This is intentionally a hardware validation step: Azel's renderer uses CCW culling in its own projection conventions, but Lagi's row-vector WVP / GXM viewport path may invert the effective front-face winding. Compare the two-sided lit mode against the CW-culled mode on the wing flats before making culling permanent.


- CW culling hardware pass: confirmed on PS Vita hardware that `SCE_GXM_CULL_CW` preserves the intended visible side of the Basic Wing's thin wing membranes under Lagi's current row-vector WVP / GXM viewport conventions. The temporary two-sided vs culled diagnostic pair has been removed; textured + RGB555 Gouraud lighting now uses CW culling permanently. One isolated stray triangle remains visible and is being left as a separate mesh/model-data issue for later investigation rather than changing the now-correct lighting/culling path.


- Camera-relative Azel light test: the standalone Basic Wing viewer now mirrors the actual dragon morph viewer defaults from pinned Azel: `setupLight(0, 0, 0x10000, 0x161918)` plus `generateLightFalloffMap(0x030102, 0, 0)`. Azel's exact 32-entry quadratic falloff generator is reproduced locally. Because Lagi's standalone viewer orbits the model rather than running a live PDS camera task, the posed model-space normals are retained and transformed into view space every frame; the test light therefore remains fixed relative to the camera while its direction changes across the model as the user rotates it. Per-corner lighting is recomputed every frame, quantized to 5-bit RGB, and fed into the existing quad-bilinear fragment path.

- RGB555 grayscale diagnostic: viewer mode 2 now consumes the exact same dynamically recomputed RGB555 Gouraud corner values and inverse-bilinear quad reconstruction as the textured lit mode. Instead of the old smooth signed-dot visualization, it converts the reconstructed signed Gouraud offsets back to a 0..31 intensity and displays their RGB average as grayscale. CW culling is applied here too so the diagnostic shows the same intended Saturn faces as the lit textured path.


- Viewer culling consistency: hardware established CW as the correct effective front-face winding for Lagi's current GXM projection path. All solid Basic Wing viewer modes now use `SCE_GXM_CULL_CW`: textured baseline, textured RGB555 Gouraud, grayscale RGB555 diagnostic, and polygon debug colors. Wireframe intentionally remains `SCE_GXM_CULL_NONE` so hidden/back-facing edges remain visible for geometry debugging. This makes mode changes compare shading/material state without also changing which solid faces are visible.


- Final RGB555 output quantization: the textured RGB555 Gouraud path now performs its final VDP1-style color calculation explicitly in 5-bit channel space. The decoded texture is converted back to integer-like 0..31 RGB, the bilinearly interpolated signed Gouraud offset is added in that domain, the result is clamped to 0..31, then rounded to a discrete 5-bit value before expanding to the Vita's RGBA8888 render target. This preserves the original quad-wide Gouraud gradient while restoring the visible Saturn lighting/color banding that was previously lost when the interpolated result remained floating point through final output.

- The grayscale Gouraud diagnostic now applies the same post-interpolation 5-bit quantization before converting the RGB result to gray, making the 32-level stepping directly visible and suitable for verifying the Saturn-style banding independently of texture color.


- Basic Wing morph-screen animation: the viewer now decodes the Basic Wing animation referenced by pinned Azel's `dragonAnimOffsets[0]` (`DRAGON0.MCB` table entry `0x10C`), which is the animation selected by `dragonMenuDragonInit()` on the morph screen. Lagi parses the native animation header and per-bone track data, reconstructs Azel's rotation update modes (0/1/3/4/5), rebuilds the 31-bone hierarchy for each decoded frame, and retains animated vertex positions plus transformed lighting normals. The viewer advances this flap loop at 30 Hz against the Vita's 60 Hz scanout, so textures, CW culling, camera-relative RGB555 lighting, and the grayscale lighting diagnostic all follow the animated geometry.

- Viewer detail zoom: right-stick dolly minimum distance has been reduced from `1.4` to `0.75`, allowing substantially closer inspection of RGB555 texture and lighting bands while retaining the same 50-degree perspective projection and reset distance of `3.0`.


- Fixed-step morph animation timing: Basic Wing flap playback is no longer derived from rendered frame count. The viewer now uses Vita process time as a monotonic microsecond source and advances the predecoded PDS animation on an exact 30 Hz phase accumulator. At 60 FPS a pose normally persists across two renders; at 30 FPS it advances once per render; under a temporary render stall it jumps directly to the animation frame corresponding to elapsed 30 Hz ticks. This removes the previous half-speed animation behavior when close zoom increases rendering cost.


- Fixed 30 Hz viewer presentation: all Basic Wing viewer modes now present on a 30 Hz cadence regardless of shader cost. The Vita still scans out at 60 Hz, but Lagi waits until at least one vblank has elapsed since the previous presentation before queuing the completed backbuffer for `SCE_DISPLAY_SETBUF_NEXTFRAME`; it therefore becomes front on the following vblank. Lightweight modes no longer run/camera-update at 60 FPS, while heavier lit modes do not receive an unconditional extra two-vblank delay if rendering already consumed one of the required intervals. The independent 30 Hz animation clock remains unchanged.


## End-of-session graphics milestone — 2026-09-28

The Basic Wing viewer has reached a strong Saturn-faithful graphics baseline on real PS Vita hardware.

Implemented and hardware-validated:
- Native SceGxm rendering at 960x544 with double-buffered scanout.
- Perspective camera with yaw/pitch and close-range dolly inspection.
- Original Saturn VDP1 texture data decoded directly from DRAGON0.CGB, including mode-1 LUT handling, texture flips, alpha/end-code behavior, and nearest-neighbor sampling.
- Hardware-confirmed CW front-face culling for all solid viewer modes; wireframe remains two-sided for diagnostics.
- Pinned Azel/PDS lighting behavior reconstructed around per-corner Saturn normals and RGB555 Gouraud data.
- Original Saturn quad identity retained: two triangles are still submitted, but Gouraud interpolation is reconstructed bilinearly across the original four-corner quad to avoid a modern triangle-diagonal lighting seam.
- Gouraud values are represented as Azel's signed 5-bit additive RGB offsets, not modern multiplicative lighting.
- Final lit color arithmetic is clamped and quantized back onto the Saturn's 0..31 RGB555 channel grid before expansion to the Vita RGBA8888 target. This restores the characteristic visible lighting/color banding instead of producing a smooth 8-bit gradient.
- Camera-relative morph-screen light behavior is active, with the pinned Azel dragon-viewer light defaults and falloff reconstruction feeding the RGB555 Gouraud path.
- Matching grayscale RGB555 lighting diagnostic, polygon-color diagnostic, and wireframe view remain available.
- The Basic Wing morph-screen flap animation is decoded from DRAGON0.MCB animation table entry 0x10C, matching the default animation selected by pinned Azel's dragon morph screen.
- Animated hierarchy poses update both geometry and transformed lighting normals.
- Morph animation runs from an independent exact 30 Hz clock, so rendering load no longer changes playback speed.
- All viewer modes are presented at a fixed 30 Hz cadence to match the PDS-era update/render expectation and keep camera/input speed consistent across shaded and unshaded modes.

Current visual interpretation:
Lagi is no longer simply displaying PDS assets through a modern renderer. The Vita GPU is being used to reproduce the Saturn VDP1 rules that materially define the game's look: original texture data, quad semantics, 5-bit Gouraud arithmetic, RGB555 output limitations, face visibility, and 30 Hz presentation. The hardware implementation is modern, but the visible constraints are deliberately Saturn-like.

Known issue intentionally deferred:
- One isolated stray triangle remains visible in the Basic Wing model. Because culling, texture mapping, and the rest of the mesh are now stable, this is being treated as a likely model/reconstruction-data issue rather than a renderer-wide winding problem.

Next session:
- Move beyond the standalone graphics viewer and begin reconnecting this proven graphics path to live game rendering/state.
