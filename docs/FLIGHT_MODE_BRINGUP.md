# Flight Mode Bring-Up

## Native collection color recovery in progress (2026-10-10)

New work branch: feature/native-point-lights-effects, based on merged main.
Point lighting and grayscale image particles/orbs remain separate open goals.
Original Disc 1 FLD_A3.PRG (logical sector 244544, 268888 bytes) confirms
0607AC3C passes the trail Gouraud pointer plus animationFrame*4 to shared helper
0602D0DC. That helper copies four big-endian words at offsets 0/2/4/6 into the
Gouraud table. Thus trail entries overlap: a four-byte stride selects an
eight-byte window, not a fixed block or a synthesized item tint.
Original orb draw at 0607AD44 selects the separate inline table at 06094F64
with colorIndex*8 and calls the same helper. The reconstructed calls omit
these arguments. Local generated-source adapters restore both contracts using
a generic Lagi color-table service and existing Gouraud sprite output.
extern/Azel is unchanged; no gameplay color selection or projection change.
Configuration and Vita syntax checks pass after correcting the adapter header's
type include. The actual adapter passes 384 host color-window cases across
strides 0/4/8, including endian order, argument forwarding and null handling.
Memory access and the downstream draw are controlled test doubles; this does
not validate GXM output or native task/color selection. Hardware validation is
still pending; this is not a completed color fix. Other image emitters and point-light
activation/evaluation/reset semantics still need reconstruction.

## User acceptance and main merge authorization (2026-10-10)

The user explicitly considers this rendering goal met for the current milestone:
the two remaining measured spikes are not noticeable enough to prioritize now.
They authorize merging feature/vdp2-perf-diagnostics to main and updating README
and internal documentation. Preserve this acceptance separately from strict
deadline proof: latest tested capture is 6f74bf2, with 39.078/41.906 ms cold
frames and a sampled 23.887 ms median. Final runtime dd579eb is host/syntax
checked but has no separate hardware capture. No final merged package build or
hardware validation is claimed. The E006/Excavation floor remains unresolved;
audio PR #11 stays separate. This acceptance supersedes the earlier instruction
to continue eliminating all measured cold spikes before closing this milestone.

## Captured CRAM dependency checkpoint (2026-10-10)

User reports no visual or performance regression for range-aware invalidation.
Latest capture is 913219 bytes, 20:45 local: sampled native RBG median
23.887 ms/p90 25.163 ms. Completed field windows cover 3960 frames with
513 over 25 ms and two over 33.333 ms (39.078/41.906 ms), without baseline
presentation intervals for those outliers. Completed presentation windows have
zero >2-vblank intervals. The previous 245-texture refresh burst is absent,
but routes/windows are unmatched, so do not claim a controlled speedup.
The remaining transition refreshes 109 textures in 2505 us and resolves
materials in 9007 us. Full deadline compliance remains unproven.

The next shared refinement captures actual CRAM reads in the native decoder:
four 32-bit words identify 128 palette blocks of 32 bytes (16 bytes/texture).
This covers arbitrary LUT palette indices, zero palette colors and eager
palette resolution. Direct RGB555 LUT entries never read CRAM and therefore
do not become stale for unrelated palette writes. Pixel/LUT range checking,
unknown-producer fallback, empty-notification fallback, CPU/GPU epoch ownership
and previously stale entries remain unchanged. No scene-dependent branch or
upstream edit is introduced. Re-decoding replaces dependency metadata too.

Dependency tests and 5184 actual-decoder differential/opacity/snapshot cases
pass. Additional representative actual decodes mutate all 128 CRAM blocks:
every observed pixel change must be covered by the original dependency mask.
Vita syntax passes; no package build or hardware validation for this refinement.
Compare refreshed counts/time on the same route and inspect palette-driven
dragon/item/save effects. New material decoding still remains a separate cold
cost; this checkpoint does not claim to solve initial-entry deadlines or floor
sampling.

## Shared range-aware invalidation checkpoint (2026-10-10)

Latest diagnostic capture: 833969 bytes, 20:31 local. Sampled RBG-enabled
native median 23.666 ms/p90 25.045 ms; 6240 completed field frames include
427 over 25 ms and three over 33.333 ms (38.904/36.750/42.786 ms). Full goal
remains unproven. The 36.750 ms frame refreshes 245 existing textures in
8189 us and uploads them in 3539 us without rebuilding static geometry.
The 42.786 ms frame spends 8829 us on materials and 2389 us refreshing
109 existing textures. This establishes refresh decoding as significant work.

Both invalidation callbacks previously discarded supplied write ranges and
advanced every active texture to a stale global epoch. Shared dependency logic
now publishes bounded atomic dirty blocks (512-byte VDP1, 32-byte CRAM), consumed
on the renderer thread. VDP1 offsets and absolute 0x25C00000 addresses normalize
to the same range; wrap is conservatively covered. Invalid/unknown/zero-size
notifications preserve full invalidation. No allocations or renderer-container
mutation occur in the producer callbacks. Concurrent notifications can cause
extra invalidation, never intentional exclusion of missing range information.

Native decoded textures track image ranges and mode-1 LUT ranges. Banked modes
track their entire CRAM bank; RGB555 textures and native flat colors have no
CRAM dependency. Mode-1 LUT materials conservatively depend on all CRAM, since
LUT entries can reference arbitrary palette colors. Unknown texture producers
keep legacy full invalidation. Only previously current, unaffected CPU/GPU
epochs are promoted; already stale entries remain stale until used. Descriptor
cache clearing and native resource write notifications remain conservative and
unchanged. This is a generic native-material policy, not a scene override.

Host dependency tests cover offsets/absolute addresses, wrap, boundary bytes,
concurrent producers, image/LUT/bank overlap and conservative fallbacks.
5184 actual-decoder pixel/opacity/snapshot cases and three flat-material cases
pass; Vita syntax check passes. No package build or hardware validation yet.
Test the same route, compare refreshed counts/times and cold-frame tails, and
inspect dragon morphing, collection sprites, save particles and palette-driven
effects for stale pixels/colors. Floor/map sampling is unchanged and unresolved.

## Opacity hardware observation and refresh profiling (2026-10-10)

Latest Downloads capture is 1295660 bytes, 20:20 local. Initial 227-texture
upload: opacity 161 us, copy 2095 us, setup 258 us, total 3415 us, allocation
zero. Previous capture measured opacity 3168 us and total 6357 us. Sampled
RBG-enabled native median is 23.774 ms/p90 25.609 ms; completed field windows
cover 6240 frames, with 648 over 25 ms and three over 33.333 ms. These are
unmatched route observations, not controlled A/B proof; log does not embed SHA.
Outliers remain 38.829/35.485/45.745 ms, all without presentation baselines.
Completed presentation windows contain zero >2-vblank intervals (6120 samples).
Do not declare the full frame deadline achieved.

The 35.485 ms frame retains static geometry but refreshes/uploads 245 textures
(517344 bytes). FieldStream build is 15901 us, material lookup 521 us, upload
5348 us; refresh decoding was hidden in validation. The 45.745 ms frame resolves
new materials (12474 us) and uploads 246 textures, including refreshed entries.
Next diagnostic adds validate/refresh/refreshed to existing FieldStream lines,
using two block-level timer reads rather than per-pixel timing. No change to
texture invalidation, refresh ownership, upload selection or scene behavior.
This distinguishes new-material work from existing-material epoch refreshes
before attempting a shared invalidation/residency optimization.

## Shared decoded-opacity checkpoint (2026-10-10)

The initial field upload in the latest hardware log spends 3168 us classifying
opacity separately from copying pixels. Native decoding now carries opacity
metadata alongside the decoded image: written-pixel count detects transparent
skips/end-code row tails, and combined alpha bits classify produced pixels.
Upload uses this classification when known; other texture producers retain the
existing full scan. Refresh decoding replaces pixels and metadata together.
No scene ID, map selection, gameplay, visibility, or upstream Azel change is used.
This removes an upload scan, not a proven net performance improvement: additional
decoder bookkeeping must be measured on the user's hardware against 2524c88.
Compare material/decode time plus upload opacity/total time and full frame tails.
The floor investigation remains unresolved and independent of this change.
Host differential tests cover both end-code modes, all supported color modes,
transparent skips, palette/LUT colors, snapshot ownership and opacity versus a
full pixel scan. Vita syntax validation passes; no package or hardware test run.

## Smooth-play checkpoint and Excavation floor investigation (2026-10-10)

User reports smooth play after decoder checkpoint 2524c88. Latest capture is
977607 bytes, 19:01 local: RBG-enabled sampled native render median 23.886 ms,
p90 25.305 ms. Completed field windows cover 3840 frames, 509 over 25 ms and
two over 33.333 ms; baseline-reset entry outliers remain 41.312/44.957 ms.
There are no >2-vblank intervals in completed presentation windows, but entry
outlier intervals lack a baseline. Do not declare the full deadline goal achieved.
VRAM arena ready=1/cdram=1, startup 4966 us; all recorded upload allocations zero.
Initial 227-texture upload takes 6357 us. These are route observations, not a
controlled A/B proof of decoder speedup or validation of every scene.

User additionally requests investigation of incorrect VDP2 floor in Excavation
and E006 cinematic, visible as broken horizontal texture patches through wires.
Both invoke startExcaBackgroundTask. Native configuration is CHCN=1 (256 colors),
CHSZ=1 (16x16), PNB=1 (one-word names), CNSM=0, SCN=8, RPMD=2. The shader's
unimplemented two-word pattern branch is therefore NOT the established cause.
Sky character data EXCA_SCR.SCB loads at 0x40000, names at 0x60000; native setup
sets parameter-B map planes at 0x60000. The shared initializer does not call
setupRotationMapPlanes for A. Captured mode-1/2 state shows mapA=00000,
mapB=60000. It also fills a 0x5000-word table starting at 0x60800.

This is a lead, not proof that A should point to 0x60800: zero is a legal VDP2
map base, and source/log inspection alone does not establish intended ground
map contents or original overlay setup. Do not hardcode an Excavation renderer
override or alter upstream Azel on this evidence. Original Disc 1 BIN/CUE is now
available in Downloads and has been inspected read-only; no game data is copied
into the repository. TWN_EXCA.PRG is at logical sector 235981 (75640 bytes).
Its native tables at 0x06064B94 and 0x06064BD4 contain sixteen pointers each to
0x25E60000 and 0x25E60800 respectively. The native initialization call passes
index 1 and the first table to setupRotationMapPlanes. Inspection of the original
shared helper confirms index 0 writes A and index 1 writes B; it does not
automatically consume the adjacent table. Therefore changing that helper to
configure both maps would not preserve the original program. The second table's
activation remains unproven; continue tracing native state before choosing a fix.
The user explicitly requires general solutions wherever possible: no scene-ID
override, forced map address, or altered visibility/gameplay decision. Any eventual
correction should address shared state adaptation or sampling semantics, with a
scene adapter only if original behavior actually requires it.
No floor correction implemented; camera/actor scopes,
flight orientation, shader, VRAM arena and decoder remain unchanged.

## VRAM arena hardware acceptance and decode checkpoint (2026-10-10)

Capture 1486225 bytes, 18:49 local, corrected-arena build: ready=1/cdram=1,
8388608-byte reservation took 5031 us at GXM startup. All 100 recorded native
texture-upload events show zero allocation time. This proves arena use on this
route, not zero allocations for arbitrarily larger scenes. Initial field upload
227 textures/512960 bytes now takes 6591 us (opacity 3218, copy 2091, setup 265),
with material resolution 11833 us. Field baseline-reset spikes remain
43.441/49.019 ms. RBG-enabled native sampled median 24.144 ms/p90 25.327 ms.
Completed field windows cover 7200 frames, 884 over 25 ms and two over 33.333 ms;
no completed-window presentation interval exceeds two vblanks. Baseline-reset
outlier intervals are still missing, so full deadline compliance is not proven.

User identified a separate periodic five-second stutter in capture software;
turning that software off removed it. Do not attribute that symptom to logging
or the native renderer. Native cold resource work remains visible in CPU timing.

Next narrow decode checkpoint: mode-0/1 images with at least 64 pixels resolve
all sixteen palette entries once, then index them directly rather than performing
the lazy cache validity check per texel. Smaller images keep lazy resolution.
Mode-1 eager resolution requires a complete in-range 32-byte LUT; unusual native
boundary LUTs retain the previous lazy behavior. CRAM/LUT transparency rules,
native snapshot ownership and palette lifetime remain unchanged. All supported
decoders stop scanning the remainder of a row after its second end marker when
the existing end-mode rule applies; pre-zeroed output preserves transparency.
2592 actual-decoder differential/snapshot cases and Vita syntax checks pass.
No native package build or hardware performance evidence for this decoder change.
Compare cold material timings and complete frame tails; do not infer a speedup
from desktop tests. Logging, geometry, arena and native gameplay are unchanged.

## Arena initialization-order correction (2026-10-10)

Latest capture: 1203845 bytes, 18:37 local. Arena ready=0/cdram=0, startup
100675 us; no arena performance benefit was exercised. Source inspection found
the reservation in renderer init(), BEFORE show_game_presentation initializes
GXM. Both allocator helpers require sceGxmMapMemory; a pre-initialization
failure is not evidence that the 128 MiB VRAM pool is exhausted. Move optional
reservation after essential GXM/backend allocation success and before native
presentation readiness. Capacity/fallback/release behavior remains unchanged.
Check ready=1 and cdram on hardware before interpreting allocation improvements.

