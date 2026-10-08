# Flight Mode Bring-Up

Lagi's first flight milestone begins immediately after the Ruins elevator movie sequence. Azel advances game status 5 to status `0x50`, which maps to game mode 3 / field index 1 and loads `FLD_A3.PRG` ("above excavation").

The implementation follows the same ownership rule used for the Ruins runtime:

```text
Azel decides.
Lagi services.
Neptune renders.
```

Flight behavior, field scripts, dragon movement, camera state, visibility, animation, encounters, VDP1/VDP2 state, and progression remain Azel-owned. Lagi restores the Vita-facing services and presentation paths required to let that runtime execute natively.

## Current hardware-validated state (2026-10-08)

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
- **Current branch HEAD / latest hardware-tested corrective checkpoint:** `f0ef39492ccca4ab6789420dd8bd1670b00e1b2d` (`Align Full Gouraud correction with Lighting mode`) — hardware-tested, visual regression still present.
- **Last pre-regression reference region:** before/around the first successfully active FLD_A3 static-grid submission path. Historical A/B checkpoints used during this session include `7b71f8f41c679ac09e76912ee2295e6a4cd8922c`, `662c3f7de0f9f2426b6cc4579127628b0f0afbe7`, `ab2b44e1820825c050d3130cb5639bb0103c74c8`, and `528778889bc5b697579c9a1f23610935cdcd414b`. These were used to bracket the regression, not as new forward-development heads.
- **Draft pull request:** #12 (`Bring up native FLD_A3 flight entry`)

This is the active rendering checkpoint. Neptune is the native SceGxm renderer, not VitaGL. The gameplay panel is 480x272 and the immediate target is reliable 30 Hz within a 33.3 ms frame budget. Do not merge PR #12 until the user explicitly approves it after Vita/Vita TV testing.

The normal route from the Ruins elevator through status `0x50`, mode 3, `FLD_A3.PRG`, `overlayStart_FLD_A3()`, and `initField()` is hardware-validated. The authentic Azel field task graph, player-controlled flight, BGM/SFX, dragon/rider, geometry, visibility/grid/LOD, radar, LCS, and UI are live. Preserve these hardware-confirmed results:

- Field orientation and winding are correct: mode 3 does not use the town Saturn-to-GXM X mirror or an extra winding XOR.
- The FLD_A3 radar map remains point-filtered at its original 48x48 output-pixel size.
- Field lighting uses the native 32.0 (`0x200000` raw) far clip, the upstream reversed light-color storage (`R=[2]`, `G=[1]`, `B=[0]`), and model-space normals with the light rotated back once per submission. Do not add brightness or gamma compensation.
- The exact midpoint-grid Full Gouraud optimization and duplicate `A,B,C / A,C,D` vertex-transform reuse are hardware-valid checkpoints and must remain intact.
- The billboard invalidation correction is working: billboards remain dynamic and no longer force the static prefix to rebuild.

At the latest hardware-tested checkpoint, the static-grid hook is active and the material-binding reuse change has removed the previous ~110-128 ms recurring material-resolution penalty. Across 39 sampled active-field `[ScenePerf]` records, render median is 22.855 ms. The 34 non-rebuild samples have a 21.259 ms median, 25.605 ms p90, and 26.206 ms maximum. Dense 900+ polygon non-rebuild samples have a 23.576 ms median. All five sampled frames over 33.3 ms are static-prefix rebuild frames; their render median is 44.244 ms and maximum is 46.924 ms. First-use texture growth remains a separate hundreds-of-milliseconds residency stall.

### Visual regression discovered after the previous work session

A clear lighting/shading regression is now the highest-priority correctness issue. The user reports that this was not present in the previous work session and became visible around the point where the FLD_A3 environment grid actually began entering Neptune's static submission/cache path after a fresh configure/build. The important boundary is therefore not merely when static-support code was written, but when the generated FLD_A3 hook was confirmed to be present and hardware logs began reporting matching nonzero static contexts/submissions.

