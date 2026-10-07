# Flight Mode Bring-Up

Lagi's first flight milestone begins immediately after the Ruins elevator movie sequence. Azel advances game status 5 to status `0x50`, which maps to game mode 3 / field index 1 and loads `FLD_A3.PRG` ("above excavation").

The implementation follows the same ownership rule used for the Ruins runtime:

```text
Azel decides.
Lagi services.
Neptune renders.
```

Flight behavior, field scripts, dragon movement, camera state, visibility, animation, encounters, VDP1/VDP2 state, and progression remain Azel-owned. Lagi restores the Vita-facing services and presentation paths required to let that runtime execute natively.

## Current hardware-validated state (2026-10-07)

The native FLD_A3 route is live on Vita hardware:

```text
Ruins elevator
    → Azel status 0x50
    → mode 3
    → FLD_A3.PRG
    → native field task graph
```

BGM, dragon/rider, radar, LCS, and field geometry are active. Field visibility remains Azel-driven: earlier diagnostics identified a 4x14 FLD_A3 visibility grid, changing camera cells, roughly four active cells at a time, and many grid objects passing Azel's visibility and clipping tests.

Hardware has also established two presentation rules that must not regress:

- Town/Ruins retains the historical Saturn-to-GXM X mirror. Native field mode 3 does **not** receive that extra X mirror and does **not** apply an extra winding XOR. Commit `1e3ec6464394a75b50f10a6185d555aeac20afae` (`Use native field horizontal orientation`) is the known-correct field orientation baseline.
- The upper-right field radar map is a 48x48 point-filtered sprite drawn at its original 48x48 output-pixel size. Its alternating mesh/checker pattern provides Saturn-style pseudo-transparency over polygons. Bilinear filtering destroys that effect. Keep the map sprite rule separate from the radar frame and other UI behavior. This behavior was established in `e467858e48b97ddccb94d40270822e9c4cbf33f6`.

Vita/Vita TV results supplied by the user are authoritative. Host builds, static checks, or log review do not constitute hardware validation.

## Current handoff - resume here

- **Date:** 2026-10-07
- **Branch:** `feature/flight-mode-bringup`
- **Latest hardware-tested rendering checkpoint:** `a476878a9d0c44b30c4e3a6508273d70c86c2482` (`Reuse static model material bindings`)
- **Draft pull request:** #12 (`Bring up native FLD_A3 flight entry`)

This is the active rendering checkpoint. Neptune is the native SceGxm renderer, not VitaGL. The gameplay panel is 480x272 and the immediate target is reliable 30 Hz within a 33.3 ms frame budget. Do not merge PR #12 until the user explicitly approves it after Vita/Vita TV testing.

The normal route from the Ruins elevator through status `0x50`, mode 3, `FLD_A3.PRG`, `overlayStart_FLD_A3()`, and `initField()` is hardware-validated. The authentic Azel field task graph, player-controlled flight, BGM/SFX, dragon/rider, geometry, visibility/grid/LOD, radar, LCS, and UI are live. Preserve these hardware-confirmed results:

- Field orientation and winding are correct: mode 3 does not use the town Saturn-to-GXM X mirror or an extra winding XOR.
- The FLD_A3 radar map remains point-filtered at its original 48x48 output-pixel size.
- Field lighting uses the native 32.0 (`0x200000` raw) far clip, the upstream reversed light-color storage (`R=[2]`, `G=[1]`, `B=[0]`), and model-space normals with the light rotated back once per submission. Do not add brightness or gamma compensation.
- The exact midpoint-grid Full Gouraud optimization and duplicate `A,B,C / A,C,D` vertex-transform reuse are hardware-valid checkpoints and must remain intact.
- The billboard invalidation correction is working: billboards remain dynamic and no longer force the static prefix to rebuild.

At the latest hardware-tested checkpoint, the static-grid hook is active and the material-binding reuse change has removed the previous ~110-128 ms recurring material-resolution penalty. Across 39 sampled active-field `[ScenePerf]` records, render median is 22.855 ms. The 34 non-rebuild samples have a 21.259 ms median, 25.605 ms p90, and 26.206 ms maximum. Dense 900+ polygon non-rebuild samples have a 23.576 ms median. All five sampled frames over 33.3 ms are static-prefix rebuild frames; their render median is 44.244 ms and maximum is 46.924 ms. First-use texture growth remains a separate hundreds-of-milliseconds residency stall.

### Current correction: preserve cached Saturn mesh-command metadata

The latest full-mode hardware run passed the complete static-grid bridge gate: every sampled active `[FieldStream]` record had matching nonzero `staticCtx=set/consumed`, `staticSubs > 0`, and no context mismatch. The generated FLD_A3 hook, bridge consumption, world-space classification, and Neptune static path are therefore all active. Do not return to the earlier generated-source activation diagnosis unless a later log actually reports `staticCtx=0/0` or `staticSubs=0`.

Hardware exposed one visual regression at map entry: while the scripted camera starts above the dragon and travels behind it, a section of field mesh that begins offscreen does not appear as the camera reveals it; the section appears only after player movement. Source inspection rules out a stale camera visibility-preparation cache: `prepareLiveTownGouraudVisibility()` is rebuilt from the current WVP every authentic-camera frame.