Capture median 23.9875 ms (sampled RBG-enabled native scenes), p90 25.026 ms;
field baseline-reset spikes 56.306/64.445 ms. Completed field windows: 3600
frames, 338 over 25 ms, two over 33.333 ms. Full goal remains unmet.

Logging is async=1 in this capture. Producers still perform vsnprintf, semaphore
locking, queue copies and wake signaling; the worker performs storage writes
and stdout mirroring. Capture includes 6695 PresentationTrace messages. No
on/off measurement exists, so logging overhead cannot be quantified or blamed
for the measured resource spikes. Keep logging reduction as a separate matched
experiment rather than mixing it with this initialization correction.

## Hardware preparation results and startup texture arena (2026-10-10)

Capture: Downloads/lagi.log, 783808 bytes, 18:25 local time, user testing c9b1f92.
RBG-enabled sampled native render median 24.089 ms, p90 25.637 ms. Completed
field windows cover 3960 frames: 462 over 25 ms, two over 33.333 ms. Field
baseline-reset outliers remain 55.824/61.080 ms; missing baseline presentation
intervals do not prove those entries displayed on time. Separate mode-1 initial
sample is 77.483 ms. Median bound holds only for completed field windows.

79 preparation events averaged 107.9 us decode and 453.6 us upload, maxima
944/747 us. Initial field still decodes 227 textures and uploads 512960 bytes
in 19.033 ms (13.027 ms allocation). Later field load discovers 137 new textures
and refreshes/uploads 246 textures in 20.635 ms (13.051 ms allocation). The
registrations publish immediately before visible entry; native load/cell setup
can complete within the same game update. Current warming therefore has no
advance opportunity for first-visible resources. No isolated speedup claimed.

User confirms 128 MiB total VRAM; do not treat all of it as spare texture budget.
Existing render targets and other CDRAM resources retain their allocation paths.
Next checkpoint reserves an optional 8 MiB mapped native texture slab during
renderer initialization, before starting the render thread. This covers this
capture's pooled high-water allocation but is not a universal capacity claim.
Scene texture release resets that slab's used cursor without releasing it;
ordinary overflow slabs retain their existing release behavior. Shutdown frees
the reserved slab too. Existing mapped texture prefixes remain untouched during
geometry-only prepares. Uploads use the same aligned first-fit allocator and
grow normally if capacity is exceeded; failed startup reservation falls back
to the existing allocator rather than failing initialization. Reservation prefers
USER_CDRAM_RW (dedicated VRAM), then GPU-mapped USER_RW_UNCACHE system RAM.
Overflow allocations remain system RAM. The previous slab allocator did not use
dedicated VRAM; this change does not migrate every renderer allocation to CDRAM.

Tradeoff: 8 MiB remains reserved across menus/movies/towns until shutdown, with
allocation latency moved to startup. This is allocation residency, NOT persistent
per-model geometry or earlier decoding. Cold decode/resource CPU work still
exceeds the target and remains next work. NativeTextureArena logs bytes/ready/
startupUs; compare native upload allocation stages and complete frame tails on
hardware; cdram identifies which heap succeeded. Native syntax/diff checks pass.
No package build or hardware validation of the arena checkpoint. CPU upload and
GPU sampling behavior in CDRAM must be measured, not assumed faster.

Host lifecycle regression test native_texture_arena_lifecycle_test.ps1 extracts
the actual freeVdp1Textures implementation and mocks only platform memory calls.
It passes scene reset/reuse, ordinary owned texture and overflow cleanup, arena
identity/capacity preservation, repeated releases, shutdown and double-shutdown
checks. This validates release control flow, not real GXM synchronization,
allocation availability, VRAM access performance or native package behavior.

## Avoid geometry rebuilds for prepared offscreen materials (2026-10-10)

Follow-up to the preparation consumer: the live frame signature now hashes
referenced polygon material indices instead of the complete decoded atlas size.
Preparing an unused texture and successfully uploading its suffix no longer
invalidates visible geometry merely because the cache grew. Active binding
changes still invalidate the signature, including equal-count replacement.
Dirty texture data or CPU/GPU texture-count mismatch explicitly requests prepare,
so native invalidation and failed/incomplete speculative uploads retain their
existing repair path. This does not change visibility, camera transforms,
geometry ownership or polygon topology. Native syntax validation only; hardware
timing benefit remains unproven. Test this follow-up together with preparation.

## Registered material preparation consumer (2026-10-10)

User confirms the latest actor correction is correct in Excavation and the
in-engine cinematic. Preserve the native camera scopes; rendering performance
is the active priority again. Latest hardware capture still contains 56.627 and
60.354 ms baseline-reset field frames despite completed-window medians meeting
25 ms. First-field texture allocation remains about 13.338 ms. Target is not met.

Neptune now consumes published registrations without creating draw submissions
or changing Azel visibility. After EndScene, it scans immutable registered model
descriptors and decodes at most one new material into the existing generic
native material cache. Cache-hit scanning stops after 1 ms; a single decode is
non-preemptible, so this is NOT a guaranteed 1 ms deadline. Decoder pixel/LUT
reads use the owned 512 KiB snapshot, while CRAM stays renderer-frame-owned.
Inventory revision or native texture generation resets the cursor. Stale write
epochs stop work; a write observed during decode discards newly appended data.

CPU decoding can overlap GPU completion. Appended GPU texture upload happens
only AFTER sceGxmFinish, preserving existing prefix payloads and descriptors.
The upload/allocation itself is not hidden by the GPU wait and may exceed the
frame deadline. NativeResourcePrepare reports decodeUs/uploadUs/textures;
GxmWait and Render include this work, while GxmFinish measures only the finish
call. Do not add these overlapping durations. Resource cache growth can trigger
the existing geometry prepare path on the next frame. This is incremental
texture preparation, NOT persistent per-model GPU geometry or a hitch fix.
Current registrations originate in native field adapters; the consumer accepts
native scene modes 1..3, but generic town registration remains future work.

Vita renderer syntax check and 2592 actual-decoder differential cases pass;
snapshot-isolation tests also compare decoding after live pixel/LUT mutation.
No package build or hardware validation. Test the exact new commit across entry,
traversal, Excavation and cinematic, checking total frame tails as well as
NativeResourcePrepare and NativeTextureUpload stages. One-at-a-time warming may
increase early-frame resource work; report a regression rather than assuming
preparation is a performance improvement. First visible materials may still
arrive before any preparation opportunity.

## Registration snapshot invalidation protocol (2026-10-10)

VDP1 texture-write callbacks now advance a bridge-owned atomic 32-bit epoch,
alongside existing renderer invalidation flags. Registration records retain
the epoch associated with their texture snapshot. At game-side publish_frame,
stale registrations refresh from one shared immutable 512 KiB VDP1 snapshot;
the new bytes/epoch are attached to current resources and slot owners. Existing
published ownership is not changed before the frame-slot boundary. Inventory
publication also advances a revision for future consumer cursor resets.

This extends snapshots to later native texture mutations, rather than assuming
loaded pixels never change. CRAM remains the renderer's frame-owned palette.
A future consumer must compare the resource epoch against the current write
epoch and skip work if another write occurred after publication; it must not
read live VDP1 bytes to continue a stale inventory job. Epoch wrap is the same
32-bit counter limitation as existing renderer invalidation epochs; no unlimited
session uniqueness claim is made. These hooks do not make concurrent arbitrary
raw-memory writers safe; native texture mutation remains game-owned.

Native syntax checks pass for bridge and renderer; diff checks pass. No native
package or hardware test performed. Decoder/preparation consumption remains
unconnected, so this is still an intermediate ownership checkpoint with added
snapshot/scan overhead, not a performance milestone. Next integration should
measure preparation during the GPU completion interval, without rewriting
in-flight retained texture payloads or changing current-frame submissions.

## Registered texture-memory ownership (2026-10-10)

Resource registrations now retain an immutable 512 KiB VDP1 address-space
snapshot captured on the game thread after bundle/character loading completes.
All models registered for that slot/load share the same snapshot; it is not
copied per model or per frame. Replacing a slot drops producer ownership, while
published/shared references preserve the prior generation's bytes. Missing
native memory prevents registration rather than publishing a descriptor whose
texture payload is unavailable. CRAM/palette remains frame-owned, not baked
into this snapshot, and must be resolved with the consumer's published frame.

This closes load-generation ownership for future deferred preparation; it does
not establish a policy for subsequent animated/runtime VDP1 writes. A consumer
must continue honoring invalidation/current content and must not reuse an old
snapshot as authoritative after a later same-generation texture mutation.
Native syntax check and diff checks pass. No decoder consumer, GPU uploads,
package build or hardware validation in this step. Snapshot memory/copy cost
is additional intermediate overhead; performance improvement remains unproven.

## Generation-tagged registration channel foundation (2026-10-10)

Implemented generic RegisteredModelResource publication independently of
RenderSubmission. Native field load completion advances the loaded slot's
generation after MCB/CGB calls finish; replacement retires registrations for
that slot. Grid setup registers visual model offsets 0..3, not collision offset
4, without calling cell draw tasks or changing pause/visibility state. Repeated
offsets deduplicate within the loaded slot. Adapted descriptors own their vertex,
polygon, lighting and Gouraud vectors and publish as shared_ptr<const> resources.
Old published snapshots stay alive until the existing frame-slot wait permits
publish_frame() to replace them. Registration survives begin_frame(); publication
changes only at the established frame boundary when inventory is dirty.

NativeResourceInventory reports generation and registered model count. The
API is scene-generic, with field grid/load adapters as the initial producer;
town producers and Neptune consumption are not implemented yet. No texture
preparation or GPU residency benefit is claimed at this intermediate checkpoint.
It adds earlier CPU adaptation and memory cost until its consumer is connected.
Existing draw-model cache/lifetime behavior is not retroactively repaired by
this independent registry. Do not use raw pointer identity as its GPU cache key;
consume bundle generation, slot and authored model offset together.

CMake generation and Vita syntax-only checks passed for the render bridge,
generated field.cpp and generated field_a3/o_fld_a3.cpp using configured native
compiler flags. No package build or hardware validation performed. Next step
is renderer-owned bounded preparation from this inventory, with native texture
readiness/generation invalidation retained and first-use costs still measured.
This is not a hardware-test milestone or completion of the residency design.

## 56b98a9 hardware acceptance and performance continuation (2026-10-10)

User confirms actor positions are now correct in both E006 in-engine cinematic
and Excavation after testing the early-actor view correction. This validates
placement in those two scenes, not every scene/camera mode. Preserve the scoped
native-view correction and the separate hardware-confirmed flight orientation.
User requests returning to the full performance goal.

Downloads log: 929,062 bytes, 22:00 UTC. Native RBG-enabled render samples:
94 records, median 24.049 ms, p90 25.322 ms. 31 completed field windows cover
3,720 frames; 678 above 25 ms, two above 33.333 ms. Rebuild outliers render/build
56.627/35.751 and 60.354/40.158 ms. Median bound holds within counted windows;
deadline compliance does not. Reset-baseline outliers remain outside completed
presentation interval counts. First field suffix upload: 227 textures,
512,960 bytes, total 19.247 ms, allocation 13.338 ms, opacity 3.045 ms,
copy 1.708 ms, setup 0.255 ms. No isolated performance win is established for
slab-tail reuse or grouped opacity scanning. Renderer reports optimized=1.

Source audit also confirms probeGpuAlloc does not CPU-clear payloads: its cost
is kernel allocation/base lookup and GXM mapping, not a redundant memset that
can simply be deleted. Retain correct memory initialization/upload semantics.
Avoid further arbitrary slab-size tweaks as a substitute for the generic
resource-inventory/residency milestone documented below. Registration must
be separate from visibility submission, scene/bundle-generation-aware,
published at the existing frame boundary, and prepared on Neptune's owning
thread before first visibility. Truly new dynamic resources retain a measured
first-use path; no hidden-cell draw execution or synthetic camera is authorized.

## Early town actor camera-space correction (2026-10-10)

Fresh log: 756,860 bytes, 21:49 UTC. TownDragon and ExcaNPC draw with
currentValid=1 but viewValid=0. TownDragon's boundary matrix equals the retained
camera matrix numerically; NPC has the same basis plus its translated origin.
E006Dragon also reports viewValid=0 with the native axis-swapped camera root.
Invalid view storage is diagnostic only, not an authoritative published view.
User confirms the placement problem affects the in-engine cinematic as well.

The bridge resets native-view validity at begin_frame. These early actor draw
calls can precede the camera task's capture, so generic submission capture
previously copied view*model as if it were world space. Generated TownDragon,
E006Dragon and ExcaNPC draw hooks now use the existing view-relative scope,
capturing the stack root before local transforms (NPC before push/translation).
The scope also seeds the frame's native presentation view when absent; a later
native camera capture can replace it. This prevents dropping the removed camera
when no later camera task publishes one. No synthetic camera, position offsets,
flight X/winding changes, or upstream edits. Draw expressions execute once.

CMake generation succeeds with installed PSP2CGC; generated scope begin/end
and original draw expressions were inspected in all three output files.
Diff checks pass. Native compilation and user hardware acceptance remain
pending. Retest Excavation dragon/Captain, E006 cinematic and working Ruins/
flight paths. This is a source-supported correction, not hardware validation.
Performance remains incomplete: capture median sampled native RBG render
24.0535 ms; completed field rebuild outliers 55.562 and 59.151 ms.