The regression is visual, not a geometry-loss problem. Video and same-camera mode cycling show that the affected canyon/rock surfaces remain present in Texture, Quads, and Wires, while Full and Lighting exhibit incorrect quad-to-quad illumination. The darkest faces can resemble holes against the unfinished black background, but the geometry is still drawn. Do not return to missing-geometry/culling explanations unless new evidence contradicts this mode comparison.

Current evidence localizes the fault to lighting/shading state or its Full-path consumption:

- The static world transform itself has been checked against Azel's native draw-boundary matrix. The native-vs-synthesized comparison showed only tiny 16.16 rounding deltas.
- The captured/synthesized light direction likewise matched to rounding-level differences (typically 0-1 against a dominant 4096 component), so a gross model-space/world-space light-vector mismatch is not supported by hardware diagnostics.
- The static path is unquestionably active in the affected build: hardware logs show matching `staticCtx=set/consumed`, nonzero `staticSubs`, stable static identity hits, and `staticRebuilt=0` cache-hit frames.
- The problem appears as hard per-quad brightness discontinuities on continuous surfaces. Texture-only presentation remains coherent.
- Mode cycling in a particularly obvious area showed that Full can become much darker than the same geometry shown by Lighting, which keeps the textured-Gouraud composition path under suspicion even though CPU-side lighting inputs remain part of the investigation.

### Lighting-regression investigation and attempted corrections

The following checkpoints were implemented and hardware-tested during the 2026-10-07/08 session. None has removed the regression, so do not treat any of them as the established root-cause fix:

1. **`3803c8542b5698285a56d7458b4ec25fb3dfa8f3` — `Keep cached static lighting in model space`.** Static cached normals remain model-space, so the refreshed Azel light was rotated back through the model basis before dot products. This made initial-build and cache-hit coordinate treatment consistent, but hardware still showed the same bad shading.
2. **`9730e3e94bc118d949dbb6dcb3c73a3280f95c6e` — `Trace native versus cached field lighting`.** Added bounded `[FieldLightCompare]` diagnostics at the native draw boundary. Hardware showed native and synthesized matrices/light vectors agreeing to rounding precision, effectively ruling out the generated static transform/light pair as the primary cause.
3. **`e3201a304cd32c985c96f3f8a515439555302713` — `Trace field object versus polygon falloff`.** Added `[FieldFalloffCompare]` diagnostics comparing Azel's native object-origin view-depth bucket with Neptune's existing per-polygon first-vertex bucket. Large differences were observed for many rigid objects (often most polygons in a submission), making falloff scope a plausible semantic mismatch.
4. **`410c2c37eb45e7a4654a820c0362c1e8a193531d` — `Clear cached static Gouraud shading each frame`.** Restored one lifecycle behavior from the former all-dynamic path by clearing the cached static `gouraud555` prefix each frame rather than allowing same-size `resize()` to retain previous-frame shade values. Hardware showed no correction.
5. **`e56999d9536e1034b5d1f650b21b4d624fedc21d` — `Use object-origin falloff for static field lighting`.** Applied one falloff bucket per rigid FLD_A3 static submission using the native object-origin depth. Hardware mode screenshots still showed the regression. This rendering behavior was subsequently removed; the diagnostic depth capture remains useful evidence.
6. **`f0ef39492ccca4ab6789420dd8bd1670b00e1b2d` — `Align Full Gouraud correction with Lighting mode`.** Restored the previous per-polygon falloff behavior and changed Full's textured-Gouraud combine to reconstruct/quantize the signed RGB555 Gouraud value around neutral `0x10` before adding it to the base texture, matching the representation exposed by Lighting mode. Hardware still showed the issue.

During regression bracketing, the remote branch HEAD was temporarily moved backward for exact historical A/B builds, including `528778889bc5b697579c9a1f23610935cdcd414b`, `7f8f68ba01c199fd18fd48d74578de78c66934d3`, `7b71f8f41c679ac09e76912ee2295e6a4cd8922c`, and `662c3f7de0f9f2426b6cc4579127628b0f0afbe7`. The branch has now been restored to `f0ef39492ccca4ab6789420dd8bd1670b00e1b2d`. When testing old commits locally, use a hard reset to the remote branch plus `git submodule update --init --recursive`; old checkpoints may also require a clean Ninja/VitaSDK configure because a reused build directory can fall back to the Visual Studio/MSVC generator.

