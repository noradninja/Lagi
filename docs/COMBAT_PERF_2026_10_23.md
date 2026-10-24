# Full-run combat performance follow-up

User accepted the divide crash and repeated-tunnel audio fixes on hardware.
The next log contains two Patergo (sub 7) encounters with sampled renderer
medians 33.49/30.54 ms and complete presentation windows 19.98–23.90 / 22.55–29.97
FPS. Other ordinary encounters are approximately 29.5–29.97 FPS. These are
sampled renderer spans, not independent CPU/GPU costs or whole-frame averages.

Battle entry cold renders take 96–158 ms; RBG preparation alone takes 56–60 ms.
The decoded-page cache expands three 512x512 pages on resource invalidation.
The first correction memoizes exact repeated pattern names within each page
rebuild, copying already decoded rows rather than repeating per-dot decoding.
Dependencies are collected before reuse; name equality includes flips/palette,
and all producer-domain/generation invalidation rules remain intact. No effect,
rotation, blending, or native timing is skipped. Direct-map collisions miss
safely. Native Azel pixel-oracle and production cache-service tests pass.

The pre-boss cinematic has a sampled 34.51-ms task spike and 31.25-ms renderer
span containing a static cache rebuild. Task/render spans may overlap. This
is a separate hitch candidate, not evidence of sustained cloud GPU overload.

Hardware validation remains required for each change: battle entry fades,
both Patergo variants including Regenerate, the pre-boss cinematic, name
entry, water, and clouds. A build or pixel test is not a timing acceptance.

## Shared dynamic model positions

The adapter now retains each expanded vertex's first native position-index
occurrence. During a submission the renderer may copy XYZ from an earlier
quad already transformed under the same current matrix/projection/billboard
basis. RGBA is retained from the current face; normals, lighting and material
records are not shared. Nothing is reused across actors or frames. Missing
topology and noncanonical geometry retain the accepted path. This applies to
all native models, not Patergo IDs. The production topology adapter test and
bitwise append-versus-baseline fixture pass for repeated appends, lights,
billboards, offsets, blend ordering, face colors and fallback layouts.

## Cinematic model registration

Inventory registration previously adapted every model into a private snapshot;
the first draw then adapted the same model again into the draw cache. Fresh
registration now creates the cache snapshot once and aliases immutable model
ownership for inventory. Registration replaces old pointer identity, duplicate
registration is still a no-op, and published inventory keeps old snapshots
alive after retirement. Production-function extraction tests verify reuse,
generation/address replacement, duplicate registration and lifetime. This
removes duplicate work at resource introductions but does not prove every
cinematic task/rebuild spike is eliminated; capture the same sequence again.

## Next capture: repeated enemies and unused stream updates

The user accepts the cinematic hitch correction. The next full run still has
Patergo at 21.28–29.97 FPS (440/1320 intervals over two VBlanks) and Gelata at
24.80–29.97 (119/960). Other ordinary fights have 0–1 late intervals. Sampled
renderer medians are 30.10/29.48 ms. No exact-run A/B gain is inferred.
The smooth boss transition is user-observed, not proof all startup work is gone.
Random entry RBG preparation is now about 21–25 ms, down from 56–60 ms; total
cold render remains about 60–65 ms, with the first 120.65-ms entry also doing
78.08 ms of scene building. That first allocation includes base buffers 8.55,
textured buffers 22.36 and subdivision buffers 10.09 ms. Loading and renderer
metrics overlap; do not add all fields. Random-entry performance is still open.

Full-subdivision drawing builds positions from the current CPU model, yet
steady position sync also wrote the unused base texture and Gouraud streams.
The next change skips just those two streams when mode, lighting, textures,
shaders, capacity and buffer readiness prove subdivision is selected. A shared
readiness predicate is used by draw and update. Base color vertices remain
current. Diagnostics, missing shaders/resources, insufficient capacity and
unlit fallback still perform the original updates; fallback immediately writes
current dynamic positions. No animation, culling, enemy identity or simulation
cadence changes. ScenePerf positionSyncLegacyVerts is zero when the legacy
streams are skipped; positionSyncVerts still counts base color vertices.
Production extraction tests cover gates, untouched legacy bytes, current base
positions, fallback catch-up, static prefixes and attributes. Hardware speed
and render-mode switching remain validation gates, not host-test claims.

The generated RNG/observer tests and original Disc 1 RNG/countdown/table-data
checks were rerun successfully. These do not close the independent original
story-gate and whole-task-scheduler equivalence audit. Goal 1 remains active.

## Bounded native-scene reservation

Flight previously reserved at least 2048 quads, growing by 25 percent. The
first captured Patergo needs 2222 quads and grows the reservation to 2730;
one combined reuse check then frees/reallocates every geometry stream.
The next independent correction reserves that same existing 16-bit subdivision
index ceiling (65535/24 = 2730 quads) at first native-scene preparation. It is
not a formation-specific pool, and active draw counts remain unchanged.
It reserves 919336 extra payload bytes versus the old 2048 floor, before 4-KiB
allocation rounding, rather than allocating this capacity mid-fade in combat.
Initial allocation cost is moved to first scene preparation, not eliminated.
This does not remove the remaining RBG page rebuild, cold material resolution
or task-side native startup work; random-entry timing still needs hardware.
The production policy fixture checks every supported count and native mode,
unchanged diagnostic/unreserved behavior and unsupported-subdivision fallback
without clamping away geometry. Device memory pressure, mode switching, first
random entry, repeated enemies and returns must still be tested.

## Hardware follow-up at 4b1f473 and shared CPU lighting source

The user's 1714144-byte log has Patergo windows 27.04–29.97 FPS,
55/960 intervals late, sampled render median 28.248 ms; Gelata 29.358–29.97,
11/1200 late, median 28.188 ms. The trailing Queen span is incomplete but its
six complete windows are 29.97 FPS, zero late. Position sync is 0.562/0.506 ms
for Patergo/Gelata. Legacy vertex writes are zero in every sampled combat
frame. First entry reports reuseGeom=1 and baseAlloc/texAlloc/subAlloc=0;
cold scene build is 26.875 ms and render 70.148 ms. This confirms the removed
work, not the 25/33-ms acceptance gate, and is not an exact-scene A/B. That
build predates 191f844's story-selector correction.

Live append also maintained an identical second CPU vertex vector solely
for liveTownVdp1Source().lightingVertices. The next shared optimization aliases
that source pointer to the existing transformed vertex vector and removes
the duplicate reserve, append and static-prefix resize. Native normals,
lighting state, Gouraud payloads, materials and draw ordering remain separate.
The production-source fixture extracts both accepted/current source builders
and append bodies, compares renderer-visible lighting bytes and geometry,
and asserts the live duplicate vector stays empty. It covers noncanonical
layout, billboards, projected offsets, native light and repeated appends.
No other mesh type's lighting storage is changed. Fresh hardware timing and
visual validation are still required; this is not a claim of solved combat.