## Excavation-only actor placement investigation (2026-10-10)

User screenshot shows dragon/Captain misplaced relative to Excavation world
geometry and clarifies this occurs only in Excavation. This is the town path,
not a reason to alter the hardware-confirmed native flight X/winding rule.
Latest log (796,740 bytes, 21:36 UTC) reports TownDragon=62 and ExcaNPC=17
submissions per bounded actor diagnostic. This proves submission, not correct
world placement, model contents or camera-space conversion. Native render
samples median 24.407 ms; completed field windows still contain 56.298 and
59.571 ms rebuild outliers. Performance completion remains contradicted.

Source: town dragon builds its local model matrix then multiplies the current
matrix; ExcaNPC translates/rotates the current matrix. The generic bridge
removes the captured native view, while Edge additionally uses an explicit
view-relative scope. A new bounded NativeActorMatrix trace prints the actual
actor-boundary and captured-view 3x4 fixed-point matrices before the existing
draw expression, plus validity/scope flags. NPC boundary already includes its
local transform; do not interpret it as a bare camera. Existing four-report
budgets remain, expressions still execute exactly once, and no transforms,
visibility or models are changed. Compare these against static town placement
before choosing a correction. No upstream Azel edits. Source diff checks pass;
native build and hardware diagnosis remain pending.

## Grouped opacity scan and latest capture (2026-10-10)

Downloads changed to 874,424 bytes, 19:50 UTC; tested binary SHA awaits user
confirmation, so do not attribute changes to slab reuse yet. 111 sampled
RBG-enabled native records: render median 24.006 ms, p90 25.360 ms. Completed
field windows cover 4,800 frames, 692 above 25 ms, two above 33.333 ms; rebuild
outliers are 55.301 and 60.761 ms. First field suffix upload remains 18.450 ms:
allocation 12.078 ms, opacity 3.479 ms, copy 1.762 ms, setup 0.254 ms.
This contradicts full deadline compliance regardless of package identity.

Upload opacity classification now checks eight packed pixels per branch by
AND-reducing bit 31, with a bounded scalar tail. This is exactly the existing
alpha>=128 rule, including empty spans; no decoder, palette, pixel copy,
Gouraud or blend semantics change. Actual shared helper passed 8,487,168 host
comparisons against std::all_of, covering every alpha value at every position
for lengths 0..257. This verifies classification, not ARM code generation,
GXM behavior or performance. Native compilation/hardware testing remains
pending. Preserve stage timings to measure rather than assume a speedup.

## Image-particle clarification and slab reuse checkpoint (2026-10-10)

User clarifies that save-station dot particles are correctly cyan. Grayscale
affects image-based collection sparkle/orbs and image particles emitted by
destructible objects from item boxes/save station. Do not describe all save
particles as grayscale or treat this as a universal missing particle tint.
The color issue remains open; no color behavior changes in this checkpoint.

New Downloads capture: 678,698 bytes, 19:35 UTC, contains upload-stage counters.
96 RBG-enabled native-scene samples: render median 23.777 ms, p90 24.976 ms.
35 complete field windows: 4,200 frames, 333 above 25 ms, two above 33.333 ms;
outliers 56.261 and 61.007 ms. Their presentation baselines are reset, so zero
>2-vblank intervals in completed windows cannot establish hitch-free behavior.
First field suffix upload: 227 textures, 512,960 padded bytes, total 19.351 ms,
allocation 13.027 ms, opacity 3.436 ms, copy 1.689 ms, setup 0.255 ms.
Allocation is the largest measured upload component in that event.

Source found batch packing abandoning free tails: when an entire suffix did
not fit the latest slab, all new textures went into a new slab, even if older
slabs could fit individual textures. Native town/field uploads now first-fit
each 64-byte-aligned texture into existing slab tails before allocating a new
slab. Existing payloads do not move; texture pointers, filtering, invalidation
generations and Azel visibility are unchanged. Growth still uses the remaining
suffix byte requirement with a 1 MiB minimum. This reduces avoidable allocation
and wasted capacity but does not implement resource prewarming or guarantee
deadline compliance when new GPU storage genuinely is required.

Actual shared selector compiled/executed in the host C++ test: empty pool,
older-tail reuse, exact fits, exhausted tails, bounds and repeated allocations
pass. Source diff checks pass. Native renderer compilation and Vita validation
remain pending; no performance win is claimed. Fresh capture must include
FieldTexturePool/NativeTextureUploadStages plus full rebuild timing.

## Upload-stage diagnostic checkpoint (2026-10-10)

NativeTextureUploadStages now separates GPU allocation, opacity scan, pixel
copy and texture setup/bookkeeping for uploads in native towns/fields. It
reports uploaded count, padded copied bytes and inclusive total time. The
total also includes descriptor validation, suffix sizing, logging and timing
overhead; it is not the sum of the four stage counters. Retained unchanged
textures continue to skip uploads. No visibility, texture bytes, filter or
residency semantics changed. Failed uploads retain existing error handling
and do not produce a successful stage summary.

Use this beside FieldTextureUpload and the rebuild outlier record to determine
whether the remaining suffix-upload cost is allocation, opacity scanning or
memory copy before choosing the next optimization. Timers run only during
resource uploads, not ordinary retained-texture frames. Source diff checks
passed; this checkpoint has not been compiled or hardware-tested yet.

## c075a81 collection/save capture (2026-10-10)

User reports collecting Stolarium and the field map and activating the save
station while testing c075a81bf54feef589f436ed2382228f8b45a1bb. Their expected
visual reference is yellow Stolarium and blue field-map/save effects (colors
recalled tentatively). Preserve authored native color selection, not an
item-name-specific renderer tint. Downloads log is 377,070 bytes, timestamp
19:20 UTC; binary identity comes from the user's test report.

62 sampled RBG-enabled native-scene records: render median 23.698 ms, p90
24.536 ms; build median 2.9505 ms, GXM finish median 13.9005 ms. These samples
are not field-only. 23 complete field windows cover 2,760 frames: 148 above
25 ms and one above 33.333 ms, maximum 55.990 ms. The completed-window median
is bounded at <=25 ms, but the deadline is not met on every frame. All 2,760
counted presentation intervals remain at two vblanks; the rebuild outlier
is at a reset baseline and is excluded from those interval counts. Do not
use that exclusion as evidence of hitch-free presentation.

The outlier records build=35.417 ms, CPU prep=42.699 ms and GXM finish=11.980
ms. CPU prep includes other stages and must not be summed with build. This
continues to point toward first-use CPU/resource work rather than an unusually
long completion wait; earlier suffix-upload evidence remains the next lead.

Color diagnostics are inconclusive for the requested save/collection route:
the eight renderer samples are ray sources 3998/39F8, while the eight shaded
particle emissions contain E280 from the homing path. They do not constitute
an emitter-to-renderer match for D325 save particles. First-eight budgets were
consumed before the desired effects; future diagnostics must select distinct
source/color signatures or explicitly target those effects. Source still
shows the type-selected trail pointer unused in sTrailParticle::Draw and
LaserTrailDraw supplying no color argument. Native table layout/animation
must be verified before wiring it. No color fix or hitch-free hardware
validation is claimed for this logging-only checkpoint.

## e5cd6df hardware effects result and grayscale investigation (2026-10-10)

User confirms collection orbs/particles, save-station effects and spawned
world particles such as destruction smoke now work, but appear grayscale.
This validates restored visibility for those effects, not their color,
occlusion, lighting or complete sprite coverage. Tested version is
e5cd6dfd05c49ebb4a6a8829096ef03fa547556b. New log is 880,802 bytes,
timestamp 19:08 UTC. NativeParticle sample now reports half=17,13 instead of
half=1,1, consistent with corrected integer-pixel projection.

RBG-enabled native-scene samples: render median 23.941 ms, p90 24.938 ms,
build median 3.046 ms, finish median 14.113 ms. 36 completed field windows
cover 4,320 frames, 288 over 25 ms and two over 33.333 ms; maximum 59.344 ms.
Outliers render/build are 55.972/35.321 and 59.344/39.181 ms. Different
route/sample coverage and combined changes prevent isolated attribution.
First-use hitches remain; the full performance goal is not achieved.
User additionally reports 30 FPS throughout ordinary traversal with perceptible
initial-load spikes. Preserve this qualitative hardware result alongside the
measured outliers; neither steady 30 FPS nor completed presentation windows
establish hitch-free first-use behavior.

Color audit: SavePointParticleTask explicitly supplies 0xD325 at all corners;
LaserHomingDraw supplies 0xE280. The adapter writes supplied quadColor into
the transient Gouraud table, runtime capture snapshots it, and the compositor
has a textured Gouraud path. In contrast, sTrailParticle stores a type-selected
m1C_gouraudData pointer but its Draw never consumes it; LaserTrailDraw also
omits a color argument. Do not infer a single missing global tint switch or
invent item colors. Stored table format/animation semantics need verification
before generated-source wiring for these paths.

Bounded diagnostics now distinguish shaded/unshaded particle emissions
(eight each) with table index and four color words. NativeSpriteColor logs
eight field Gouraud commands with captured words and whether the shaded
program is selected. This changes logging only, not sprite color behavior.
Use matching source/PMOD/color words to locate save-station color loss before
changing emitter, bridge or shader. Native world-sprite depth and collection
light reconstruction remain separate open tasks.

## Actual C++ decoder differential verification (2026-10-10)

tests/vdp1_decoder_differential_test.ps1 extracts the actual pre-cache decoder
from 386fff22889b8e63676e8d6d23177f36c1fd4f6f and the current decoder into
a generated host harness. MSVC BuildTools compiled and executed 2,592
comparisons successfully: modes 0-5, SPD/end-code flags, three widths/heights,
four palette banks, zero/all-ones/random VRAM and changed CRAM between passes.
It compares return status, dimensions and every decoded RGBA pixel.

The harness supplies controlled memory, descriptor accessors, logging stubs
and the same color resolver to both actual function bodies. It isolates
decoder/cache equivalence; it does not validate those harness adapters, native
memory synchronization, GPU uploads, pixel appearance or Vita frame time.
Generated source/executable remain in build/host-vdp1-decode for inspection.
This is stronger than the independent cache reference test, not a replacement
for the user's current e5cd6df hardware test. No runtime changes in this test.

## All-frame median budget bound (2026-10-10)

The report now derives a conservative median <=25 ms proof from completed
field windows when strictly fewer than half their frames exceed 25 ms.
This covers every counted frame, unlike ScenePerf heartbeat samples, but
does not recover the exact median or cover incomplete tails/reset gaps.
The strict threshold handles even-count middle-value averaging; exactly
half above the budget is inconclusive. Empty/even/odd threshold tests pass.
For the 643,982-byte provisional comparison: 392/3,000 frames exceed 25 ms,
so the completed-window median bound is proven. The two 56-59 ms rebuild
outliers still contradict deadline compliance; overall completion remains
unproven and no performance target has been relaxed.

## New comparison capture, identity pending confirmation (2026-10-10)

Downloads log changed to 643,982 bytes, timestamp 18:59 UTC, while user reports
testing 386fff22889b8e63676e8d6d23177f36c1fd4f6f. Treat it provisionally as
that test, not an independently identified binary. Its 76 RBG-enabled native
samples have render median 24.2005 ms, p90 25.637 ms and finish median
14.175 ms, versus e8ac156's 24.733/26.465/15.196 ms. Route and sample-count
differences prevent attributing the entire change to coefficient decoding.
25 complete field windows cover 3,000 frames: 392 over 25 ms and two over
33.333 ms, max 59.156 ms. Complete presentation windows report zero
>2-vblank intervals. Reset-baseline rebuild outliers remain 56.022 and
59.156 ms with builds 35.441 and 38.984 ms. No goal completion claim.

Separate sprite-path audit: native projected particles currently enter the
VDP1 screen-space compositor, which uses depth ALWAYS with depth writes
disabled. Correcting pixel units restores size but does not establish world
occlusion. A future generic world-sprite submission needs explicit transform
space/depth ownership, separate from foreground LCS/UI. Do not change all UI
depth testing to repair particles. This is a source limitation, not a newly
observed hardware regression on the untested sprite correction.

## Located field resource-inventory boundary (2026-10-10)

Source audit: setupField2 in field_a3/o_fld_a3.cpp calls
loadFileFromFileList(r4->mC), creates every grid cell task, pauses them, then
enables Azel-selected cells. setupGridCell binds each task's memory area and
environment/billboard cell lists. field.cpp::loadFileFromFileList loads MCB
into the native bundle slot and CGB into the native VDP1 character area;
getMemoryArea maps indices above two to slot two. This is a candidate
game-thread inventory boundary, not evidence that asynchronous readiness,
publication, or resource preparation has been implemented.

The generic bridge must distinguish resource registration from render
submission. Registration must not run cell draw tasks, unpause cells or
change visibility. Collect authored render-model offsets/LOD alternatives
from the loaded grid; do not treat the fifth collision-model offset as a
visual resource. Publish adapted immutable descriptors with bundle/scene
generation and texture-memory readiness at the existing frame boundary;
Neptune prepares GPU resources only on its owning thread. Pointer identity
alone cannot distinguish reloaded bundles at reused addresses. A grid
inventory does not cover task-created actors, particles or dynamic object
models, which retain first-encounter registration.

