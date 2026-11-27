# 1D.0 — Neptune residency audit (initial source pass)

Scope: source inspection of src/platform/vita/neptune_renderer.cpp on feature/field-geometry-cache following 1C.2. No runtime changes. Line anchors refer to the inspected revision; bridge producer and complete draw-path audit remain open.

## A — Shared path and gates
At 15269–15335 nativeSceneMode is derived from native_scene_mode, camera validity and published submissions; buildLiveTownFrame is shared by eligible native scenes despite its historical name. Menu-only frames and BasicWing/static diagnostic paths have separate handling. At 8512–9160 the builder scans published adapted models, hashes static submission identity, rebuilds a combined static prefix on signature change, refreshes static lighting independently, appends dynamic objects and sorts range descriptors by azelOrder (8875–8925). The final signature includes polygon count and referenced texture indices; changed signature, dirty textures, texture count, readiness or missing base payload trigger prepare_vdp1_model (9080–9115). Otherwise dynamic positions are copied into mapped buffers. The combined mesh rejects >65535 vertices and unresolved material indices. Do not assume all draw paths preserve order merely because range descriptors are sorted; trace each consumer before replacement.

| Anchor | Gate / coupling | Replacement invariant to prove |
|---|---|---|
| 2652 | Flight/full/clean LiveTown texture deferral | Valid resident descriptor and generation tracking, compatible pipeline |
| 6490 | Flight-only prefix validation skip | Proven stable descriptor prefix |
| 6761 | Flight/full subdivision topology retention | Stable slot topology and capacity |
| 8768 | Flight/fieldEnvironment placement cache, matrix, non-dynamic/non-billboard, no animated color, fixed local coordinates, versioned model | Explicit transform/coordinate and geometry mutability contract; no game-mode gate |
| 12757 | Flight/full static subdivision slot reuse, no probe | Stable polygon/position association and compatible diagnostics |
| 16074 | Flight timing windows | Mode-neutral counters with optional workload tags |
| 17700 | Mode transition bumps reset epoch | Explicit resource ownership epoch; retain existing reset until proven |

Additional game-specific rendering branches at 5424, 5520, 5898, 5948, 5963, 7878, 8964, 14779, 15355 and 15946 require semantic audit; do not treat them as redundant performance gates. Circle diagnostic at 12762–12782 recognizes polygon indices and material; isolate from reusable core eventually.

## B — Identity and lifetime (initial)
7704 material cache uses model identity and generation. 8276 dynamic classification uses explicit dynamic/billboard flags plus auto-dynamic instance identity including scene mode and field identifiers. 8641 static signature includes model pointer, generation, bundle/cell/object/table offset, polygon count and matrix. 8768 placement cache also keys transformed vertices by matrix and geometry counts. These are instance identities, not a reusable local-space geometry identity. 8554 reset epoch clears placement, subdivision and transform history on render thread. 8990–9019 active texture generation validation already works beyond flight; preserve it. Producer generation creation/reuse and adapted model ownership still require bridge audit. Proposed identity: opaque resource namespace + handle + geometry revision + layout; separate instance transform, selected resource, shading and material binding revisions. Never infer immutable geometry from pointer alone.

## C — GXM (initial)
3542 probeGpuAlloc uses 4 KiB-aligned uncached user memory mapped for GXM; probeCdramAlloc uses 256 KiB rounding. 6388 releaseResidentVdp1Model unmaps/frees separate base, lighting, textured, Gouraud and subdivision streams. 6717 subdivision allocation reserves 9 vertices and 24 indices per polygon. 15985–16001 ends scene, prepares registered material, then sceGxmFinish; 16444 signals producer free semaphore after rendering. 17713 snapshot swap relies on free-slot ownership. Preserve existing finish and ownership protocol in first implementation; audit all early returns and release callers before asynchronous retirement. Measure mapped bytes and high water; choose arena memory type from hardware measurements, not assumptions. U16 indices require range-local indexing and bounded draw sizes.

## D — Candidate design, not approved implementation
Retain existing combined path as fallback. Introduce versioned geometry resource records and ordered per-frame draw ranges, initially for immutable rigid geometry with compatible transforms and shading. Keep dynamic shading and material dependencies separate. First implementation may retain transformed placement payloads rather than moving projection or Saturn-specific subdivision to shaders; local-space residency is a later capability if equivalence is proven. Preserve authoritative ordering, clipping, priority, transparency, texture flips and Gouraud interpolation. Avoid one draw per polygon; measure draw count and submission time. Do not remove mode gates until equivalent eligibility is validated.

## E — Acceptance and open work
Next: trace bridge producers and generation lifecycle, all draw range consumers, allocation/release callers, native scene mode definitions, sprite and VDP2 boundaries. Then finalize design and budget. Test Valley, Garil, Ruins, Camp, Excavation, battle, cutscenes and transitions with independent switches. Record deadline misses, renderer/geometry time, uploads, draw counts, cache hits, fallback reasons and memory high-water. Compare equivalent workload bands rather than requiring identical runs. No Disc 1 asset inspection needed for this pass. No build or hardware test performed because this commit is documentation only.