The concrete renderer bug is in the metadata paired with the cached static prefix. `g_liveTownMeshRanges` records source polygons carrying Saturn VDP1 mesh mode (`CMDPMOD` bit 8), which Neptune submits with ordered-overdraw behavior. The frame builder cleared those ranges every frame. A static-cache rebuild recreated them, but a cache hit restored only the cached vertices, polygon records, materials, and lighting arrays. It did not restore the static mesh-command ranges. A later Azel visible-set change—commonly triggered when the player starts moving—rebuilt the prefix and recreated the missing metadata, matching the observed delayed appearance.

The next checkpoint preserves a separate copy of the static mesh-command ranges when the static prefix is rebuilt and restores it on cache hits before dynamic submissions append their ranges. CRAM and VDP1 texture invalidation clear the paired metadata. `[FieldStream]` now reports `meshRanges=total/static`, allowing the next hardware run to prove that static ranges remain present on `staticRebuilt=0` frames. This is renderer-owned presentation metadata; Azel's visibility, clipping, LOD, model choice, and scripted camera remain untouched.

### Immediate next task

1. Build the new checkpoint and test the complete opening scripted camera without moving the player.
2. Confirm the previously absent mesh section appears as soon as the camera reveals it.
3. In `[FieldStream]`, confirm `meshRanges=total/static` retains a nonzero static count on `staticRebuilt=0` frames. Continue to require matching `staticCtx`, `staticSubs > 0`, and nonzero billboards.
4. Confirm field orientation/winding, corrected lighting, dragon/world relationship, and the point-filtered 48x48 radar remain unchanged.
5. If the visual gate passes, resume the generic persistent-geometry/resource work. The remaining sampled deadline misses are all rebuild events; do not optimize audio or the dynamic actor path for this milestone.

At `a476878`, 247 of 296 logged field records still rebuilt the monolithic static prefix. Ordinary build median is 6.680 ms and object append median is 5.062 ms. Rebuild build median has fallen to 26.786 ms because recurring material resolution is now cached; geometry upload remains about 7.928 ms median. That is a major improvement over the prior ~128 ms rebuild median, but rebuild frames still render around 44-47 ms and miss 30 Hz.

With classification now working, the remaining architectural milestone is still a generic Neptune resident model/resource cache shared by fields and towns, including Zoah: first encounter adapts/decodes/uploads a stable Azel model or cell resource once, and later Azel visibility/LOD decisions select resident resources without rebuilding a monolithic visible-world mesh. Do not create an FLD_A3-only renderer architecture.

Keep audio work on PR #11, the end-of-area crash, and native ray/laser rendering out of this checkpoint. The usual home build environment is `E:\dev\Lagi`, VitaSDK at `E:\dev\VitaSDK`, PSP2CGC commonly at `E:\PSVITA\sdk\host_tools\bin\psp2cgc.exe`, with `cmake --build build --parallel 32` producing `build\Lagi.vpk`.

## Rendering performance and streaming investigation

Traversal hitches correlate with changes in the visible field geometry set. The branch currently records:

- `[FieldCellTransition]`: camera-cell and active-cell-count changes.
- `[FieldStream]`: frame, submissions, polygon/vertex counts, model-cache misses, material misses, texture-count growth, whether `prepare_vdp1_model()` ran, GPU texture count/dirty state, and append/material/upload/build timings.
- Per-published-frame model-cache misses exposed through the render bridge.

The first diagnostic run showed that Neptune represented the live field as one changing flattened mesh. When its visible geometry size changed, Neptune called `prepare_vdp1_model()` again. Before texture preservation, that function released and re-uploaded the entire resident texture set, producing roughly 400-470 ms uploads even when the frame introduced no new model or texture.

The earlier texture-reuse checkpoint, `8a531f88a8d59381c2d0daa8fad1e5c1f7ae6a19` (`Reuse field textures across geometry changes`), added texture-preserving geometry rebuilds through:

```cpp
prepare_vdp1_model(const Vdp1ModelSource&, bool preserveResidentTextures = false)
releaseResidentVdp1Model(bool preserveTextures = false)
```

The field path requests preservation when rebuilding live geometry. CRAM and VDP1 texture invalidation still mark texture data dirty or free textures when required. Hardware traversal is noticeably smoother after this change, so the texture-residency optimization is qualitatively validated.

It is not the final streaming design. The latest hardware log still contained about 279 geometry reprepares. Ordinary non-prepare field builds had a median around 15.8-15.9 ms; prepare-event builds had a median around 95.5 ms, including roughly 81.7 ms of upload work. Those events remain visible hitches.

Earlier pre-optimization workload timings, retained as a baseline:

| Workload | Median | 90th percentile | Relevant budget / interpretation |
|---|---:|---:|---|
| Neptune render | ~44.5 ms/frame | ~123.5 ms | Above the 33.3 ms 30 Hz budget; current primary FPS limiter |
| Field geometry build | ~15.9 ms/frame | ~81.5 ms | High tail tracks reprepare/resource work |
| Audio worker, field BGM | ~6.92 ms/256 samples | ~8.01 ms | 5.804 ms deadline; ~83.5% of sampled chunks exceeded it |
| SCSP processing | ~6.41 ms | — | Main audio cost |
| Slot processing | ~3.09 ms | — | Part of SCSP work |
| DSP | ~3.15 ms | — | Part of SCSP work |
| 68K | ~1.87 ms | — | Relatively stable |

Audio runs on CPU2, so its chunk cost must not be added arithmetically to render frame time. It is under real-time pressure, but render work independently exceeds the 30 Hz frame budget. Rendering and geometry-chunk hitching are the priority for this milestone; audio optimization stays on its separate branch/PR.

## Neptune residency direction