published_adapted_model currently exposes only this frame's submitted model
list, although backing adapted models persist in g_modelCache. Iterating that
published list cannot discover not-yet-submitted cells. Actual hardware
FieldTextureUpload confirms the first slow frame uploads only the appended
227 textures (retained=677,total=904,uploaded=227); retained texture reupload
is not its cause. Earlier preparation needs the separate inventory channel,
not another modification to the active flattened geometry set.

## Native sprite projection unit correction (2026-10-10)

User confirms missing collection orb sprites, following trails and save-station
particles in every debug view, not just Full. Source audit found a concrete
adapter error: PDS::initVDP1Projection computes r0 from integer half-screen
width via setDividend(width/2, cos, sin), then FP_Mul applies aspect ratios.
Thus width/height scales carry raw integer pixels, despite fixedPoint type.
MTH_Mul_5_6(scale, viewPosition, inverseDepth) already returns raw pixels.
Upstream battleEnemyLifeMeter compares those results directly with integer
screen bounds. Vita drawQuadInternal incorrectly used getInteger(), shifting
them by an additional 16 bits, then clamped tiny half-sizes to one pixel.

The adapter now uses asS32() for particle position/half-size projection and
removes the equivalent /65536 in the native ray-segment projection path.
No Azel changes, arbitrary particle enlargement, camera rewrite or desktop
BGFX route. Texture animation, command descriptors and clipping are retained.
The reference check demonstrates scale=176, x=1, z=2 projects to 88 pixels,
not zero; it is a numerical/source guard, not full runtime coverage.
Vita package compilation passed. Require hardware checks of orbs, trails,
save particles, occlusion and frame-time impact. The separate collection
sparkle Unimplemented draw and immediate billboard stub remain unresolved.

## Hardware baseline e8ac156 results (2026-10-10)

User identifies tested package as e8ac1568c79ff01c97d78d4ac5a3db1ec34af61d
and confirms LCS icons now draw correctly in all modes. This hardware result
validates the icon-layering correction; do not extend that statement to the
target rectangle, particles, or collection lights without user confirmation.

Fresh Downloads lagi.log is 522,290 bytes, timestamp 2026-10-10 18:49 UTC.
Its 98 RBG-enabled native-scene heartbeat samples have render median 24.733 ms,
p90 26.465 ms, max 80.390 ms; build median 3.0665 ms, finish median 15.196 ms.
These sampled statistics are not an exact field-only/all-frame median.
23 complete field windows cover 2,760 frames: 1,100 exceed 25 ms and two exceed
33.333 ms (max 67.530 ms). Matching complete presentation windows contain zero
>2-vblank intervals, max 33.393 ms, but reset-baseline outliers remain excluded
from interval classification. The performance objective is not met.

The two actual outlier records report render/build/CPU-prep/finish:
60.872/40.125/47.598/11.869 ms and 67.530/46.380/50.746/15.607 ms.
FieldStream frame 2502 grows textures 677->904, material 17.893 ms,
texture upload 17.749 ms. Frame 6560 grows 1758->1895, material 17.428 ms,
texture upload 19.531 ms. Both reuse geometry with zero base/subdivision
allocation timing. This strengthens first-use resource preparation as the
remaining rebuild target; it is not proof of a benefit from later experiments.
The earlier 477.919 ms tunnel stall is not present in this capture, which
does not establish that it is fixed or the same route was fully exercised.

NativeParticle diagnostics contain eight negative-depth rejections and eight
emissions with src=514C, size=0C50, pmod=0080, z=131072, center=0,0,
half=1,1. These bounded early samples prove some commands are emitted but do
not identify their task or demonstrate visible flight effects. Investigate
projection/size and emitter transform contracts before applying scale changes.
Collection sparkles still stop before a drawing call; keep that separate.

Next A/B candidates remain the aligned coefficient shader (386fff2) and
per-decode indexed color cache (af32835). They are absent from e8ac156 and
have no hardware timing or correctness result yet.

## Per-decode indexed color cache experiment (2026-10-10)

decodeLiveVdp1Texture now memoizes each used indexed dot's resolved RGBA
within one descriptor decode. Modes 0/1 retain their existing CRAM/LUT color
resolver; modes 2/3/4 retain their existing bank/mask resolver. At most 16,
64, 128 or 256 color resolutions occur instead of one per surviving texel.
Validity bits are distinct from RGBA values, so transparent zero results are
cached too. No persistent palette cache is added: each invocation starts
empty, preserving native texture invalidation and published palette state.
Direct RGB mode 5, flat polygons, SPD/end-code gates and addressing are
unchanged. This is shared scene decoding, not a field visibility modification.

The host reference test covers all dot masks/bits, repeated and zero values,
and palette changes between decodes, with source integration guards. It does
not execute the Vita C++ decoder or prove full texture/GPU equivalence.
Require fresh first-use material timing and visual checks before claiming a
win; hardware baseline e8ac156 does not include this experiment. Vita package
build and the host reference test passed locally; hardware remains pending.

## Remaining first-use rebuild costs (2026-10-10 source/log audit)

The older 750,237-byte capture's two expensive FieldStream builds are:

| Frame | Build | Material resolve | Texture upload | Texture growth |
| --- | ---: | ---: | ---: | --- |
| 1961 | 40.327 ms | 18.342 ms | 17.900 ms | 677 -> 904 |
| 6407 | 45.300 ms | 17.319 ms | 18.488 ms | 1758 -> 1895 |

Both report reuseGeom=1, baseAlloc=0 and subAlloc=0. Thus these records
specifically do not support buffer allocation churn as the dominant remaining
rebuild cost. Ordinary no-growth prepare records are much smaller (e.g. frame
2321 build 4.274 ms, upload 1.881 ms). The two slow builds accompany the
61.039/66.441 ms reset-baseline renderer outliers. Counters can overlap and
are not a complete additive accounting of frame time.

Source confirms liveTownTextureIndex decodes descriptors on their first miss;
resolvedLiveTownMaterialIndices then caches per-model bindings, and
uploadVdp1Textures uploads appended texture entries using resident slabs.
Persistent geometry residency alone cannot remove first-use material decode
and upload costs. The next resource-system step must consider preparing
available scene model/material resources before first visible use, using
stable source identities and published native memory, without changing Azel
visibility or withholding geometry until preparation completes. Do not
predecode game-thread-mutating memory from the renderer unsynchronized.

Keep the 477.919 ms tunnel outlier separate: its build is only 3.580 ms, so
the two first-use rebuild records do not explain it. Fresh e8ac156 stage
telemetry is required before choosing an optimization for that stall.

## Reproducible hardware log report (2026-10-10)

Run `tools/analyze-flight-log.ps1 -LogPath <hardware-log>` to produce JSON
containing sampled native-scene distributions split by RBG enablement,
complete field renderer/presentation window totals, and individual outliers.
It deliberately does not assert an exact all-frame median from heartbeat
samples or hide incomplete-tail outliers behind passing complete windows.
Package identity must still be supplied separately. Missing stage fields are
null, not zero; overlapping CPU counters must not be added. The fixture test
checks even-count median, nearest-rank p90, window totals and both old/new
outlier formats.

The unchanged 750,237-byte log has 110 RBG-enabled native-scene samples:
render median 25.1045 ms, p90 26.736 ms, finish median 15.084 ms. These samples
are not field-only. Its 38 completed field render windows cover 4,560 frames,
2,654 over 25 ms and two over 33.333 ms (maximum 66.441 ms). Completed
presentation windows cover 4,560 intervals with zero >2-vblank intervals, but
the separate tunnel outlier is 480.707 ms/29 vblanks; it must not be discarded.
This remains older evidence, not a capture of e8ac156 or the shader experiment.

## Aligned coefficient decoding experiment (2026-10-10)

The shared RBG0 shader now converts the fetched coefficient texel's four
UNORM channels to bytes once. Two-byte entries select RG/BA directly;
four-byte entries read RGBA directly, eliminating individual selectByte
branches. Texture fetch count, coefficient addressing, invalid-bit selection,
signed payload decoding and A/B policy are unchanged. This applies to every
scene using the shared shader, not just flight.

`tests/rbg0_coefficient_decode_test.ps1` passed all 65,536 two-byte values in
both aligned lanes with UNORM reconstruction and invalid/sign checks, plus
four-byte sign-boundary reference cases. Vita shader/package compilation
passed; compiled RBG0 GXP is 9,460 bytes. These are not GPU pixel equivalence
or performance results. User is currently testing the separate diagnostic
baseline `e8ac1568c79ff01c97d78d4ac5a3db1ec34af61d`; do not attribute that
capture to this shader experiment. Require the same route/settings and fresh
visual checks on this experiment before retaining it as a performance win.

## Outlier stage attribution (2026-10-10)

FlightPresentOutlier now includes existing CPU preparation, submission,
composition, GXM end/finish, and RBG preparation/resolve timings for the actual
slow frame. The 60-frame ScenePerf heartbeat can miss isolated stalls such as
the 477.919 ms tunnel frame; these event records avoid attributing that stall
from unrelated sampled frames. CPU preparation includes build/composition,
so these fields are not all additive. GXM finish is a CPU wait for completion,
not a direct GPU execution timestamp. Rendering behavior is unchanged.
Use a fresh hardware capture to locate the remaining deadline violations;
neither the median target nor hitch elimination is established yet.

## Requested native point lights and flight particles (2026-10-10)

User requests collection lighting and missing visible flight particles. Both
remain open. Source audit: fieldParticlePool calls drawProjectedParticle for
its simple/billboard entries; the Vita animated-quad adapter already emits
distorted VDP1 commands, preserving animation, texture and optional Gouraud
data. Bounded NativeParticle reports now distinguish missing input, depth
rejection and emitted commands (eight reports per category). Emission records
include source address, texture size, PMOD, view depth and projected bounds.
No animation, clipping or rendering behavior changes in this diagnostic.
Absence of these records does not prove that every effect is absent: other
emitters may use other draw paths. The immediate two-point billboard service
is separately a Vita stub; its located caller is battlePowerGauge, not proof
of the reported flight failure. Do not generalize the stub to all particles.

Further source tracing identifies a separate flight collection gap:
`LCS.cpp::sSparkleParticle::Draw` computes position and scaled opacity, then
stops at `Unimplemented()` with a TODO referencing Saturn function 0602f610.
Consequently these collection sparkles cannot reach the native particle
emission diagnostics. The reconstructed immediate-billboard helper carries
the same function address, but its only actual caller is battlePowerGauge.
Implementing that helper alone will not connect the flight sparkle task.
Recover the collection sprite descriptor, color selection and endpoint
contract before wiring it through generated-source integration; do not choose
an arbitrary replacement texture or apply the camera twice. The existing
helper transforms its inputs despite its header describing view-space points,
so the transform contract specifically needs verification. No particle visual
fix or hardware validation is claimed by this source audit.

Field dragon special-color drawing calls dragonFieldTaskDrawSub1Sub0, which
is unimplemented upstream, then publishes point position via Sub1Sub1 with
default parameter zero. The post-draw special-light path is also unimplemented.
pointLightParams stores native position and two derived parameter words, but
the bridge currently captures only directional vector/color/falloff. The
actual point-light evaluation and reset semantics need source reconstruction
before implementing Neptune output. Do not infer radius/intensity from the
packed parameter or replace Azel's light with a generic additive glow. Inspect
the collection task's special-color control and native lighting mode alongside
the original pipeline before choosing the bridge representation.

## LCS layering regression and frame metadata lifetime (2026-10-10)

User reports LCS icons and the targeting rectangle layering incorrectly after
the far-plane background ordering change. Source identifies a missing native
frame lifecycle step: upstream PDS advances global `frameIndex` each frame;
`createVdp1ExtendedCommand()` stamps that index and the fetch helper rejects
metadata from a different frame. Lagi rewound command slots without advancing
the index. LCS emitters do not create extended depth, so recycled slots could
inherit a previous mountain sprite's valid-looking far-plane depth. The bridge
then classified foreground commands as background.

The adapter now advances Azel's frame index before rewinding the transient
VDP1 tail. Current-frame background depth remains valid; prior-frame extended
metadata is rejected by the existing upstream helper. No command-type or
texture-address exceptions, upstream edits, or shader changes are introduced.
The background-depth host check additionally verifies this lifecycle boundary.
Hardware must confirm LCS icons/rectangle above world geometry and mountains
behind world/dragon across movement and camera transitions. The supplied video
has not been viewed because the available viewer cannot open local MP4 files;
the source diagnosis does not constitute visual validation of the correction.

The fresh 750,237-byte log includes FlightPresentOutlier telemetry. The 61.039
and 66.441 ms rebuild frames both have `baseline=0`, confirming that they are
excluded from interval classification after reset. This explains why complete
presentation windows can miss these renderer stalls. A separate tunnel record
has `baseline=1`, render=477.919 ms, build=3.580 ms and interval=480.707 ms /
29 vblanks; it is a real measured long interval, but its low build time does
not identify its cause. Do not attribute it to texture streaming without
further evidence. The full performance target remains unmet.

## Updated single-pass capture and interval audit (2026-10-10)

The latest log is 536,134 bytes. Its 95 RBG0-enabled ScenePerf samples have
24.428 ms render median, 26.112 ms p90 and 81.355 ms maximum across native
scenes. GPU finish median is 15.366 ms; raw prepare median is 58 us. The prior
973,550-byte capture had 27.732 ms render and 17.812 ms finish medians; route
and workload differences mean these runs are not a controlled shader A/B.
The updated run supports improved timing, not sole attribution to RPMD2.