### Current regression hypothesis and next diagnostic

**2026-10-08 local diagnostic and re-entry checkpoint (base `814805b280226e81b6e39241b18dce8542528adc`, uncommitted):** The supplied hardware capture contains 192 `[FieldPolygonCompare]` records and 48 `[FieldPolygonPayload]` records with zero reported mismatches. These are bounded samples of the first eight static submissions per sample, over 24 sample frames; they do not prove equivalence for every polygon or GPU draw. The sampled cached lighting controls/counts, normals/colors, command associations and CPU shade writes agree. The user reports that the issue is flight-only and is especially apparent when previously offscreen quads return into view during the opening camera pan or return flight. Ruins rendering remains correct.

Source inspection identifies a separate concrete subdivision-position lifecycle defect. A capacity-reuse prepare rewrites material UVs but preserves XYZ from the previous polygon association. Full and Lighting then skip offscreen polygons and write static XYZ only on `g_liveTownStaticRebuilt` frames. A static quad skipped in that rebuild can become visible on a cache-hit frame without receiving current XYZ, even though CPU visibility and lighting use the current geometry. Movement that changes the static submission set can rebuild and temporarily repair that quad. This explains the delayed re-entry symptom without changing Azel visibility or lighting math.

The local correction keeps render-thread-owned position-valid bits for field static polygons. Static-prefix rebuild and subdivision-buffer prepare invalidate them; a visible static quad writes its nine current midpoint XYZ positions before becoming valid. Static quads already initialized keep the existing position-write optimization, and dynamic submissions retain their per-frame updates. This correction is gated to native mode-3 geometry; town/Ruins behavior is unchanged. Up to 32 `[FieldPositionRepair]` records report actual stale XYZ before repair on cache-hit frames, independently of the earlier diagnostic sample window.

Vita compilation, package generation and whitespace checks passed. **Hardware validation is pending.** Repeat the opening pan, stationary turnaround and return flight in Full and Lighting, then inspect the new log and presentation. Do not claim that the persistent lighting regression is fixed until the actual affected flight surfaces render correctly; an additional brightness defect remains possible. The diagnostic and correction are local changes in `C:\Dev\Lagi`, not pushed branch commits. Do not repeat the prior falloff or Gouraud rewrite attempts without new contradictory evidence.

The strongest remaining hypothesis is **static-light refresh equivalence / retained per-polygon lighting metadata or downstream shaded-payload association**, not missing geometry and not a gross world-transform error. The old all-dynamic path reconstructed the full flattened submission every frame; the static cache-hit path retains polygon records/associations and refreshes only a smaller set of lighting state. A targeted next diagnostic should compare the cached static polygon lighting inputs against the current adapted model on the same submission and polygon index:

- `lightingControl`
- `lightingCount`
- all per-corner normals
- mode-2 per-corner colors / `hasColor`
- current `LivePolygonLightState`
- current visibility result
- `gouraud555` before and after `updateLiveTownAzelLighting()`
- the final Full shaded payload written for the same polygon

Prefer hashing/comparing the cached and current values and logging only mismatches or a bounded set of affected polygons. Do not add another broad lighting rewrite until one of these retained associations is shown to diverge. The historical regression boundary around static-grid activation remains the key clue.
### Current correction: preserve cached Saturn mesh-command metadata

The latest full-mode hardware run passed the complete static-grid bridge gate: every sampled active `[FieldStream]` record had matching nonzero `staticCtx=set/consumed`, `staticSubs > 0`, and no context mismatch. The generated FLD_A3 hook, bridge consumption, world-space classification, and Neptune static path are therefore all active. Do not return to the earlier generated-source activation diagnosis unless a later log actually reports `staticCtx=0/0` or `staticSubs=0`.