The remaining hitch is a generic Neptune resource-residency problem, not a field-only special case. A monolithic flattened scene will reproduce the same failure mode in large towns such as Zoah and potentially in other scene types.

The intended architecture is:

```text
Azel stable model / cell identity
        ↓ first encounter
Lagi adapts and decodes presentation data
        ↓
Neptune creates persistent GPU geometry/material/texture resource
        ↓
Azel visibility changes
        ↓
submit a different set of resident resources
        ↓
no full-scene rebuild or re-upload
```

The cache should be generic enough for town and field presentation. Static environment geometry should be adapted, decoded, and uploaded once, then selected for drawing according to Azel's visibility/cell decisions. Dynamic actors—dragon, rider, enemies, particles, projectiles, and other changing geometry—remain dynamic. Azel continues to own scene identity, visibility, clipping, and gameplay decisions; Lagi only bridges stable identities and data; Neptune owns native SceGxm resources and rendering.

Mode 3 still begins with `begin_frame(forceDynamicSubmissions=true)` for ordinary task-owned submissions, but the Phase 4 bridge now captures Azel's native field view, removes it from submitted transforms, and reapplies it exactly once in Neptune. The generated FLD_A3 environment-grid hook explicitly publishes rigid cells with `dynamic=false`; dragon/rider, billboards, effects, and other task-owned objects remain dynamic. Hardware logs at `a476878` prove the static hook is active through matching `staticCtx` counts and nonzero `staticSubs`.

The existing field camera path is:

```text
field camera task
    → camera slot
    → applyCameraStatusToEngine()
    → updateEngineCamera(...)
    → cameraProperties2 / pCurrentMatrix / m384_viewMatrix
```

Upstream helpers include `getFieldCameraStatus()` and `getFieldCameraMatrix()`. Town's `begin_view_relative_submission_scope()` and `removeViewTransform()` remain the equivalent scoped mechanism for town submissions. Do not blanket-classify task-owned actors or effects as static merely because their matrices can now be expressed in world space.

### Paged resident geometry design

Do not replace the current monolithic visible mesh with one ever-growing whole-map mesh. The existing Full path uses 16-bit GXM indices: base geometry reaches the limit at 65,535 vertices, and the 3x3 subdivision path reaches it at 7,281 source quads. A single FLD_A3/Zoah superset buffer would therefore exchange recurring rebuilds for a hard capacity ceiling.

The next resource layer should use bounded geometry pages shared by fields and towns:

1. A stable Azel static-instance key selects a Neptune resource record. For FLD_A3 the generated adapter can publish the existing `s_visdibilityCellTask::m14_index` as the cell identity and the `s_grid1::EA` Saturn address (or an explicit ordinal derived in the same draw loop) as the object identity, together with the chosen model offset and world matrix. This requires only generated-source/bridge integration; do not edit `extern/Azel`.
2. On first encounter, Lagi adapts the model and Neptune appends its immutable world-space positions, material indices, subdivision UVs, and topology into a mapped page whose vertex/index counts remain within 16-bit limits. Texture resources remain globally shared by decoded material identity.
3. Each published Azel frame marks exactly which resident instance records are active. Neptune performs its conservative per-quad hardware clip and current-light update only for those active ranges, then rebuilds small per-page/per-texture index streams. Visibility-set changes must not call `prepare_vdp1_model()` or rewrite resident positions/UV/topology.
4. Pages, not individual models, are the texture-batching unit. This retains the existing ascending-texture draw order and avoids multiplying draw calls by every small field object. Saturn mesh-mode ranges remain explicit ordered-overdraw phases inside their owning page.
5. Dynamic submissions keep the current frame-owned path until separately classified. They must not invalidate resident static pages.

The first hardware-testable residency checkpoint should cover only rigid environment submissions and report resident hits, misses, page growth, active resources/polygons, and bytes written. Its acceptance gate is zero geometry reprepares on visibility-only changes after first encounter, unchanged presentation, and rebuild-frame render time below 33.3 ms. First-use page growth and texture decoding should remain separately visible in the log; prewarming is a later checkpoint rather than being hidden in the visibility path.

The identity-discovery precursor now publishes real FLD_A3 static keys without changing draw behavior. The generated adapter supplies `m14_index` as `cellIndex` and the selected `s_grid1::EA` Saturn address as `objectIndex`; the exact-count CMake sentinel verifies that this generated-source hook appears once. Neptune's static signature now includes native model pointer, bundle/cell/object identity, model offset, and world matrix, closing the earlier model-offset-only alias risk. A diagnostic identity set reports `staticId=hits/misses/known` in `[FieldStream]`: `hits` and `misses` are for the current published frame, while `known` is the cumulative number of stable static instances encountered since resource invalidation. This checkpoint does not yet allocate resident geometry or claim a performance improvement. Hardware should show misses converging toward zero when revisiting already-seen cells; unstable or continuously growing identities must be fixed before they become GPU resource keys.

One bounded Full-mode rebuild optimization can run independently of the later page architecture. When a changing visible set fits the existing mapped geometry capacity, Full mode renders exclusively from the 3x3 subdivision vertex/index buffers. The prepare path nevertheless regenerated the separate base six-vertex textured/Gouraud UV payload and texture-batched index stream, even though neither is consumed by that frame. Capacity-reuse prepares now keep those allocations but defer their payload rebuild, reporting `skipBase=1` and `texBuild=0`. Fresh allocations still initialize the base payload, and switching to a non-Full diagnostic mode lazily forces a normal prepare before that payload can be consumed. Subdivision UV/topology generation, lighting, visibility, materials, texture order, and draw submission are unchanged. This should remove roughly the former recurring `texBuild` bucket from rebuild frames; hardware validation is pending and the persistent page design is still required to eliminate the remaining upload/reprepare work.