All 26 complete presentation windows (3,120 intervals) report zero missed
two-vblank intervals. Renderer windows covering 3,120 frames nevertheless
contain two deadline overruns, 60.895 and 67.729 ms; 1,143 frames exceed 25 ms.
The sampled median meets 25 ms, but deadline/hitch acceptance remains unmet.
`recordFlightPresentation()` omits the first interval after reset, whereas
renderer aggregates include their first frame. This is a candidate explanation,
not a proven explanation of both spikes. The follow-up telemetry emits
`FlightPresentOutlier` whenever renderer time exceeds 33.333 ms or the measured
interval spans more than two vblanks. It records whether a prior endpoint
exists (`baseline`), render/build time, interval/vblank gap, pacing time,
rebuild and RBG state. A baseline of zero cannot establish a display interval.
This observation-only change preserves all render/pacing behavior and exposes
slow initial frames that the presentation aggregate cannot classify.

## Shared RPMD2 single-pass checkpoint (hardware pending)

The user requests a universal VDP2 system shared with frontend screens such
as name entry. Source inspection confirms native scene resolves and frontend
D5/name-entry composition both call `drawVdp2Rbg0Gpu()`, using the same
fragment program. This checkpoint combines RPMD2 selection into one draw:
A's coefficient fetch supplies its scale and selector; B-selected pixels use
B's independent transform, planes, format, over-pattern and coefficient.
Disabled A coefficients select A with unit scale. Disabled B coefficients use
unit scale. RPMD0/1/3 keep their existing selection/window behavior. No
upstream Azel source is edited. D5's RPMD3 still uses the shared implementation
and its authored line-window rules.

The compiled GXP is 10,020 bytes versus 8,844 previously. Increased program
size/register pressure may offset reduced selection and draw overhead; no
speedup is claimed from compilation. The signed-wrap and raw-generation host
checks pass; these do not validate shader image output. Hardware must cover
name entry, Ruins, open flight sky, canyon transitions, E006 and Excavation,
plus the RBG-disabled tunnel, and compare completion/presentation timings.

Sharing the draw/shader is already implemented; sharing all raw-memory staging
and GPU residency is a separate remaining integration step. Frontend raw upload
still owns its movie/frontend storage, while native scenes use the producer
generation snapshots. A universal resource owner must preserve these lifetimes
and frontend render-slot synchronization rather than aliasing their storage.
Future optimizations should operate on common VDP2 resources and authored layer
state so screens inherit improvements without area-specific renderer paths.

## Producer-generation hardware result (2026-10-09)

The fresh Downloads log (757,748 bytes) contains the split `gxmend` and
`gxmfinish` telemetry and validates the dirty-generation implementation's
timing effect. Among 99 RBG0-enabled ScenePerf samples, `rbgPrep` median is
54 us and p90 is 67 us, compared with 7,837 us median in the preceding run.
Render median is 27.951 ms, p90 30.337 ms, and maximum 83.687 ms. CPU
preparation median is 9.002 ms. GXM completion remains 17.839 ms median:
`gxmend` is 112 us and `gxmfinish` is 17.725 ms. This localizes the remaining
large wait to GPU completion rather than command finalization. The sample
set spans native scenes and is not a field-only timing distribution.

Thirty complete Full-mode flight windows cover 3,600 renderer frames and
3,600 presentation intervals. Twenty-six presentation windows have no missed
two-vblank intervals and report nominal 29.970 FPS. Four windows contain five
missed intervals total, each with a three-vblank maximum near 50.05 ms.
Three renderer windows exceed 33.333 ms, with maxima 64.614, 33.424 and
70.942 ms. Most open-field frames remain over the 25 ms budget. These results
validate recovery from sustained 20 FPS, but fail the complete performance
goal. First-use/rebuild hitches remain part of the acceptance gate.

Ten sampled tunnel frames have RBG0 disabled: render median 8.164 ms,
`gxmfinish` median 1.976 ms, and CPU preparation median 5.026 ms. Different
scene contents prevent treating this as a controlled GPU A/B measurement;
together with the completion split, it supports targeting RBG0 GPU work next.
The next candidate is a generic RPMD2 single-pass selection path, retaining
the current RPMD0/1/3 behavior and exact coefficient, screen-over, tile and
palette semantics. Complementary early discards in the existing passes mean
draw-count reduction alone does not predict the speedup. Preserve the current
hardware-tested checkpoint for comparison and measure the next shader on Vita.
The new log includes canyon -> E006 -> Excavation and later field/tunnel
activity; no fresh visual acceptance statement accompanies it, so the earlier
failed canyon fade remains unresolved.

## Full-mode VDP2 regression capture and producer generations (2026-10-09)

- **Authoritative working branch:** `feature/vdp2-perf-diagnostics`. The capture
  supplied on 2026-10-09 does not contain the later `gxmend` / `gxmfinish`
  fields, so it is treated as a hardware run of the preceding CPU-shadow and
  flight-layering checkpoint, most likely `21319cb`. This revision identity is
  inferred from the log contract; it was not embedded explicitly in the log.
- The user's full-mode observation is steady 20 FPS after native VDP2 became
  active. Across 74 sampled `ScenePerf` records, render time is 34.925 ms
  median, 36.664 ms p90, and 81.578 ms maximum. CPU preparation is 15.534 ms
  median; combined GXM completion wait is 17.834 ms median and 20.160 ms p90.
  `rbgPrep` is 7.837 ms median and `rbgResolve` is 0.683 ms median. Ordinary
  world build is 2.936 ms median. Later 120-frame renderer windows miss the
  33.333 ms budget on essentially every frame. This independently confirms
  that native VDP2 CPU preparation plus GPU work, rather than field geometry,
  is the steady-state limiter in this run.
- The CPU shadow removed the earlier 36-37 ms uncached/GXM comparison penalty,
  but scanning the complete 528 KiB cached snapshot on the render thread still
  costs approximately 7.8 ms on hardware. Commit `ee7318d` (`Publish VDP2 dirty
  block generations`) moves the exact 4 KiB comparisons to the producer while
  Azel's source and the staging history are ordinary cached CPU memory. A
  per-block generation array is published and ownership-swapped with the raw
  snapshot. The render thread compares only 129 generation integers, copying
  the corresponding raw blocks to GXM storage. The producer comparison remains
  byte-exact and detects changes that later revert; no probabilistic hashes are
  used. GPU resource recreation still forces a complete upload.
- `tests/vdp2_raw_shadow_test.ps1` now covers initial, unchanged, changed-block,
  changed-tail, parameter-only, and GPU-recreation behavior using published
  generations. `tests/gxm_completion_profile_test.ps1` also passes. The full
  Vita build, shader build, link, SELF generation, and VPK packaging pass at
  `ee7318d`. These are host/toolchain results only. Hardware must confirm lower
  `rbgPrep`, improved presentation cadence, unchanged sky/floor/palette output,
  and the new `gxmend` / `gxmfinish` split before this optimization is accepted.
- The user also reports that the fade-out fails at the end of the first canyon
  section. The log places the event at field mode 3 -> transitional mode -1,
  followed by Azel's `fadeOutAllSequences()` and `fadePalette()` calls before
  E006 loads. Existing adapter code is intended to retain the last native scene
  mode and publish Azel's primary fade during mode -1. The `SceneComposite`
  diagnostic prints mode `4294967295` from inside the bridge before that later
  preservation assignment, so it does not by itself prove the renderer
  consumed mode -1. The visible failure remains authoritative and unresolved.
  Do not change Azel's transition logic or add a renderer-owned fade timer from
  this evidence; first add/inspect post-publication transition and fade-state
  telemetry after the performance checkpoint is measured.
- FLD_A3 uses `RPMD=2`. Neptune currently resolves it using two full-screen
  RBG0 draws: parameter B first, then parameter A, both performing expensive
  per-pixel Saturn tile/palette work. A future generic optimization may combine
  RPMD2 A/B selection into one fragment pass while preserving RPMD0/1/3, but it
  is deliberately not mixed into `ee7318d`; the producer-generation result is
  intended to remain independently hardware-testable.

Follow-up source audit: the RPMD2 A/B passes discard complementary pixels
before `sampleRbg0Cell()`. They therefore do not both perform tile/palette
sampling for every output pixel. A combined pass could remove duplicate
selection/rasterization overhead, but a near-halving of GPU time cannot be
inferred from the draw count. Measure the split completion times first.
Producer generations also move the exact comparison work to the game thread;
they do not remove that comparison from total CPU work. Evaluate actual
presentation intervals as well as renderer time before claiming improvement.
The generation test additionally passes alternating-buffer ownership,
change/reversion, unchanged repetitions, and multiple staging calls before
publication. The runtime/package remains the `ee7318d` implementation.

## VDP2 transfer and flight mountain-strip checkpoint (2026-10-09)

- **Authoritative branch:** `feature/vdp2-perf-diagnostics`; resumed remote baseline `f69c5693f675eac3058734636295b84a32adc3f7`.
- The post-`f4e8af7` hardware capture proves a CPU-side regression. With RBG0 enabled, median render rose from 29.79 ms to 62.67 ms, CPU preparation from 10.37 ms to 43.23 ms, and `rbgPrep` from 2.63 ms to 36.73 ms. Median `gxmwait` remained approximately 18.3 ms and `rbgResolve` approximately 0.67 ms. After the initial 528,384-byte upload, sampled frames transferred only 8,192 bytes, so dirty-block selection worked but its comparison path did not.
- The regression source is confirmed in `prepareSceneVdp2Background()`: the dirty-block checkpoint compared the 528 KiB current CPU snapshot directly against GPU-mapped memory every frame. Reading the GXM mapping for the complete comparison made a nominally small transfer cost roughly 34 ms of CPU preparation.
- Commit `f743e3f` (`Compare VDP2 updates against CPU shadow`) keeps an independent, 64-byte-aligned 528,384-byte CPU shadow of the bytes last copied to the GPU resource. Each 4 KiB block compares cached CPU snapshot memory with cached CPU shadow memory. A changed block is copied to GPU storage first and then to the shadow; `rbgRawBytes` records only those uploaded bytes. Initial allocation, failed allocation cleanup, renderer shutdown, and GPU-resource recreation invalidate the shadow relationship and force a complete upload. Rotation, coefficient, window, and scrolling changes do not invalidate raw storage, and the SGX RBG0 resolve still executes on every applicable frame.
- `tests/vdp2_raw_shadow_test.ps1` passes initial full upload, identical/zero-byte update, one changed 4 KiB block, final-block/tail bounds, parameter-only change, and GPU recreation cases. The optimized Vita configure/build, shader compile, link, SELF generation, and `build/Lagi.vpk` packaging all pass. These are host/toolchain results; the expected return of `rbgPrep` toward 2-3 ms remains a hardware validation target, not a measured result.
- The flight mountain strip is now source-identified. It is not NBG1: Azel's `a3_background_layer.cpp` emits repeated normal VDP1 sprites and attaches an `s_vd1ExtendedCommand` depth of `(far clip - 1) / far clip`. Lagi's capture discarded that authored depth and Neptune consequently drew the strip in the same late pass as HUD/UI, after the world and dragon.
- Commit `77a3a37` (`Layer far VDP1 sprites behind world geometry`) preserves valid Azel extended-command depth in the bridge. Neptune generically classifies only authored far-plane VDP1 commands (`depth >= 0.999`) as background presentation, draws them after the RBG0 sky and before native 3D geometry, and excludes them from the later foreground pass. All remaining VDP1 sprites, NBG1 backings, cinematic bars, text, and HUD retain their previous late composition. No FLD_A3 texture/address special case and no `extern/Azel` edit was introduced. A shared sprite-buffer cursor prevents the foreground pass from overwriting vertices already queued by the background pass.
- `tests/vdp1_background_depth_test.ps1` verifies depth publication/classification and the source order `RBG0 -> far-plane VDP1 -> world -> foreground VDP1`. The Vita build passes. Hardware must still confirm that mountains sit above the sky but behind geometry/dragon, their transparent/mesh presentation remains correct, radar/UI/subtitles are unchanged, and the CPU-shadow timings recover. Do not claim either hardware correction complete until that capture is returned.
- The pre-fix hardware log contains 24 sampled field frames. Replacing only the measured 36.87 ms median `rbgPrep` regression with the earlier 2.63 ms baseline projects a 28.76 ms render median, 30.35 ms p90, and 30.73 ms cache-hit maximum. This is a planning estimate, not hardware validation, and remains above the 25 ms median target. The same log has an 18.35 ms median combined `sceGxmEndScene()`/`sceGxmFinish()` interval, now the largest remaining ordinary-frame bucket.
- The next profiling checkpoint separates that combined interval into `gxmend` and `gxmfinish` fields while retaining `gxmwait` for continuity. This is observation-only: scene submission, synchronization, mapped-resource lifetime, and display cadence are unchanged. If `gxmfinish` owns the interval, the next performance work must reduce or safely overlap GPU workload; if `gxmend` owns it, command finalization/submission is the target. `tests/gxm_completion_profile_test.ps1` protects the timing order and log contract. Hardware results are pending.


Lagi's first flight milestone begins immediately after the Ruins elevator movie sequence. Azel advances game status 5 to status `0x50`, which maps to game mode 3 / field index 1 and loads `FLD_A3.PRG` ("above excavation").

The implementation follows the same ownership rule used for the Ruins runtime:

```text
Azel decides.
Lagi services.
Neptune renders.
```

Flight behavior, field scripts, dragon movement, camera state, visibility, animation, encounters, VDP1/VDP2 state, and progression remain Azel-owned. Lagi restores the Vita-facing services and presentation paths required to let that runtime execute natively.


## RBG0 accuracy/performance and effects checkpoint (2026-10-09, unvalidated)

Texture upload now uses a single bulk copy for tightly packed rows; padded rows
copy texels and zero only the padding rather than clearing the entire buffer
first. Pixel bytes, point filtering, texture descriptors and ownership are
unchanged. The host texture_upload_layout_test.ps1 compares the two byte-layout
algorithms across 114 dimensions starting from dirty destination memory; all
cases pass. This checks layout equivalence, not GXM timing or cache lifecycle.
Vita renderer syntax checking passes; hardware upload-time savings are pending.

21:45 capture: scene clear samples are approximately 0.75–0.86 ms after SGX
tile initialization replaces CPU depth/stencil clears. Heavy flight views
still miss presentation slots. Tunnel entry retains 1,915 textures and adds
58, but texture upload costs 96,956 us and build costs 107,099 us. The old
uploaded count described only the appended suffix, while dirty refresh copied
the entire retained prefix as well.

The local follow-up tracks decoded and uploaded generations by texture index.
Native memory/palette invalidation advances the generation. Textures referenced
by the current flattened native polygon records are decoded from the published
native memory if stale; unchanged GPU entries are skipped during refresh.
Inactive stale entries are decoded/uploaded when next referenced, preserving
current content without copying the full historical cache on every transition.
New live decodes record their generation immediately. Full GPU residency release
clears uploaded generations. FieldTextureUpload now counts actual copied textures,
including refreshed retained entries, rather than only appended entries.
Vita renderer syntax check and diff whitespace check pass. Hardware visuals,
palette/address-reuse behavior and tunnel-entry hitch timing remain unverified.

The next actor diagnostic traces the actual E006 dragon, town dragon, and
Excavation NPC draw expressions with a separate four-report budget at each
call site. NativeActorDraw reports how many bridge submissions that expression
emits. The original expression executes exactly once on every invocation;
there is no substitute model, culling change or task ownership change.
This closes the initialization-trace blind spot for the reused dragon model.
CMake generation and Vita syntax checks of all three generated actor source
files pass. Hardware draw-boundary evidence is pending.

Latest 21:34 hardware log includes the actor diagnostics. It records 32 model
initializations and no unsupported-draw or model-adaptation rejection reports.
This does not prove that the reused dragon reaches its draw boundary; dragon
reinitialization does not call init3DModelRawData. Four of twenty flight
presentation windows have zero missed two-vblank slots; heavier windows still
miss slots. Later scene render samples remain about 31–33 ms.

The next local performance change removes CPU clears of the scene depth and
stencil backing buffers. The installed GXM SDK documents that initialized
depth/stencil surfaces default to no forced load/store, with tile background
depth 1.0 and stencil 0. Neptune now explicitly sets those same background
values and disables forced load before the scene. Spill buffers remain allocated
and attached; partial-render handling is unchanged. Color-buffer clearing remains.
Renderer Vita syntax checking passes. Frame-time savings and visual behavior
need hardware verification; this is not a fix claim for missing actors or
mountain overlap.

Performance-first follow-up: the latest capture's `FieldStream` frame 7004
reports 173376 us texture upload, 179699 us build, reuseGeom=1, reuseTex=0,
and 1942 resident textures. This is a texture refresh/allocation stall, not
evidence that world-grid geometry itself costs 179 ms.

The local upload change retains GPU mappings across content invalidation when
the complete retained prefix matches dimensions and native texture descriptors.
Dirty pixels are still copied, and opacity metadata is recalculated. Compatible
textures do not repeat allocation or GXM texture initialization; new suffixes
are allocated normally, and mismatched layouts retain the full replacement
path. The render-thread invalidation boundary remains intact. No game-owned
resource lifetime, culling, sampling precision or filtering is changed here.

Vita C++ syntax checking of `neptune_renderer.cpp` passes. Full SELF/VPK build
and on-device refresh/canyon/tunnel timing are pending; no performance win is
claimed until the hardware log confirms it. Cold model/texture discovery and
the remaining RBG0 GPU cost remain separate open work.

The 20:15 user log contains the new `SceneComposite` records. Open FLD_A3
uses RBG0 priority 3 and map A/B addresses 0x64000/0x60800. The tunnel
explicitly disables RBG0 (BGON=000A, PRIR=0) and has roughly 9 ms renderer
windows; open views remain roughly 29–32 ms with 179–192 ms build spikes.
No claim of meeting the full frame-time target is supported.

The attempted projection-parity culling change failed hardware acceptance:
it swapped visible winding without correcting the reported 180-degree camera
orientation. It has been reverted to the previous room culling convention.
The user subsequently accepted camera orientation. Do not change camera yaw or
rasterizer winding to address the remaining missing actors or background overlap.

The current hardware report still has no dragon/rider in the cinematic, no
dragon or Captain in Excavation, and flight mountains appearing over world
geometry. Source inspection confirms RBG0 is composited before VDP1 world
geometry, with depth writes disabled; this rules out a simple final-background
draw-order swap, not every compositing, culling or texture-discard defect.
The native NPC load-completion task clears its pending flags, and the generic
NPC switches to its Draw2 method after initialization. These source paths do
not yet prove that the corresponding actors reach rendering on hardware.

Bounded diagnostics now report NativeActorModelReady (at most 32 initializations),
NativeActorDrawMissing (at most 16 distinct model pointers reaching an upstream
empty draw variant), and NativeModelAdaptRejected (at most 16 rejected model
pointers). No substitute draw behavior was introduced. CMake generation,
the adapter replacement test, and Vita syntax checks for the generated
mainMenuDebugTasks.cpp and bridge pass. No SELF/VPK was built by this checkpoint.
The available 20:48 log predates these diagnostics; hardware actor-boundary
evidence and full performance acceptance remain pending.

The 20:33 log still reports approximately 33.5–35 ms ordinary scene renders,
including 18.4–20.1 ms GPU wait. Excavation publishes map A=0x00000 and
map B=0x60000; its native initializer configures only map B. This is a concrete
lead for the corrupted floor, not proof of the intended missing map-A setup.
Ruins publishes map A=0x60000 and map B=0x61000, so the two cases must not be
treated as one scene-specific floor-disable workaround.

RBG0 aligned-word fetch checkpoint: pattern-name and CRAM reads now use one
aligned RG/BA pair from the packed raw texture, avoiding the general unaligned
reader's cross-texel case. CRAM addresses wrap at 4 KiB as native
`getVdp2Cram()` does. The optimized fragment program compiles successfully;
its GXP size falls from 9,620 to 8,804 bytes. This is compiled-program evidence,
not a measured GPU speedup. The SDK shader statistics tool did not return and
was cancelled; hardware timing and visual acceptance remain required.

Shared-plane sampling now wraps integer coordinates directly to the aliased
plane instead of first wrapping to the four-by-four map and then to the plane.
The original unwrapped screen-over test is retained. Both 512- and 1024-dot
plane dimensions match the old calculation for 65,538 signed coordinate cases
(-16,384 through +16,384). The combined fragment program compiles to 8,820
bytes; this optimization is not yet hardware-timed. The log remains the 20:33
capture and cannot validate these newer shader changes.

Independent Excavation endian correction (next build): its native u32 pattern
fill at 0x60800 stored intended 0x5000 names as bytes 00-50 on little-endian
Vita, which `getVdp2VramU16()` and SGX correctly decode as 0x0050. The
generated-source adapter now uses `setVdp2VramU16()` for the same 8,192-byte
region. `cmake -P tests/exca_pattern_adapter_test.cmake` passes against the
actual adapter and upstream source. No map addresses, camera state, plane
visibility or upstream files change. This repairs a proven write-format bug,
not the still-unproven intended map-A setup or the camera orientation. A full
Vita build and hardware check remain user-run; do not attribute an already-built
shader-only checkpoint's results to this later source adaptation.

Independent VRAM address correction (next build): the upstream helper masked
relative addresses to 2 MiB despite owning only a 512 KiB `vdp2Ram` array.
The generated Vita adapter now masks with `sizeof(vdp2Ram) - 1`, preserving
all existing in-range accesses and keeping mirrored addresses inside the owned
buffer. `cmake -P tests/vdp2_vram_adapter_test.cmake` validates the actual
adapter and representative boundary addresses. No upstream file or scene
policy changes. This prevents out-of-allocation reads; it does not establish
that such a read caused the reported floor or camera failure.

RBG0 coefficient reuse (later shader checkpoint): RPMD=2 parameter A previously
fetched the same coefficient separately for its invalid-bit A/B selection and
coordinate scaling. The shader now decodes both from one packed fetch; B still
reads A's selector and its own coefficient when enabled. RPMD=0/1/3 selection
rules are unchanged. Shader compilation passes (8,844-byte GXP), and the old
last-byte selector agrees with the reused flag for all 65,536 16-bit words plus
1,280 32-bit boundary/high-byte cases. These are decode checks, not hardware
performance or full-image equivalence proof. The newly arrived 20:48 log was
captured before this later edit and must not be attributed to coefficient reuse.

20:48 hardware capture: sampled ScenePerf medians are 30,629 us in mode 1,
32,440 us in mode 2 and 29,022 us in mode 3. Sample maxima include 85,025 us
in mode 1 and 34,362 us in mode 3; the full target is not met. Two field
windows render at about 29.6 ms yet present at about 47–48 ms (20–21 FPS).
The installed-executable comparison was confirmed by the user; the current
log has no unique embedded build ID, so retain the executable/VPK hashes.

Next publication checkpoint: RBG0's two existing 528,384-byte CPU snapshots
are now exchanged under the already-acquired free-slot token instead of
copied again. A pending-dirty flag prevents repeated publication from swapping
back to an old snapshot. The producer writes only its buffer and cannot swap
until the previous renderer has completed GPU work and presentation; SGX
upload and raw decoding are unchanged. Renderer Vita syntax verification passes;
hardware correctness/timing remains unverified.

Existing ScenePerf GPU wait and render timings are retained. FlightPresentWindow
now adds mean preRender, postRender, publish and present times using scalar
timers only. preRender is interval minus render/postRender/presentation and
includes publication (publish must not be added to it again); it also includes
renderer wakeup and any unmeasured front-end/pre-render work. postRender measures
work between the closed render timer and presentation. These fields distinguish
missed display slots from GPU-only cost without another polygon traversal.

The user now authorizes reading newly supplied logs directly from
`D:\Users\Noradninja\Downloads`. Check file freshness/package correspondence
before using a capture; an unchanged file is not evidence for a newer edit.

Follow-up hardware capture: the updated log still contains 34–36 ms scene
renders. Several 120-frame windows average about 30 ms rendering while
presentation averages about 50 ms (20 FPS); renderer-only timing does not
prove the end-to-end 33 ms target. The user identifies mountains appearing
over foreground polygons. RBG0 is currently composed before VDP1, so changing
that call order alone is not an established fix.

The subsequent unvalidated SGX change uses a balanced 16-plane selector and
an exact shared-plane path when all sixteen native map addresses are equal.
The latter skips plane selection, not native rotation, screen-over or pixel
decoding. Shader compilation passes; exhaustive selector-slot checks and
boundary/negative-coordinate shared-plane checks pass. Hardware speed and
visual correctness remain unproven.

Transition-only `SceneComposite` logs capture native priorities/maps, and at
most eight `SceneOverlay` records identify oversized late-composed field
sprites. These diagnose overlap without hiding objects, changing ordering,
adding a view mode or adding a per-polygon profiling pass. The next capture
must establish the responsible layer/command and measure the SGX change.

The latest user-supplied log spans Ruins, canyon and tunnel views. Ruins
samples show approximately 18–22 ms graphics wait; tunnel samples show about
2 ms. A later 120-frame window averages 30.955 ms renderer time, with a
204.492 ms maximum and a 179.495 ms build maximum. Both sustained GPU cost
and static-frame rebuild hitches need attention; neither the 33 ms worst-frame
target nor complete effects support is accepted yet.

The working-tree checkpoint preserves the local menu changes and adds:

- Independent native RBG0 A/B plane sizes and screen-over modes/OVPNR.
- Native transparent-dot control and priority-zero background suppression.
- Packed coefficient fetches on SGX, with 16-bit coefficient decoding and
  packed line-window reads; background sampling remains point-filtered.
- Live VRAM decoding for 64/128/256-color and RGB555 screen sprites, rather
  than falling back to a Ruins-specific texture decoder for field effects.

Only the revised RBG0 shader has been compiled successfully. Full C++ build,
SELF/VPK packaging, performance improvement and hardware visuals are pending.
Native particle/pickup commands already exist; this decoder change alone does
not establish that their complete lifecycle, ordering and appearance work.
Pinned Azel prepares `pointLightParams` but lacks the consumer calculation;
recover that native behavior before adding attenuation, rather than inventing
an alternate point-light model.

VDP2 design rule: Azel register state governs visibility, mapping and effects;
Neptune performs the pixel work on SGX. Use NEON only for measured CPU work
where it preserves the native results. Do not disable backgrounds by town ID
or trade accuracy for a claimed frame-time win. Wait for the user's next
hardware log before selecting the next performance change.