Hardware exposed one visual regression at map entry: while the scripted camera starts above the dragon and travels behind it, a section of field mesh that begins offscreen does not appear as the camera reveals it; the section appears only after player movement. Source inspection rules out a stale camera visibility-preparation cache: `prepareLiveTownGouraudVisibility()` is rebuilt from the current WVP every authentic-camera frame.

The concrete renderer bug is in the metadata paired with the cached static prefix. `g_liveTownMeshRanges` records source polygons carrying Saturn VDP1 mesh mode (`CMDPMOD` bit 8), which Neptune submits with ordered-overdraw behavior. The frame builder cleared those ranges every frame. A static-cache rebuild recreated them, but a cache hit restored only the cached vertices, polygon records, materials, and lighting arrays. It did not restore the static mesh-command ranges. A later Azel visible-set change—commonly triggered when the player starts moving—rebuilt the prefix and recreated the missing metadata, matching the observed delayed appearance.

The next checkpoint preserves a separate copy of the static mesh-command ranges when the static prefix is rebuilt and restores it on cache hits before dynamic submissions append their ranges. CRAM and VDP1 texture invalidation clear the paired metadata. `[FieldStream]` now reports `meshRanges=total/static`, allowing the next hardware run to prove that static ranges remain present on `staticRebuilt=0` frames. This is renderer-owned presentation metadata; Azel's visibility, clipping, LOD, model choice, and scripted camera remain untouched.

### Immediate next task

1. Keep the branch at `f0ef39492ccca4ab6789420dd8bd1670b00e1b2d` unless performing a deliberate historical A/B.
2. Reproduce the obvious canyon/rock shading defect and capture the same camera position while cycling Full, Texture, Lighting, Quads, and Wires.
3. Instrument cached-static polygon lighting metadata versus the current adapted model for the exact same static submission/range. Log bounded mismatches in lighting mode/count, normals, mode-2 colors, visibility, CPU `gouraud555`, and final Full payload association.
4. Preserve the proven field invariants while diagnosing: native field orientation/winding, 32.0 far clip, reversed Azel light-color storage, point-filtered 48x48 radar, billboard dynamism, and Azel-owned visibility/LOD.
5. Do not resume residency/performance optimization until this visual regression is understood. The static path materially improves performance, but correctness takes priority.
6. Keep audio work, ray/laser rendering, and unrelated gameplay changes out of this checkpoint.

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

**Identity checkpoint crash analysis (2026-10-07): fixed in code, hardware retest pending.** The crashing package was identified exactly as commit `1b7291ea9829fdf37056bc47b3e075cbb9319048` (`Trace stable field resource identities`), with a complete ~4.1 MB decompressed Vita core and the matching unstripped `lagi.velf`. Symbolication is conclusive: the fault occurred on the `LagiRender` thread inside the new `LiveTownStaticIdentity` `std::unordered_set` while `buildLiveTownFrame()` was entering `bucket_count() -> reserve()`. The bad address `0xCCCCCD28` is allocator poison, and the stack rules out the elevator script, Azel gameplay, and the later `8510605` Full-mode payload optimization.

The identity set had accidentally acquired cross-thread mutation: CRAM/VDP1 invalidation callbacks could call `g_liveTownKnownStaticIdentities.clear()` while `LagiRender` was simultaneously finding/inserting/reserving buckets. Commit `37d545bbbaa4f29b4471d72d66a4a9fe4b07855f` restores renderer ownership. Invalidation producers now only increment an atomic identity-reset epoch; at the start of `buildLiveTownFrame()`, `LagiRender` observes a changed epoch and performs the actual `unordered_set::clear()` itself before any `bucket_count()`, `reserve()`, `find()`, or `emplace()` work. The diagnostic semantics remain the same—known identities reset after resource invalidation—but the container is never directly mutated off the render thread. Hardware validation must repeat the transition/traversal route that produced the core before residency work continues.

