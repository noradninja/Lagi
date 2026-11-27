# Neptune architecture and optimization direction

## Scope
Neptune is a reusable native rendering framework for Sega Saturn game ports to PlayStation Vita. Lagi is its first consumer. Portability means reuse across Saturn games on Vita, not portability across hardware. SceGxm, SGX543MP4+ and Vita-specific memory and synchronization optimizations are encouraged; a cross-platform backend abstraction is not a goal.

Azel decides; Neptune renders. Game adapters own gameplay, camera, visibility, LOD selection, animation, asset loading and game-specific policies. Neptune owns Saturn rendering semantics, VDP1/VDP2 primitives and composition, texture decoding, CRAM dependencies, geometry residency, shading and GXM resources. Eligibility for optimization must depend on rendering properties, not scene names, disc assets or gameplay modes. Garil Desert is a stress test, not a special-case target. Preserve original ordering, priority, color calculation, clipping and shading.

## Current checkpoint
1C.2 (0cd5ec020f03f50978197e9b89c54f07f7f68a3d) was hardware tested on 2026-11-27. User reports significantly improved Garil performance and unchanged Valley performance. Prior capture analysis reported approximately 0.102 ms CRAM invalidation versus 1.41–1.56 ms previously, 3633 fast paths and no fallbacks, 99.05% placement hits, and 28.39 average presentation FPS. These are observational, non-identical runs, not controlled A/B benchmarks. Existing field-only gates are transitional validation boundaries, not final architecture. Defer 1C.3 material cleanup pending profiling.

## 1D roadmap
- 1D.0: Read-only audit of game/mode dependencies, submission paths, resource identity, mutation, lifetime, ordering and synchronization. Produce a gate inventory and eligibility contract before implementation.
- 1D.1: Generalized geometry resource identity and lifetime management, separating immutable geometry, instance transforms, dynamic shading and material state.
- 1D.2: Persistent GXM geometry ranges with bounded memory, safe retirement and fallback.
- 1D.3: Submit resident ranges in authoritative order without rebuilding combined meshes when visibility changes.
- 1D.4: Generalize placement, subdivision and texture optimizations by capability, preserving legacy fallback until validated.
- 1D.5: Validate town, flight, battle and in-engine cutscenes, including transitions, palette animation, LOD, dynamic lighting and memory pressure.

## Audit acceptance
Identify every relevant gate and its safety rationale; distinguish game policy from renderer invariants. Map resource producers and consumers, stable identity and generation changes, VDP1/CRAM writes, SPCTL changes, scene teardown, in-flight GPU ownership and buffer reuse. Define eligibility for rigid meshes, rigid animated hierarchy parts, deforming geometry, billboards and transient primitives. Preserve Azel submission decisions and Saturn compositing semantics. Record baseline timing, deadline misses, rebuild causes, cache hits, bytes uploaded and memory high-water marks. Use independent switches and atomic commits; do not remove existing safety gates before replacement invariants and hardware tests exist. No renderer implementation changes are authorized by the audit alone.

## Naming and ownership convention
Use neptune::, include/neptune/ and NEPTUNE_ for genuinely reusable rendering systems; retain lagi::, LAGI_ and azel_bridge:: for game/application integration. Keep Saturn hardware terminology and Vita/GXM implementation names. Rename misleading town/field terminology incrementally when touching a shared subsystem, in mechanical commits separate from behavior changes. Do not disguise game-specific policy with generic names or remove guards simply to rename them. Preserve compatibility aliases for renamed public build flags and update tests/diagnostics/documents. Existing shared buildLiveTownFrame, LiveTown residency and field cache naming are audit candidates, not blanket rename authorization.

## 1D.1A checkpoint
065fa49 adds include/neptune/geometry_resource_key.h; e993c6e adds a standalone key test. Resource identity consists of nonzero producer namespace, opaque resource ID and revision. The representation key adds versioned layout and coordinate convention, local/transformed space, exact fixed-point transform for transformed payloads, and a baked-attribute dependency revision. Local keys require a canonical zero transform. Full equality, not hash alone, controls reuse. Zero/invalid keys must fall back; key validity alone does not prove eligibility. Producer must supply revisions for every baked dependency and not reuse namespace/resource/revision for different content. No raw pointer, scene mode, bundle, cell, object or game identifier is required. Owner retirement, namespace allocation, explicit dependency version production and bounded GPU allocations are later steps; no runtime consumers yet. Host test can be compiled with C++17 or newer (MSVC developer shell: cl /nologo /std:c++17 /EHsc /Iinclude tests/neptune_geometry_resource_key_test.cpp). A local equivalent source copy passed GCC C++17 with warnings as errors; no Vita build or hardware run.