The detailed prepare instrumentation below was used to split the former ~80-100 ms reprepare events into allocation, topology/UV, subdivision, copy, texture, and CPU flatten/transform work. At `a476878`, recurring material lookup is no longer the dominant rebuild cost: rebuild build median is 26.786 ms and geometry upload median is 7.928 ms. The instrumentation remains useful for first-use growth and for proving that the paged-residency path removes rather than merely shortens visibility-driven reprepares.

The current instrumentation checkpoint extends prepare-event `[FieldStream]` records with the following microsecond buckets. It does not change rendering or resource lifetime:

| Field | Work measured |
|---|---|
| `reuseTex` | Whether the existing resident texture set was reused |
| `reuseGeom` | Whether all mapped geometry buffers had sufficient capacity and were retained |
| `release` | Geometry-buffer unmap/free work in `releaseResidentVdp1Model()` |
| `baseAlloc` | Base color/lighting/index buffer allocation and GXM mapping |
| `wire` | Wire-subdivision buffer allocation/mapping and linear-index initialization |
| `texUpload` | Texture allocation, copy, GXM texture initialization, and filtering; expected to remain zero when `reuseTex=1` |
| `texAlloc` | Textured/gouraud vertex and texture-index buffer allocation/mapping |
| `texBuild` | Base textured UV generation and per-texture index-batch rebuild |
| `subAlloc` | Filled subdivision vertex/index buffer allocation and mapping |
| `subBuild` | Filled subdivision positions, UVs, and immutable topology rebuild |
| `copy` | Base color/lighting vertex copies and linear base-index initialization |

The existing `append` field continues to cover per-frame CPU flatten/transform work. `upload` remains the enclosing prepare/update interval so the sum of the detailed buckets can be compared with unclassified overhead. These measurements require a new Vita/Vita TV traversal log before choosing the next optimization.

The first capacity-reuse optimization now keeps the flattened field geometry buffers mapped when the next Azel-visible set fits their existing capacities. In that case `reuseGeom=1`, `release=0`, `baseAlloc=0`, `wire=0`, `texAlloc=0`, and `subAlloc=0` (apart from timer noise), while topology/UV data and current vertices are still rebuilt exactly as before. Texture invalidation remains independent: CRAM or VDP1 changes may produce `reuseGeom=1` with `reuseTex=0` and a nonzero `texUpload`. This is a bounded hitch reduction and measurement step, not a replacement for persistent per-model/per-cell resources. Hardware validation is pending.

**Build validation (2026-10-07): PASS at `ee67463c0c390abd916a9bad155d5e18a11af2ef`.** A clean Vita CMake/Ninja build compiled all 327 steps, linked `lagi.velf`/`lagi.self`, and produced `Lagi.vpk`. No warning originated in the capacity-reuse code. This proves compile/link/package integration only; rendering correctness and performance remain pending Vita/Vita TV validation.

**Full-mode hardware result (2026-10-07): capacity reuse works, overall target not met.** Across 2,438 `[FieldStream]` frames, 269 rebuilt geometry and 2,169 did not. Of the prepare events, 262 reused mapped geometry and only seven exceeded the existing capacity. For `reuseGeom=1`, release and all geometry allocation buckets were effectively zero. This confirms the mapped-capacity change removed the intended allocation churn.

The same run localized the next bottleneck:

| Full-mode field workload | Median | P90 | Maximum |
|---|---:|---:|---:|
| Render (`[ScenePerf]`, field samples) | 54.882 ms | 68.067 ms | 125.924 ms |
| Build, all `[FieldStream]` frames | 15.396 ms | 60.536 ms | 606.704 ms |
| Build, no prepare | 14.993 ms | 18.125 ms | 22.067 ms |
| Build, prepare | 76.055 ms | 89.698 ms | 606.704 ms |
| Upload/prepare interval, `reuseGeom=1` | 61.683 ms | 71.964 ms | 444.443 ms |
| Base textured UV + texture-batch rebuild | 46.522 ms | 54.085 ms | 60.441 ms |
| Filled subdivision rebuild | 14.393 ms | 16.496 ms | 19.006 ms |

Ten prepare events also uploaded newly decoded textures; the largest first-growth event decoded 227 textures and spent about 305 ms in texture upload. Those first-use uploads require persistent model/material residency in the final design, but they are distinct from the recurring prepare cost.

The recurring `texBuild` cost came primarily from an avoidable quadratic loop: for every resident texture, `buildVdp1TexturedBuffers()` rescanned every polygon to collect matching indices. With roughly 900 textures and 1,000 polygons in the hardware run, visibility changes performed around 900,000 comparisons before writing the batches. The next checkpoint replaces that loop with three linear passes: count indices per texture, prefix batch offsets, then emit polygons in original order. Draw grouping and within-texture polygon order remain identical. Hardware validation of that change is pending.

The run also emitted 2,438 `[FieldStream]` lines because the diagnostic's `build >= 10 ms` condition matched nearly every ordinary full-mode frame. Internal `[FieldStream] build` had a 14.993 ms no-prepare median, while the enclosing `[ScenePerf] build` median for ordinary sampled frames was 24.982 ms. The roughly 10 ms gap includes the synchronous log write performed after the internal timer, so per-frame diagnostics were materially perturbing the render thread. The instrumentation now records every prepare, model/material miss, and texture-growth event, but samples otherwise ordinary frames only once per 120 field frames. This preserves transition evidence without continuously taxing the measured path.