## Work in progress after hardware build 8993213 (2026-10-09)

The user supplied a fresh `lagi.log` after explicitly rejecting the older log.
This capture is the evidence for the next changes. Ruins orientation, flight
subtitles/bar, LCS, Excavation fade-up, Captain selection/FMVs, FMV skipping,
and the return to Above Excavation/tutorial text are working on hardware.
Battle is deliberately outside the current milestone: do not enable it.

Remaining hardware failures: Ruins water flicker and cyan hall quad; towns
around 20 FPS; no flight exit fade; black E006 cinematic (Start can skip it);
invisible Excavation dragon and Captain. Earlier missing particles, collectible
trails and excessive pickup lighting also remain unaccepted. The user identifies
VDP2 sky enablement as the introduction point for the water/FPS regression.

The new log shows E006 advancing through frame 1080 and completing/skipping at
frame 1106, with 115 model submissions on its periodic heartbeat. Both its
fade-in and fade-out counters advance. This proves task progression, not visible
cinematic rendering. Its streamed `updateEngineCamera` call bypassed the town
camera capture; the generated source now captures that authored view directly.

The bundle relocation helper also added VDP1 allocation offsets to every
CMDCOLR, including immediate RGB555 polygon colors. The next generated loader
relocates texture source addresses and mode-1 LUT addresses only, preserving
flat colors and palette-bank values. This targets the cyan quad through the
shared resource service rather than editing Ruins geometry. Matrix-driven model
draw variant 3, previously an empty upstream stub, now traverses the authored
bone matrices. Shared VDP2 word reads reuse a packed texel for adjacent bytes,
including a fallback for words crossing a texel boundary, to reduce redundant
sky/background texture reads without changing resolution.

These changes are not yet a hardware pass. Vita C++ compilation, affected
RBG0/NBG shader compilation, linking and SELF generation passed. Continue
investigating the water mask, town presentation timing, field exit sequencing,
and missing actors; do not infer their acceptance from a successful build.

For a new chat at home: read this section and the checkpoint history below,
inspect the actual branch HEAD, and use the newest explicitly supplied hardware
log. Make the edits, validate and push Git, then give pull/local-build directions.
When a new device log is required, wait for the user to send it; an existing
`D:\data\lagi\lagi.log` is not automatically the latest capture.

Pull/build/install for this checkpoint (stop if any command fails):

```powershell
Set-Location C:\Dev\Lagi
git switch feature/flight-mode-bringup
git pull --ff-only origin feature/flight-mode-bringup
git submodule update --init --recursive
$env:VITASDK='C:\Dev\VitaSDK'
$env:PSP2CGC='C:\Dev\sdk\host_tools\bin\psp2cgc.exe'
$env:PATH="$env:VITASDK\bin;$env:PATH"
cmake -S . -B build -DLAGI_OPTIMIZE_NEPTUNE=ON
cmake --build build --parallel 32
```

Install `C:\Dev\Lagi\build\Lagi.vpk`. Test the Ruins hall/water and town FPS,
flight cave exit/fade, E006 without skipping first, then Excavation dragon and
Captain. Also check the already-working subtitles/LCS and FMV skip/return.
Send a fresh log and photos before the next hardware-dependent patch. The new
chat should obtain the checkpoint hash from the Git commit containing this
section; the parent hardware baseline is `8993213`.

## Previous checkpoint - live presentation and native effects (2026-10-09)

The user's new hardware report confirms Ruins orientation is fixed, and flight
subtitles/black bar and LCS text/icons/tiles work. Remaining evidence: unintended
Ruins water/white-blue background, frozen flight image through E006/Excavation,
missing effects, wrong laser color, and excessive item-collection lighting.

The new log proves E006's stream advances to frame 1160, completes at exactly
310556 consumed bytes, and loads TWN_EXCA.PRG. This is a presentation failure,
not evidence that the cinematic task stopped. Two gates were wrong: rendering
required a nonempty fallback camera despite a valid captured native matrix,
and publication excluded native town mode 2 (Excavation uses mode 2/status 0x51).
Both are corrected generically. Scene fades/UI are also presented on empty
world frames and during native module transitions, avoiding a retained flight
image when the old task graph is torn down. Azel still advances all fade state.

RBG0's WCTLC visibility window is now applied independently of WCTLD rotation
parameter switching. Ruins authors this mask for its water/background; removing
RBG0 altogether would suppress native content instead of fixing visibility.
The shared shader consumes the authored rectangle/line-window data.

Native projected particles now preserve texture flips and supplied Gouraud
colors; unshaded particles no longer request an uninitialized Gouraud table.
The frame snapshot carries the four colors, and Neptune composes them through
its existing RGB555 shader. Previously disabled flight ray calls now terminate
at Lagi's shared VDP1 service, using Azel's endpoints, width, texture/palette and
colors. This restores the path for beams and homing/trail effects without
hard-coding a blue palette or changing native item behavior.

The collection-light locality issue remains open: native field code requests
point-light state, but the current bridge implements directional lighting.
Do not claim that the present checkpoint fixes its spatial falloff. Flight exit
fade timing also needs hardware confirmation: the supplied log shows no gradual
field fade request before module teardown, though E006 fades advance normally.

Use the pull/build/install steps below. Capture Ruins background at the same
hall location, flight particles and an item collection, laser/trail color, cave
entry, E006 camera/world animation and fades, Excavation arrival and its visible
pause menu. Send the full fresh log and photos of any remaining fault. Wait for
that user evidence before dependent fixes; do not mark the overall goal complete.

Validation for this checkpoint: Vita C++ and RBG0 shader compilation, link,
VELF/SELF creation and VPK packaging passed. These checks do not establish
on-device correctness. Upstream Azel remains unchanged.

## Ruins orientation correction - 2026-10-09

The user reports Ruins meshes inside out and horizontally reversed after the
native-camera checkpoint. The old town-only projection X reflection remained
active even when consuming the captured native view. That reflection reverses
screen orientation and projected triangle winding. Native town, cinematic and
flight views now all omit it and use the same existing native culling convention.
The reconstructed camera fallback retains its historical transform.

This correction supersedes the prior checkpoint's statement that town mirroring
was retained. Azel camera/model state remains authoritative; no area-specific
mesh reorder or game-state change is introduced. Check Ruins left/right layout,
outer walls and actor visibility, then flight and cinematic orientation. Hardware
acceptance still requires the user's fresh log/photo. Use the pull/build/install
instructions below; wait for that evidence before dependent changes.

## Latest authoritative checkpoint - native scene transitions (2026-10-09)

This section supersedes the earlier checkpoint below. Branch:
`feature/flight-mode-bringup`. Azel decides; Lagi bridges/services; Neptune renders.

### Supplied hardware evidence

The supplied `lagi.log` and cinematic photo show successful FLD_A3 -> TWN_E006
loading, but missing fades, malformed/missing cinematic world geometry, and an
apparent freeze before Excavation. E006 now keeps static geometry resident
(`staticRebuilt=0`); its build/upload cost is low, so the previous recurring CPU
rebuild issue is not the current explanation. The log ends during E006 fade-out
(count 40) and does not prove a completed TWN_EXCA transition.

The scene adapter skipped camera/clipping publication when no Edge actor existed.
E006 therefore inherited presentation state from flight. Native fades advanced
in the log while the renderer used a separate clock. Repacking a running native
fade also halved negative offsets and reversed red/blue channels.

### Shared implementation

- Towns, fields and real-time cinematics capture Azel's native view, remove it
  from view-relative model submissions and apply it once in Neptune. Camera,
  near/far planes and lighting no longer depend on an Edge actor being present.
- All native scenes use the flight texture pool, descriptor lookup, geometry
  capacity/residency, UV retention and first-visible subdivision position repair.
  Existing flight orientation and radar behavior remain intact.
- Native primary fade colors are published with each completed frame and applied
  after world/UI composition. Azel owns the countdown. Field entry uses the
  existing 30-frame entry request through Azel's controller; Neptune has no
  independent scene fade clock. Source-color packing now inverts native unpacking.
- Ruins' VDP2 text/backing snapshot is shared by all native scenes, including
  opening badges, LCS, subtitles and cinematic bars. Hardware must confirm each.
- Streamed bundles receive their native texture base once. Teardown frees the
  payload separately from its host wrapper. Command-ring header writes preserve
  all 32 bits; parsing is bounded by actual file size instead of sector padding.
  Invalid packet lengths stop with a diagnostic rather than an infinite loop.

A read-only audit of the supplied Disc 1 image found E006.EPK is 310,556 bytes:
24-byte header, 580 valid packets, 60 Hz, 1,160 native frames, and no trailing
payload bytes. The final sector therefore contains padding that must not be
parsed. No E006 header crosses the command-ring boundary in this file; the
32-bit rollover fix is a general correction, not evidence for this particular
freeze. The next hardware log remains the transition acceptance gate.

Upstream changes are generated by CMake; `extern/Azel` remains unchanged.

Validation: Vita C++/shader compilation, link, VELF/SELF creation and VPK
packaging passed; package entries were inspected. The workspace path contains
spaces, so packaging used relative asset paths to avoid the SDK packaging rule's
unquoted asset arguments. The user's C:\Dev\Lagi build path avoids that issue.
These build checks do not establish on-device rendering or transition correctness.

### Required local build and next hardware capture

Run in PowerShell. Stop if a command fails; do not discard local changes to make
pull succeed.

```powershell
Set-Location C:\Dev\Lagi
git switch feature/flight-mode-bringup
if ($LASTEXITCODE) { throw 'Branch switch failed' }
git pull --ff-only origin feature/flight-mode-bringup
if ($LASTEXITCODE) { throw 'Pull failed' }
git submodule update --init --recursive
if ($LASTEXITCODE) { throw 'Submodule update failed' }
$env:VITASDK = 'C:\Dev\VitaSDK'
$env:PSP2CGC = 'C:\Dev\sdk\host_tools\bin\psp2cgc.exe'
$env:PATH = "$env:VITASDK\bin;$env:PATH"
cmake -S . -B build -DLAGI_OPTIMIZE_NEPTUNE=ON
if ($LASTEXITCODE) { throw 'Configure failed' }
cmake --build build --parallel 32
if ($LASTEXITCODE) { throw 'Build failed' }
```

Install `C:\Dev\Lagi\build\Lagi.vpk`. Capture a fresh complete log from flight
entry through its exit, E006 cinematic and Excavation arrival. Check flight
fade-out, cinematic fade-in, world geometry alongside dragon/rider, cinematic
fade-out and town arrival. Also check opening badge/background, LCS text/backing,
subtitles and their black bar. Include a photo if geometry/UI is still wrong.

New `[AzelCutscene] timeline`, `complete` and `invalid packet` records distinguish
stream progress from a frozen presentation. Do not advance past this checkpoint
or perform dependent fixes until the user supplies the next hardware log.

The retained workflow is: inspect user evidence, edit, perform available build
checks, push Git, provide pull/build/install directions, then wait for the user's
log whenever hardware analysis is needed. Keep this workflow in this document.

## Earlier checkpoint - 2026-10-09

> **Resume from this section.** This checkpoint supersedes the older dated
> "Current handoff" / "Immediate next task" sections below. Those older sections
> are retained as investigation history and should not be treated as the current
> branch head or next action.

- **Branch:** `feature/flight-mode-bringup`
- **Latest code checkpoint before this documentation update:** `2a6d1d47346c14f61ce02abcb2404418f74b0c12` (`Reset scene residency tracking only on real mode transitions`)
- **Previous renderer-residency commit:** `c6fa3b42fc7089a2bd13d005fde44d7d20ccfca0` (`Keep static scene geometry resident during native cutscenes`)
- **Cutscene / Excavation bring-up commit:** `8b13e27161facfe2543cfc219544b359a8f9e277` (`Complete native town cutscene and Excavation bring-up`)
- **Architecture rule remains absolute:** **Azel decides. Lagi services/bridges. Neptune renders.**
- Do not create an E006-only renderer, cutscene-specific scene recreation, or
  area-specific progression logic. Native in-engine cutscenes must feed the same
  general Azel -> Lagi bridge -> Neptune rendering and resource-residency path
  used by ordinary towns and fields.

### Progression now reached on hardware

The original end-of-first-field crash has been traced and moved substantially
forward. The old failure was a stale overlay dispatch caused by milestone link
trimming: Azel correctly selected game status `0x06` / `TWN_E006.PRG`, but
Lagi's generated town source had removed the E006 dispatch, leaving
`gFieldOverlayFunction` pointing at the torn-down FLD_A3 overlay. That path
eventually called `allocateHeapForTask(nullptr, ...)` and faulted at address
`0x00000004`.

That stale-dispatch failure is fixed. The Vita now reaches Azel's authentic
post-flight path:

```text
FLD_A3 door
    -> Azel status 0x06
    -> TWN_E006.PRG
    -> native town/E006 setup
    -> fade / in-engine cutscene path
    -> E006 streamed command data
    -> fade / transition
    -> TWN_EXCA.PRG (Excavation Site)
```

The E006 sequence is an **in-engine Azel cutscene, not Cinepak**. Expected game
ordering remains:

```text
fade out
    -> fade in to in-engine cinematic
    -> fade out
    -> fade in at Excavation Site
```

Lagi must not manually reproduce that ordering. Azel owns the script, fades,
camera, actors, animation, subtitles, completion and next-area choice.

### Generic native town-cutscene bring-up now wired

