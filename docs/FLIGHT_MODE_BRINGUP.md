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

## Rendering performance and streaming investigation

Traversal hitches correlate with changes in the visible field geometry set. The branch currently records:

- `[FieldCellTransition]`: camera-cell and active-cell-count changes.
- `[FieldStream]`: frame, submissions, polygon/vertex counts, model-cache misses, material misses, texture-count growth, whether `prepare_vdp1_model()` ran, GPU texture count/dirty state, and append/material/upload/build timings.
- Per-published-frame model-cache misses exposed through the render bridge.

The first diagnostic run showed that Neptune represented the live field as one changing flattened mesh. When its visible geometry size changed, Neptune called `prepare_vdp1_model()` again. Before texture preservation, that function released and re-uploaded the entire resident texture set, producing roughly 400-470 ms uploads even when the frame introduced no new model or texture.

The current branch head, `8a531f88a8d59381c2d0daa8fad1e5c1f7ae6a19` (`Reuse field textures across geometry changes`), adds texture-preserving geometry rebuilds through:

```cpp
prepare_vdp1_model(const Vdp1ModelSource&, bool preserveResidentTextures = false)
releaseResidentVdp1Model(bool preserveTextures = false)
```

The field path requests preservation when rebuilding live geometry. CRAM and VDP1 texture invalidation still mark texture data dirty or free textures when required. Hardware traversal is noticeably smoother after this change, so the texture-residency optimization is qualitatively validated.

It is not the final streaming design. The latest hardware log still contained about 279 geometry reprepares. Ordinary non-prepare field builds had a median around 15.8-15.9 ms; prepare-event builds had a median around 95.5 ms, including roughly 81.7 ms of upload work. Those events remain visible hitches.

Latest measured workload timings:

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

Field submissions are temporarily forced dynamic through `begin_frame(forceDynamicSubmissions=true)` because they currently arrive with camera-space matrices. This avoided catastrophic static-cache churn when those matrices changed, but it cannot be the final resource model. Phase 4 must classify submission transform spaces and apply Azel's field camera exactly once.

The existing field camera path is:

```text
field camera task
    → camera slot
    → applyCameraStatusToEngine()
    → updateEngineCamera(...)
    → cameraProperties2 / pCurrentMatrix / m384_viewMatrix
```

Upstream helpers include `getFieldCameraStatus()` and `getFieldCameraMatrix()`. Town already uses `begin_view_relative_submission_scope()` and `removeViewTransform()` to convert scoped `view * world` submissions back to world space. Investigate a generic equivalent for static field cells, but do not blanket-convert field submissions until terrain, dynamic objects, dragon/rider, and effects have each been classified.

The immediate profiling task is to split the remaining ~80-100 ms reprepare events into buffer free/allocate/map/unmap, topology and UV rebuild, subdivision-buffer rebuild, CPU flatten/transform, and other `prepare_vdp1_model()` work. Instrumentation should remain narrowly scoped and preserve current rendering behavior.

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