**Follow-up visual ownership fix (2026-10-07): hardware validation pending.** The first post-crash-fix hardware traversal survived and showed materially better steady field performance, but exposed sporadic missing/reappearing static quads, incorrect texture bindings, and an initially black/incomplete static field that repaired itself after movement forced a visible-set refresh. The remaining CRAM/VDP1 invalidation callbacks were still directly clearing `g_liveTownMaterialCache`, `g_liveTownStaticMeshRanges`, signatures/prepared state, and in the VDP1 case freeing resident textures off the render thread. Those objects form one renderer-owned resource transaction: clearing range/material metadata while a cached static prefix remains drawable can make geometry disappear until the next static rebuild, while freeing texture state concurrently can present stale or mismatched bindings. The callbacks now publish only atomic invalidation bits. `LagiRender` consumes those bits before rendering a frame and coherently clears material/identity/static-range state, invalidates signatures/prepared state, frees VDP1 textures when required, and marks texture data dirty. Resetting the static signature in the same render-thread transaction guarantees the following `buildLiveTownFrame()` rebuilds the static prefix instead of reusing half-invalidated state.

**Cached static-light coordinate fix (2026-10-07): hardware validation pending.** A full there/back traversal in Full, Lighting-only, and Texture modes after the ownership fix showed no missing meshes and no texture corruption. The remaining apparent holes were black-lit quads against the not-yet-implemented field background. The cause was isolated to `refreshLiveTownStaticLighting()`: initial `appendLiveTownModel()` correctly rotated Azel's captured camera/world-space light vector into model space before dotting it against model-space normals, but the cached-static refresh path copied the raw light vector directly. Static cache hits therefore changed lighting coordinate spaces without changing geometry or materials. The model-space light transform is now a shared helper used by both initial append and cached-static refresh, while billboard lighting retains its existing camera-facing basis path. This is intentionally a lighting-only correctness fix; geometry caching, texture residency, visibility, audio, and Azel ownership are unchanged.

**Static-light equivalence diagnostic (2026-10-07): hardware capture pending.** Hardware showed the lighting defect persisted after the cached-refresh coordinate fix, while geometry and texture correctness remained stable. The next checkpoint therefore makes no rendering change. For a bounded set of rigid FLD_A3 static-grid submissions, the render bridge now samples Azel's actual `pCurrentMatrix` and raw `currentLightVector_M` at the native `addObjectToDrawList()` boundary, removes the captured field view from the native matrix, and compares that result with the generated hook's synthesized world matrix. It also computes model-space light two ways: directly from Azel's native view-space matrix/light pair and from Lagi's synthesized world-space matrix/light pair. `[FieldLightCompare]` reports maximum basis/translation deltas and all three model-space light deltas keyed by cell/object/model identity. If these values agree while the visual defect remains, the investigation moves downstream to polygon-to-light association/order rather than applying another speculative transform fix.

**Field falloff-scope diagnostic (2026-10-07): hardware capture pending.** The native-vs-synthesized matrix/light checkpoint matched to rounding precision on hardware (basis deltas only a few 16.16 units and model-space light deltas 0–1 against a dominant 4096 component), ruling out the static world transform as the cause of black/inconsistent quads. The next diagnostic remains presentation-neutral. At each rigid FLD_A3 static draw boundary, the bridge now records Azel's native object-origin view depth from `pCurrentMatrix[2][3]`. Neptune's cached-static lighting refresh computes the falloff bucket implied by that object-origin depth and compares it with the existing per-polygon first-vertex falloff buckets across the same submission range. `[FieldFalloffCompare]` reports the object bucket, polygon bucket range, and count of polygons that differ. This tests whether Neptune is incorrectly applying distance falloff per polygon where Azel's native draw path is object-scoped; no lighting values or draw state are changed by this checkpoint.