The next profiling checkpoint expands the once-per-60-frame `[ScenePerf]` sample without changing presentation. `gourPrep` records the conservative quad projection/visibility pass; `gourPayload`, `gourBucket`, `gourIndex`, and `gourDraw` expose the existing full-mode subdivided Gouraud work; `clear` covers the CPU color/depth/stencil buffer clears; `compose` covers Azel-owned VDP2, text, VDP1 UI, and fade submission; and `cpuprep` retains the enclosing pre-wait CPU total with main scene submission removed. These buckets are intended to resolve the roughly 14.8 ms median that remained outside build, lighting, main submission, and GXM wait in the full-mode hardware log. Hardware results for this checkpoint are pending.

Full-mode subdivided Gouraud submission also repeated a resident-texture-sized scan for every ordered world/mesh phase, even when a small Saturn mesh range referenced only one or two textures. The submission path now records only the texture buckets touched by each phase, sorts that sparse set to preserve the previous ascending texture draw order, and builds/draws only those buckets. It also removes an earlier whole-frame bucket count that was immediately discarded by the per-phase pass. Azel's polygon visibility, mesh range boundaries, ordered-overdraw phases, and within-texture polygon order are unchanged. Hardware validation is pending.

The per-frame flattened append path now pre-sizes its six parallel geometry/material/light arrays and writes them by index instead of growing each vector per vertex or polygon. It constructs the submission-wide light payload once and identifies contiguous Saturn mesh ranges during the existing polygon copy rather than rescanning the model afterward. Vertex transform arithmetic, flattened ordering, material resolution, and mesh phase boundaries are unchanged. This remains an interim reduction to the measured ~12.2 ms ordinary append cost; persistent world-space resources are still the architectural destination. Hardware validation is pending.

Lighting transform ownership now follows Azel's native formulation more closely. Instead of rotating every polygon normal into camera space while flattening, Neptune transpose-multiplies the captured light vector by the submission rotation once and leaves model normals in their adapted model space. For the rigid transforms used by the field path this produces the same dot product, removes the per-corner matrix/clamp/round loop, and matches upstream `3dEngine_flush.cpp`'s model-space light calculation. Billboard submissions use their renderer-resolved billboard basis for the same inverse-rotation step. Visual and performance validation on Vita hardware are pending.

**Expanded Full-mode hardware profile (2026-10-07): target still not met.** In 36 field `[ScenePerf]` samples, render median was 45.423 ms, p90 66.423 ms, and the maximum was 424.819 ms. Restricting the ordinary-frame comparison to the 30 samples with upload below 10 ms gives render median 44.604 ms, p90 48.979 ms, and maximum 53.413 ms. This confirms the per-frame `[FieldStream]` logging throttle removed roughly 10 ms from the earlier 54.565 ms ordinary median, but render remains well above both the 25 ms median goal and 33.3 ms frame deadline.

| Ordinary Full-mode bucket | Median | p90 |
|---|---:|---:|
| Flatten/build | 15.371 ms | 18.576 ms |
| Object append within build | 12.546 ms | 15.027 ms |
| Lighting | 3.923 ms | 4.618 ms |
| Gouraud visibility preparation | 3.554 ms | 4.038 ms |
| Main submission | 8.777 ms | 10.017 ms |
| └ Gouraud payload | 6.728 ms | 7.970 ms |
| └ Gouraud index/draw phase | 1.847 ms | 1.995 ms |
| CPU color/depth/stencil clear | 2.011 ms | 2.138 ms |
| VDP2/UI composition | 0.731 ms | 0.765 ms |
| Final `sceGxmEndScene`/`sceGxmFinish` wait | 1.813 ms | 1.927 ms |

The outer render total contains a per-sample median residual of 8.782 ms after subtracting build, lighting, Gouraud visibility, clears, main submission, composition, and the final GXM wait, but this is not yet evidence of an 8.8 ms GXM cost. Every one of the 36 field `[ScenePerf]` samples was phase-aligned with and immediately preceded by a synchronous `[PresentationTrace][NeptuneScene]` write inside the measured interval. That trace now emits only on an Azel scene-mode transition and does so after the render timer closes. The next checkpoint also records `sceGxmBeginScene()` explicitly as `begin`, so any true residual can be separated from logging overhead.

The same capture contained 2,066 `[PresentationTrace][AzelVDP1]` records in 3,864 total log lines. Its animated command hash changed nearly every frame, defeating the intended 120-frame heartbeat. Hash changes no longer trigger a write by themselves: the trace now records mode/status transitions plus a periodic 120-frame structural sample. Azel's VDP1 command publication is unchanged. The later pre-sized append and per-submission model-space-light changes were also not yet validated by this capture and require a new hardware run.

The linear texture-batch construction is hardware-validated by this capture. Across 275 logged prepare events, prepare/build median fell from the earlier ~76.1 ms to 33.383 ms. `texBuild` fell from ~46.5 ms to 4.936 ms median, while total prepare upload was 19.857 ms median. The dominant recurring prepare substage is now subdivision-buffer construction at 14.204 ms median (16.544 ms p90). Geometry-set changes still occurred frequently, so this improvement reduces but does not eliminate traversal hitches; persistent per-model/per-cell geometry remains required.