The broader bring-up pass intentionally wired the subsystem instead of waiting
for one null/stub per hardware run.

The generated Vita copy of Azel's town cutscene runtime now receives generic
platform services for:

- mounted Disc 1 ISO/CUE streaming through Lagi's existing random-access disc
  layer rather than desktop `fopen()`,
- stream open/read/close lifecycle,
- explicit Saturn-style cleared task initialization,
- the 0x20000 command-stream ring buffer,
- the expected 0x8000 audio scratch area,
- safe stream/task teardown and completion state,
- a 30 Hz Vita timeline driven from the stream's declared frame rate,
- pause/timeline gating without replacing Azel sequencing,
- native cutscene entity/model/camera/subtitle command execution,
- and the native follow-on `TWN_EXCA` overlay/dependencies.

The exact Saturn streamed-audio packet opcode in upstream Azel is still
incompletely reconstructed. Do **not** invent E006-specific packet semantics.
Lagi's existing native SCSP/BGM services remain separate while the general
cutscene stream parser progresses.

### Current rendering/performance problem discovered in E006

Hardware reached the in-engine E006 presentation, but the scene showed a major
presentation/performance failure: the lower scene geometry appeared badly
fragmented/corrupt while one CPU core was near saturation (about 93% in the
captured PSVShell overlay).

The key profiler evidence was not GPU saturation. During affected E006 frames:

```text
[ScenePerf]
build ~= 496 ms
upload ~= 493 ms
cpuprep ~= 503 ms
gxmwait ~= 19 ms
render ~= 522 ms
staticRebuilt=1
```

A normal initial/healthy-style sample was orders of magnitude smaller
(`build` around hundreds of microseconds and essentially no upload). This
showed that the native cutscene was causing Neptune to repeatedly rebuild /
re-upload scene geometry on the CPU rather than using the already proven
field/town residency path.

**Do not optimize a separate cutscene renderer.** The correct architecture is:

```text
Azel town / field / in-engine cutscene task graph
        -> normal model submissions
        -> shared Lagi render bridge
        -> Neptune static/dynamic residency classification
        -> resident GXM geometry/material/texture resources
        -> SceGxm
```

Camera motion, fades, VDP2 changes and palette/color-offset activity must not
invalidate rigid VDP1 world geometry.

### Latest renderer-residency correction - hardware validation pending

Commits `c6fa3b4` and `2a6d1d4` implement the current generalized correction.
They were pushed before the 2026-10-09 work-session handoff but have **not yet
been hardware-validated**.

The correction changes Neptune's residency rules, not Azel's game logic:

1. **CRAM / VDP1 texture invalidation no longer destroys the static geometry
   cache.** Palette animation, fades, texture content updates, subtitles and
   related presentation changes refresh texture/material state without
   resetting the static world signature, static mesh ranges or resident world
   topology.
2. **Transform-changing submissions are automatically promoted to the existing
   dynamic-object path.** Neptune tracks native instance identity separately
   from its matrix. If a supposedly rigid submission's transform changes, it is
   kept dynamic for that scene. This is generic for cutscene actors, moving
   machinery, NPCs and other task-owned objects.
3. **Real game-mode/scene ownership transitions reset residency tracking.**
   Camera/fade/palette changes do not. This prevents stale instance history from
   crossing a genuine mode transition without turning ordinary presentation
   changes into full geometry rebuilds.
4. The field/town rendering pipeline remains the only native 3D path. There is
   no special E006 renderer.

### Immediate next hardware acceptance test

Build the current branch and replay the same first-flight door transition.
Capture `lagi.log` and, if useful, video/screenshots through E006 and into
Excavation.

The primary acceptance condition is that after any legitimate initial
scene/resource build, steady E006 frames report roughly:

```text
staticRebuilt=0
upload = small / near-zero for unchanged resident geometry
build = normal field/town-scale work, not hundreds of milliseconds
```

There may be a one-time rebuild when a previously misclassified object is first
observed moving and is promoted to the dynamic path. It must not trigger
continuous static-world rebuilds afterward.

Also verify visually:

- E006 static environment remains coherent while the scripted camera moves.
- Moving cutscene actors animate/move without becoming part of the static
  environment cache.
- Fades and palette/VDP2 activity do not make world geometry disappear or
  regenerate.
- The sequence continues under Azel ownership into `TWN_EXCA.PRG`.
- Previously proven FLD_A3 and Ruins/town rendering behavior is not regressed.

If the residency correction fixes CPU rebuild/upload cost but the fragmented
visual remains, diagnose that as a normal Neptune submission/material/geometry
association problem using the same field/town pipeline. Do not introduce a
cutscene-specific renderer.

### Optimization experiment after correctness baseline

Once the E006 -> Excavation route is stable enough to serve as a correctness
baseline, perform a controlled compiler optimization A/B.

Current runtime optimization is selective: hot Neptune/audio units use `-O2`,
while the full runtime is not globally built with `-O3` / `-ffast-math`.
Shader compiler `-O3 -fastprecision` flags do not imply ARM runtime
optimization.

Recommended order:

1. Baseline current branch on hardware.
2. Add a build option for broad ARM `-O3`.
3. Preserve Neptune's strict floating-point behavior initially.
4. Hardware A/B the same Ruins -> FLD_A3 -> E006 -> Excavation route.
5. Only then test fast-math separately where it is demonstrably safe.

Do not bundle `-ffast-math` into the first optimization experiment because
camera/projection/interpolation and reconstructed Saturn math can be sensitive
to changed floating-point semantics.

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
## Optimized hardware capture and remaining drop audit - 2026-10-08

The 12:32:44 log confirms NeptuneBuild optimized=1 fieldDescriptorIndex=1.
Across 33 complete Full-mode windows (3960 measured frames), mean renderer time
is 9.751 ms, 5 frames exceed 25 ms, and 2 exceed 33.333 ms. The first window
includes 263.730 ms renderer time / 254.656 ms build at field entry. One later
window contains a 47.573 ms renderer / 40.038 ms outer-build outlier.
Across 3960 measured presentation intervals there are four gaps above two
vblanks: three in the first window (maximum six vblanks / 100.091 ms) and one
later (three vblanks / 50.053 ms). The other 31 windows have no missed two-vblank
intervals and present at the display's nominal 29.970 FPS. User reports near
flawless playback with only a couple of brief drops. This validates a substantial
performance gain but does not prove the strict minimum throughout the route.

FieldStream's build timer ends before logging; the outer build timer includes
synchronous vprintf and sceIoWrite. Outside entry, the largest detailed work
bucket is 18.006 ms, below the 40.038 ms outer-build outlier. Logging/scheduling
is a candidate, not a demonstrated cause. The next checkpoint stops printing
FieldStream solely for routine geometry-visible-set changes or new static
instance identities. First models/materials, texture growth, allocation, and the
120-frame heartbeat still report. Counters and per-frame timing windows remain.
maxFieldLog measures the actual FieldStream write time in each window so the
next hardware capture can test the explanation without per-frame diagnostic
writes. Renderer behavior and visibility are unchanged. Field entry allocation
and initial texture upload remain separate unresolved costs. Sky support remains
unfinished. Goal stays active pending another hardware capture.

## Shared sky, field opening fade, and startup allocations - 2026-10-08

The 13:06:15 hardware log contains 34 complete Full-mode windows (4080 frames).
Mean renderer time is 9.117 ms. Five frames exceed 25 ms; only the entry frame
exceeds 33.333 ms (264.856 ms renderer / 255.628 ms build). After the first
window, renderer maximum is 31.275 ms. Presentation still contains three gaps
in the first window and one later 50.047 ms / three-vblank gap. The remaining
32 complete presentation windows maintain nominal 29.970 FPS. maxFieldLog
reaches 9.904 ms. This is a strong steady-state result, but the later interval
and entry stall remain; logging is a candidate rather than a proven cause.

This checkpoint moves persistent-file and stdout writes to a lower-priority
worker. Producers format once and enqueue into a bounded 256 KiB ring; the
worker drains 16 KiB batches. Queue saturation reports dropped-message counts
instead of blocking a frame on storage. Shutdown drains after render/audio
workers stop. Initialization failure retains synchronous logging. Confirm
`[Log] ... async=1` in the next capture.

Field texture uploads now allocate 64-byte-aligned slices from shared mapped
GPU blocks instead of mapping one kernel block per texture. Pixel conversion,
filtering, descriptor identity and stable-prefix reuse remain unchanged. Blocks
are released with their texture cache. FieldTexturePool reports allocation
sizes and slab counts; the existing first-frame build/upload timers measure
whether this reduces startup cost. No preloading or cell visibility changes.

Native scene snapshots now carry the complete VDP2 VRAM/CRAM and RBG0 rotation
state, published under the same ownership boundary as the world. The existing
frontend RBG0 draw is shared with native town/field scenes. BGON controls layer
enabling, and GXM performs rotation, coefficient reads, character/palette fetch
and composition behind VDP1 geometry. RPMD=2 selects A versus B with coefficient
A's MSB, exposing the authored flight sky without any A3-specific map or camera
rules. Native RPMD=3 also supports rectangular/line window combinations through
bounded coverage geometry. This reuses the existing RBG0 cell-format renderer;
it does not claim complete emulation of every Saturn bitmap/coefficient format.

A field entry restarts a 30-presented-frame opening fade through the generic
presentation fade service, including reentry through menus/movies. This avoids
retaining the town's terminal fade-out or treating BACK-only CLOFEN as a global
black overlay. Each GPU color/fade pass now uses its own vertex slice so later
passes cannot overwrite a queued overlay's colors. Module and map selection
remain Azel-owned. No modifications to the pinned extern/Azel source tree.

The Vita runtime and native RBG0 shader build locally. Hardware validation of
this checkpoint is pending: check the opening fade and sky during the initial
camera pan, turn around and traverse the full route both ways, then compare the
first upload/build timings and every FlightPresentWindow. The performance goal
remains active until that capture verifies the remaining gaps. Earlier notes
about unfinished sky support refer to older checkpoints.


## Shared rendering requirements and working procedure - 2026-10-09

These requirements apply to future work on this branch and extend the latest
authoritative checkpoint above.

### One shared native 3D presentation path

Standardize native town, flight, and in-engine cutscene rendering on the methods
used by the proven flight path. Area type must not select an independent renderer
or require reimplementing presentation when another level is added. Preserve
Azel's ownership of gameplay, scripts, camera, visibility, fades, transitions,
text content, and timing; Lagi supplies shared services/bridges, and Neptune
renders the submitted scene.

The flight path currently lacks the opening area badge text and its background
(the badge above the canyon), LCS-associated text and backgrounds, and the
subtitle text and black bar already supported in Ruins town. Bring that existing
Ruins presentation support into the shared native presentation path so every
town, field, and in-engine cutscene can use it. Restore both text and authored
backgrounds through Azel's existing state; do not hard-code labels, subtitle
timing, or an area-specific overlay. These are requested implementation changes,
not a claim that the current build already provides them.

Preserve validated flight orientation/winding, radar point filtering, lighting,
static position validity, texture reuse, and geometry residency. Shared methods
must still consume the correct Azel scene state; standardization is not permission
to apply the town-only horizontal mirror to flight.

### Required edit, publish, local build, and log workflow

1. Read this document's latest authoritative checkpoint and confirm the branch
   and revision before continuing.
2. If the current build requires hardware-log analysis, stop dependent renderer
   changes and wait until the user supplies that build's log. Do not infer a
   hardware result from compilation or an older capture.
3. Once the required log has been analyzed, implement the next supported change,
   run appropriate available checks, and commit/push it to
   `feature/flight-mode-bringup`.
4. Give the user the published commit ID and concrete pull/build/install/test
   directions. The user performs the local Vita build and hardware run.
5. Wait for the resulting log before diagnosing the next hardware-dependent
   correction. Record the evidence and next checkpoint here.

The current renderer commits `c6fa3b4` and `2a6d1d4` still require the E006
residency/visual acceptance capture described above. This documentation update
does not validate those commits or implement the shared text/background changes.

For a clean local checkout on the existing feature branch:

```powershell
Set-Location C:\Dev\Lagi
git status --short
# If changes are listed, preserve them before proceeding; do not discard them.
git switch feature/flight-mode-bringup
if ($LASTEXITCODE -ne 0) { throw 'Branch switch failed' }
git pull --ff-only origin feature/flight-mode-bringup
if ($LASTEXITCODE -ne 0) { throw 'Pull failed' }
git submodule update --init --recursive
if ($LASTEXITCODE -ne 0) { throw 'Submodule update failed' }
git rev-parse HEAD

$env:VITASDK = 'C:\Dev\VitaSDK'
$env:PSP2CGC = 'C:\Dev\sdk\host_tools\bin\psp2cgc.exe'
$env:PATH = "$env:VITASDK\bin;$env:PATH"
cmake -S . -B build -DLAGI_OPTIMIZE_NEPTUNE=ON
if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
cmake --build build --parallel 32
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
```

Install `C:\Dev\Lagi\build\Lagi.vpk` using the existing Vita deployment
procedure. Replay Ruins -> FLD_A3 -> first-field door -> E006 -> Excavation.
Send the fresh `ux0:data/lagi/lagi.log`, the built commit ID, and observations;
include video/screenshots if geometry is fragmented or text/backgrounds are
missing. Evaluate steady E006 `staticRebuilt`, build/upload costs, moving actors,
fades, and the Azel-owned Excavation transition before the next renderer edit.