**Static Gouraud-prefix equivalence fix (2026-10-07): hardware validation pending.** The shading regression first became visible when FLD_A3 grid geometry successfully entered the static submission path. Path comparison found one concrete per-frame lifecycle difference: the former all-dynamic path passed every polygon through `appendLiveTownModel()`, which zeroed `gouraud555` before current-frame lighting, while a static cache hit only resized the cached prefix and therefore retained the previous frame's Gouraud payload. `updateLiveTownAzelLighting()` clears/recomputes a polygon only after its prepared-visibility check, so a skipped cached polygon could retain stale shade data. Static-prefix reuse now explicitly clears only the cached `gouraud555` entries every frame, restoring the old lighting-output lifecycle without touching geometry, polygon records, materials, textures, visibility, or resource residency.

**FLD_A3 object-scope falloff fix (2026-10-07): hardware validation pending.** The Gouraud-prefix reset did not remove the visible patchwork shading. The falloff diagnostic then showed a much larger semantic mismatch: for many rigid grid objects, Neptune's per-polygon first-vertex falloff buckets spanned wide ranges while nearly every polygon disagreed with the bucket implied by Azel's native object-origin view depth (for example, 25/28, 27/28, and 22/22 polygons differing on sampled objects). `LivePolygonLightState` now carries the native object-origin depth captured at Azel's `addObjectToDrawList()` boundary. `updateLiveTownAzelLighting()` uses that single native depth bucket for every polygon belonging to a rigid FLD_A3 static submission; submissions without native origin depth (dynamic objects and billboards) keep the existing per-polygon fallback. Light direction, normals, mode-2 colors, Gouraud interpolation, geometry, textures, visibility, and cache residency are unchanged.

**Mode-comparison correction checkpoint (2026-10-07): hardware validation pending.** Hardware mode cycling showed intact geometry and texture presentation, while Full became dramatically darker than the same surfaces shown by Lighting. The object-origin falloff experiment did not correct the defect and has therefore been removed from rendering behavior; the native-depth diagnostics remain available, and `updateLiveTownAzelLighting()` again uses the prior per-polygon falloff calculation. Shader review confirmed Full already intended Saturn's signed Gouraud correction semantics, but it consumed the interpolated floating correction directly while Lighting first reconstructed and quantized the 5-bit Gouraud value around neutral `0x10`. The subdivided Full shader and its non-subdivided half-precision fallback now explicitly share that decode: `light5 = round(clamp(shade*31 + 16))`, then `correction5 = light5 - 16`, then add the correction to the quantized base RGB555 texture. This keeps Saturn's additive signed correction model while making Full consume exactly the same 5-bit lighting representation exposed by Lighting mode. Geometry, texture lookup, interpolation coordinates, visibility, and CPU lighting inputs are unchanged.

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

## Performance resumption — 2026-10-08 local checkpoints

The user confirms the flight visual regression is corrected. Preserve the static
subdivision position-validity correction. Performance is now the active objective:
consistent 30 FPS (33.3 ms deadline), best effort 25 ms across cell loads.

The 10:59:48 capture verifies texture-prefix retention: eleven growth uploads
cost 1.817 ms median and 8.899 ms maximum, replacing the previous ~350 ms full
reuploads. Initial field upload is still 298.118 ms. Sampled render median is
24.012 ms, p90 33.511 ms, max 39.957 ms; 10/89 samples exceed the deadline.
Some unsampled FieldStream frames still take 58-68 ms in build alone.

Completed lighting diagnostic bursts coincide with 56-58 ms build spikes even
without prepare, texture uploads or resource misses. The quiet checkpoint disables
polygon comparison/stage/payload, falloff and position-repair logs; timing and
texture-upload logs remain. No rendering behavior was changed by this cleanup.

Real geometry-capacity growth also remains expensive: frame 8645 spends 40.336 ms
in prepare/upload, including new buffer allocations and initialization; frame
10994 spends 44.476 ms with resident textures but geometry allocations. The next
local checkpoint reserves consistent capacity across base, textured, wire and
subdivision buffers for at least 2048 field quads (observed peak 1356). Larger
requests receive bounded 25-percent headroom up to the existing subdivision
16-bit index-count limit. Actual copied geometry, initialized active subdivision
payload and submitted draw counts remain unchanged. Other modes retain exact
allocation sizes. FieldGeometryCapacity logs active/reserved counts only when
allocating. Corrected field position validity and texture prefix retention remain.