The next subdivision checkpoint removes duplicated work specifically from mapped-capacity reuse. A reused field buffer previously rebuilt all 3x3 subdivided positions, zero shades, UVs, and generic 24-index quad topology during prepare; Full mode then rewrote all dynamic positions and visible shades again during submission in the same frame. Reuse prepares now refresh only material-dependent UVs and initialize topology entries beyond the previous polygon count. Fresh allocations retain complete position/shade/topology initialization, and the Full-mode submission path is unchanged. Hardware validation is pending.

A subsequent pre-midpoint hardware capture shows that the duplicate-subdivision change materially reduced prepare cost, but the later half of FLD_A3 still runs close to the 30 Hz limit. Using 34 ordinary late-section `[ScenePerf]` samples (field geometry, upload < 10 ms), render median is 33.563 ms with a 36.216 ms p90. The median ordinary buckets in that late section are: build 12.585 ms, object append 9.300 ms, lighting 3.795 ms, Gouraud visibility preparation 3.300 ms, Full-mode Gouraud payload 6.239 ms, index/draw phase 1.458 ms, clear 2.046 ms, and final GXM wait 1.829 ms. The payload grows with visible-quad count and reaches 8.376 ms in the heaviest sampled frame. This capture is the baseline for the midpoint-grid optimization; it predates that optimization.

Full mode's 3x3 subdivision samples each source quad only at `u/v = 0, 1/2, 1`. Commit `434a18b9cd33531461cad00ef4123dcfcfdc0bcc` (`Use midpoint grids for Full Gouraud payload`) replaces repeated general bilinear evaluation with an exact nine-point midpoint grid shared by positions, UVs, and Gouraud shade channels. The source corner ordering, 3x3 topology, texture flip handling, visible-quad selection, and draw ordering are unchanged. This directly targets the measured ~6-8 ms ordinary Full-mode payload hotspot and also removes unnecessary general bilinear work from prepare-time subdivision UV/initial-position construction. Hardware visual/performance validation is pending.

**Midpoint-grid hardware result (2026-10-07): PASS, materially faster.** The user reports consistently better traversal performance with no noted shading/presentation regression. In 37 mode-3 `[ScenePerf]` samples from the post-change capture, median render time is 28.207 ms and p90 is 40.787 ms. Restricting to the denser samples at 900+ visible polygons gives median render 30.624 ms, median build 13.031 ms, median object append 9.922 ms, median lighting 4.366 ms, median Gouraud visibility preparation 3.594 ms, median Gouraud payload 2.581 ms, median main submission 4.215 ms, and median final GXM wait 1.857 ms. Relative to the pre-midpoint late-section baseline (render 33.563 ms median; Gouraud payload 6.239 ms median), the payload fell by about 3.66 ms / 59%, while dense-scene render median improved by about 2.94 ms / 8.8%. The midpoint arithmetic therefore removes most of the intended hotspot and puts many dense ordinary frames within the 33.3 ms 30 Hz budget.

The same capture confirms prepare-time subdivision work also fell: recurring mapped-capacity prepare events commonly show `subBuild` around 2-3 ms rather than the earlier 4-6 ms range, consistent with removing general bilinear evaluation from prepare-time 3x3 construction. The dominant steady-state CPU cost is now the flattened object append/transform path (~9-10 ms median in dense samples), followed by lighting (~4.4 ms) and Gouraud visibility preparation (~3.6 ms). The architectural next step remains persistent resident model/cell geometry with per-draw transforms, which should reduce both steady-state append cost and geometry-set rebuild churn without changing Azel visibility ownership.

Before replacing the monolithic live scene with resident per-model geometry, one low-risk append-path checkpoint removes redundant work already implied by the adapter's canonical layout. `LiveVdp1Model` expands every Saturn quad as `A,B,C / A,C,D`; the live flatten path was nevertheless matrix-transforming all six expanded vertices independently. Commit `7233bee939122a56ab64737b8ca63620e437847c` (`Skip duplicate quad vertex transforms`) transforms only A, B, C, and D, then copies the transformed A/C values into the duplicated triangulation slots. The flattened six-vertex order, colors, lighting-position mirror, polygon/material ordering, billboards, and downstream submission semantics are unchanged; a generic fallback remains for any future noncanonical model layout. This removes one third of the per-quad matrix transforms in the measured ~9-10 ms dense-scene object-append hotspot. Hardware visual/performance validation is pending.

**Post-duplicate-transform hardware observation (2026-10-07):** steady-state field performance now separates into two clear bands. With `staticRebuilt=0`, roughly 1,000-1,080 polygon scenes can reach ~1.5-2.1 ms object append and ~15-20 ms total render time. Similar polygon counts in frames flagged `staticRebuilt=1` still sit around ~9-11 ms object append and ~28-33 ms render. First-use model/material misses and texture-growth stalls remain separate, much larger events.

A later hardware capture after the billboard invalidation fix showed the next architectural fact directly: normal field traversal now keeps `staticRebuilt=0`, but `[FieldStream]` reports `staticSubs=0` with roughly 22 billboards throughout FLD_A3. The static cache itself is no longer being spuriously invalidated; rather, the environment never enters it because mode 3 still defaults ordinary submissions to the forced-dynamic camera-space path. Dense steady-state rendering is mostly inside the 33.3 ms target, but object append/transform remains around 9-10 ms and geometry-count changes still trigger `prepare_vdp1_model()`.

**Phase 4 world-space/static-grid checkpoint (pending hardware validation):** commits `6c82eb4` through `a0b5cdc` move field presentation onto an explicit native-camera boundary without changing Azel ownership. The generated `fieldCamera.cpp` copy captures Azel's exact active view matrix immediately after `applyCameraStatusToEngine()` writes it to `pCurrentMatrix`. The render bridge removes that view transform from ordinary mode-3 submissions, rotates the captured directional light back through inverse(view), and publishes world-space model/light state. Neptune receives the same native 3x4 view matrix at the presentation boundary, transposes it for its row-vector shader convention, and reapplies it exactly once. The field lighting falloff depth path now derives view-space Z from that same native view matrix, preserving the corrected Saturn/Azel lighting result.

The generated FLD_A3 `s_visdibilityCellTask::gridCellDraw_normal()` copy now gives the normal textured environment-grid model an explicit identity→translate→ZYX world transform and publishes it with `dynamic=false`. Azel still owns clipping, depth-range/LOD selection, active-cell visibility, and model choice. Dragon/rider, moving tasks, effects and billboards remain on the dynamic path. Static-cache identity now also hashes the stable model matrix, so a true world-transform change invalidates the cached prefix while camera movement alone does not.

Expected hardware diagnostics for this checkpoint:
- `staticSubs > 0` during normal FLD_A3 traversal;
- `billboards` remains nonzero and `staticRebuilt=0` on steady frames;
- scene orientation/winding, radar, dragon relationship and corrected lighting remain visually unchanged;
- dense-frame `obj` time should fall because rigid grid geometry is no longer matrix-transformed every frame;
- cell/LOD visibility changes may still rebuild the current monolithic static prefix. Eliminating that remaining visible-set rebuild requires the later persistent per-model/per-cell GPU resource cache; this checkpoint establishes the world-space/static classification required for that architecture.

**Phase 4 hardware result (2026-10-07): camera/lighting presentation remained viable, but static-grid classification did not activate.** The test completed normal FLD_A3 traversal and preserved the previously corrected lighting, field orientation/winding, radar and general scene presentation. However, every sampled `[FieldStream]` record still reports `staticSubs=0` while billboards remain ~22 and `staticRebuilt=0`. Dense ordinary frames likewise remain on the old dynamic flatten path: a representative 1,010-polygon frame reports `obj=10.152 ms`, `build=13.121 ms`, and `render=29.696 ms`. First-use texture growth is unchanged and still produces hundreds-of-milliseconds stalls, e.g. `textures=933->941`, `texUpload=344.698 ms`, `build=375.459 ms`.

This means the new native-view/world-space infrastructure did not regress the field visibly, but the specific FLD_A3 static-grid hook that should publish `dynamic=false` is not reaching the renderer. The next action is **not** to continue optimizing the dynamic path. First verify the generated `o_fld_a3.cpp` build copy and make the generated-source patch self-checking. The current CMake patch uses `string(REPLACE ...)` inside a source file that already receives several other replacements; the existing final `source_text != original_text` guard only proves that *some* replacement occurred, so the static-grid replacement can silently miss without failing configuration. Add a dedicated sentinel/count check around the `gridCellDraw_normal()` replacement, inspect the generated source to confirm the inserted `set_town_submission_context(... dynamic=false ...)` code is present at the exact `addObjectToDrawList()` boundary, and only then retest hardware.

Acceptance gate for the retry:
- generated `o_fld_a3.cpp` visibly contains the explicit static-grid submission context;
- CMake configuration fails if that exact patch does not apply;
- hardware `[FieldStream]` reports `staticSubs > 0` in FLD_A3;
- `billboards` remains nonzero;
- steady frames keep `staticRebuilt=0`;
- dense `obj` time falls materially below the current ~9-10 ms range without visual regressions.


Code audit found that the static cache was being invalidated by the mere presence of a billboard: `g_liveTownStaticRebuilt = hasBillboards || staticSignature != g_liveTownStaticSignature`. Billboards are already classified dynamic and appended after the cached static prefix, so they do not belong in the static invalidation condition. Commit `df6fc95f34f1b81f2432112402c119a31c518ee8` (`Fix field cache churn and native light falloff`) removes billboard presence from the static-cache rebuild condition and extends `[FieldStream]` with `staticSubs`, `billboards`, and `staticRebuilt` counters so hardware can verify that dynamic billboard activity no longer churns the static cache. This does not change Azel visibility, model ordering, or billboard transforms.

The same audit found a concrete lighting correctness bug matching the user's report that shaded/full output looked darker than Saturn. `updateLiveTownAzelLighting()` retained the old Ruins bring-up far clip constant `0xF000` when indexing Azel's 32-entry light falloff table. FLD_A3 publishes a native far clip of 32.0 (`0x200000` raw), so the hard-coded short range drove field geometry too quickly toward the far/dark end of the falloff curve. The same commit now derives the fixed-point reciprocal from the active native field far clip, matching upstream `GetDistanceFalloff()`. It also fixes the directional-light channel order to match upstream `ComputeColorFromNormal()`: `setLightVector_M` stores the color array reversed, so red uses `lightColor[2]`, green `[1]`, and blue `[0]`. No arbitrary brightness multiplier is applied; the change restores Azel/Saturn math. Hardware visual/performance validation is pending.

First-use resource spikes remain distinct from the steady-state problem. New model/material discovery can still force large texture uploads (hundreds of milliseconds) when the resident texture set grows; this is a future persistent-residency/prewarm concern, not the current ordinary-frame 30 Hz ceiling.