The capacity checkpoint passes Vita syntax/Wall, full package build and whitespace
checks. Hardware validation is pending: repeat Full-mode end-and-back traversal;
expect reuseGeom=1 after first field allocation, zero recurring geometry allocation
buckets for scenes within reserved capacity, and no visual regression. This moves
allocation work to field entry and uses additional mapped memory; it does not
remove static-prefix reconstruction, first material/model misses, or every frame
above 33.3 ms. Changes remain local, uncommitted/unpushed.

The subsequent local checkpoint also retains subdivision UV payload at field
buffer slots when width, height and Saturn flip are unchanged and the same mapped
storage/topology already exists. These are the complete inputs to the current UV
formula. A new slot, changed dimensions/flip, or newly allocated storage rebuilds
UVs normally. Static position validity is still invalidated independently on
prepare, so UV reuse cannot revive the corrected stale-XYZ bug. Town behavior is
unchanged. Target is the measured ~2-3 ms recurring subBuild cost; actual timing
and visual equivalence require hardware validation. Includes the prior quiet,
texture-prefix and geometry-capacity checkpoints, all still local/unpushed.

The latest checkpoint adds FlightTimingWindow: every renderer frame contributes
to a 120-field-frame aggregate, reporting mean/max render time, counts above
25 ms and 33.333 ms, max build time and rebuild count. Windows reset on render
mode changes and leaving flight. Detailed ScenePerf samples remain. This catches
renderer deadline failures that the one-in-60 sample could miss, without per-frame
log writes. The final incomplete window is not emitted. These are renderer-only
measurements; game/display/presentation pacing still requires separate evidence.
The device D: drive was unavailable during this checkpoint, so no new hardware
performance conclusion was drawn. Full Vita compilation and whitespace checks
pass. All capacity/UV optimizations still require the next hardware capture.

The 11:15:48 hardware capture confirms substantially smoother traversal by user
observation. Sampled steady renderer median is 21.147 ms (max 31.881 ms); static
rebuild median is 35.794 ms (max 39.221 ms). All six sampled render deadlines
missed are rebuild frames. Only one geometry allocation occurred, but it was
mid-traversal at frame 2211: active=1057, build=86.843 ms, upload=59.313 ms.
The log contains no FlightTimingWindow lines and cannot validate the later UV
reuse/timing checkpoint. Initial upload remains a separate field-entry cost.

Audit found that reservation allocation was deferred by the reuse test: inherited
smaller town buffers were reused whenever the current field count fit. The new
check compares capacities to the requested reservation sizes, establishing the
2048-quad allocation on first field prepare rather than the first growing cell.
Non-field requested capacities equal active counts as before. This shifts known
allocation work to entry; it does not remove entry time or static rebuild cost.
Latest package includes all prior local checkpoints. Full Vita syntax/Wall,
package build and whitespace checks pass; hardware validation remains pending.

## Published local-build checkpoint (2026-10-08)

The preceding local checkpoints are now being published together on
feature/flight-mode-bringup. This includes the hardware-confirmed stale-position
rendering correction, retained texture uploads, geometry reservation, UV reuse,
and FlightTimingWindow instrumentation. The first-field reservation correction
still needs hardware validation. Vita package build and diff checks pass.

The shared GXM VDP2 sky request remains unfinished and is not included in this
checkpoint. Use the branch commit ID to identify builds; do not infer sky support
from the performance package filename.
## Flight descriptor lookup checkpoint (2026-10-08)

The 11:23:30 capture still has 16-26 ms first-model material resolution buckets
outside field entry. liveTownTextureIndex previously scanned every resident
texture for every polygon on each model's first encounter. Field mode now uses
an exact packed 64-bit PMOD/COLR/SRCA/SIZE key to find the same resident index.
Indexing processes only appended descriptors; duplicate keys retain the first
index, matching the previous search. The index is cleared with renderer
invalidation and replacement of the authoritative room texture source. Town
lookup behavior is unchanged. Texture decode, upload, visibility, lighting and
material semantics remain the existing paths.