## Runtime path

```text
Ruins elevator sequence
        ↓
EVT004_1.CPK / EVT004_2.CPK
        ↓
Azel evaluates game status 5
        ↓
setNextGameStatus(0x50)
        ↓
game status 0x50
        ↓
game mode 3 / field index 1
        ↓
loadField(...)
        ↓
FLD_A3.PRG
        ↓
overlayStart_FLD_A3(...)
        ↓
initField(...)
        ↓
field task graph
        ↓
dragon / camera / visibility / scripts / encounters
        ↓
activateDragonFlight()
```

## Milestone ladder

```text
A — FLD_A3 loads without crash
B — field task graph runs
C — first 3D geometry visible
D — correct field camera
E — dragon visible
F — dragon animation
G — player-controlled flight
H — correct field visibility / LOD
I — VDP2 / background presentation
J — radar / LCS / field UI
K — field effects and native ray/laser presentation
L — progression to the next Azel game state
```

## Phase 1 — Native FLD_A3 entry

Restore `overlayStart_FLD_A3` to the generated Vita field dispatcher and instrument the transition without changing rendering behavior.

The hardware log should establish:

```text
game status 5 completes
→ request 0x50
→ game mode 3 / field 1
→ FLD_A3.PRG selected
→ overlayStart_FLD_A3
→ initField
→ common field resources
→ field file list
→ field script task
→ dragon task
→ field overlay tasks
→ camera task
→ encounter task
→ sound-bank request
```

Acceptance gate: Azel reaches a stable FLD_A3 field task graph, even if the screen remains black. No field-specific Neptune rendering is added in this phase.

**Hardware validation (2026-10-06): PASS.** The normal title → Ruins → elevator → post-elevator movie route reaches game status `0x50` / mode 3, loads `FLD_A3.PRG`, initializes the native field task graph, continues running with roughly 200 active tasks, and native BGM is audible. Presentation remains unchanged as expected because mode 3 had not yet been published through Neptune.

## Phase 2 — Mode 3 presentation ownership

Extend Lagi's generic scene capability check from town-only mode 1 to field mode 3, and add a field presentation adapter to `lagi_scene_bridge`.

The adapter is a presentation boundary, not a second field runtime. Azel remains authoritative.

Acceptance gate: mode 3 participates in the existing producer/consumer presentation boundary without regressing towns or movies.

**Implementation in progress:** mode 3 now uses the same generic publish boundary as town mode 1. Neptune tracks the active Azel game mode separately from the town actor, so field frames do not inject Edge. The initial field adapter deliberately treats Azel's submitted matrices as camera-space and publishes them against an identity host camera; this is a diagnostic first-visible-frame step, not the final field/world transform model.

## Phase 3 — Field presentation adapter

Expose renderer-facing state already produced by Azel:

- field camera and projection/FOV
- static field geometry
- visibility results
- dynamic field objects
- dragon world pose and animated hierarchy
- rider hierarchy
- active lighting
- VDP1 command stream
- VDP2 state

Reuse the existing generic Azel render bridge and Neptune material/geometry paths.

Acceptance gate: recognizable Excavation geometry appears, even if transforms are not yet correct.

## Phase 4 — Transform semantics

Determine whether each field submission is world-space or already view-relative, then use scoped bridge metadata to prevent double-camera transforms.

Validate terrain, dynamic objects, dragon, rider, and effects separately.

Acceptance gate: stable world geometry, correct dragon/world relationship, and no mirroring or double-camera errors.

## Phase 5 — Dragon presentation and control

Consume Azel's existing field dragon task. Do not implement Vita-specific flight logic.

```text
Vita controls
   ↓
Saturn input state
   ↓
Azel dragonFlightUpdate()
   ↓
Azel position / orientation / animation
   ↓
Neptune presentation
```

Acceptance gate: visible animated dragon and rider with working flight controls.

## Phase 6 — Field camera

Publish Azel's native field camera, including the configured follow mode and field FOV.

Acceptance gate: framing and flight response agree with Saturn reference behavior.

## Phase 7 — VDP2 / background

Bring field VDP2 state online incrementally. Log unsupported state first; do not block the first 3D milestone on perfect background composition.

Track BGON, NBG/RBG use, scroll, line scroll, CRAM, priority, color calculation, windows, and back-screen state.

## Phase 8 — Field UI

Reconnect the field radar, LCS/lock-on, HUD, text, and other VDP1/VDP2 UI through Azel's own state and command streams.

## Phase 9 — Field effects

Restore field-specific effects that were intentionally suppressed during the boot-to-Ruins milestone. In particular, A3 ray/laser drawing currently bypasses upstream desktop BGFX/GLM rendering and must be translated to Neptune rather than re-enabling the desktop path.

## Phase 10 — Saturn comparison and progression

Compare the same flight sequence against Saturn for spawn position, orientation, camera, movement response, animation, visibility/LOD, geometry, lighting, radar/LCS, audio, and script progression.

The milestone completes when Azel naturally advances from the supported FLD_A3 sequence into its next game state.

## Deferred audio optimization

Audio performance work is intentionally separate from flight bring-up. The current deferred branch is `feature/audio-affinity-profiling` / PR #11. It preserves the existing CPU2 default and adds hardware A/B affinity testing for CPU 0, CPU 1, CPU 2, and all user cores. A dedicated ARM DSP worker is considered only after that profiling establishes a worthwhile synchronization target.