Vita syntax/Wall, full package build and whitespace checks pass. Hardware timing
and visual validation are pending. This is a local change after f914c24; no push
has been performed. f914c24's push was rejected by automatic approval review and
an explicit user authorization request is pending. The shared VDP2 sky remains
unfinished. The performance goal remains unproven: the prior Full-mode capture
has 265 of 3960 measured renderer frames over 33.333 ms and 903 over 25 ms.
## Neptune compiler optimization checkpoint (2026-10-08)

An audit of the authoritative local build.ninja found Neptune compiled with
-ffunction-sections/-fdata-sections but no optimization flag. The CMake cache
has an empty CMAKE_BUILD_TYPE, so Release defaults were not active. The renderer
therefore performed the measured CPU transforms, lighting and material decode
without optimization, even though selected audio sources already used -O2.

LAGI_OPTIMIZE_NEPTUNE now defaults ON and adds -O2 only to
src/platform/vita/neptune_renderer.cpp. -fno-fast-math and -ffp-contract=off
preserve normal floating-point semantics and prevent fused operation contraction.
Azel gameplay and other sources retain their existing compiler settings. The
startup log reports [NeptuneBuild] optimized=1 fieldDescriptorIndex=1, allowing
the next capture to distinguish this build from prior unoptimized captures.
The generated Ninja rule confirms these flags apply to the renderer source.
Hardware performance and visual equivalence remain unverified.

For the existing local checkout, build the optimized renderer with:

```powershell
Set-Location C:\Dev\Lagi
$env:VITASDK = 'C:\Dev\VitaSDK'
$env:PSP2CGC = 'C:\Dev\sdk\host_tools\bin\psp2cgc.exe'
$env:PATH = "$env:VITASDK\bin;$env:PATH"
cmake -S . -B build -DLAGI_OPTIMIZE_NEPTUNE=ON
if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
cmake --build build --parallel 8
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
```

LAGI_OPTIMIZE_NEPTUNE=OFF removes the source-specific optimization for an A/B
capture; keep CMAKE_BUILD_TYPE and global compiler flags unchanged. In the
currently verified empty-build-type configuration this restores the previous
unoptimized renderer build. Package output remains build\Lagi.vpk. No remote
push has been performed; the explicit publishing approval remains pending.
The optimized checkpoint also adds FlightPresentWindow after display presentation.
Each 120-interval window reports mean/max elapsed presentation interval, fpsMilli
(actual interval-rate estimate multiplied by 1000), and display-vblank gaps.
over2Vblanks counts intervals that missed the normal two-vblank / nominal 30 Hz
cadence. Interval timestamps include game-thread waits, render work, GPU waits,
pacing, and intervening logging. This is separate from the renderer-only 25 ms
budget window and does not change scheduling. The movie path resets this counter;
view changes and leaving native field reset it too. The first field present
establishes the baseline, and a final incomplete window is not emitted.

Use a complete Full-mode end-and-back hardware traversal to assess this build.
Confirm NeptuneBuild optimized=1 fieldDescriptorIndex=1, inspect every
FlightTimingWindow and FlightPresentWindow, and verify returning offscreen
geometry still renders and lights correctly. Build success alone does not
establish 30 FPS or 25 ms. The log currently on D: remains the 11:23:30 capture
from before these changes, so no hardware improvement is claimed yet.

## Pullable checkpoint - 2026-10-08

The user explicitly authorized publishing the current flight checkpoint.
This combines the corrected static positions and cell-load improvements in
f914c24 with the exact field descriptor index, Neptune -O2 configuration, and
FlightPresentWindow instrumentation. Earlier references to pending publishing
approval describe the historical local checkpoints. The latest local Vita
package build passed; the optimized checkpoint still needs a fresh hardware
capture. Shared GXM VDP2 sky support remains unfinished and is not included.
