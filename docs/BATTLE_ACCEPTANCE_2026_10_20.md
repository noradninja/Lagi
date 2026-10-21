# Latest battle acceptance and follow-up

User confirms f138a83's compositor and previously missing water coverage work
on hardware. Preserve that route. Target renderer time is ideally 25 ms,
maximum 33 ms; builds and unit checks do not establish either deadline.

## Priority queue

1. Recover performance without removing restored effects/water.
2. Post-boss Edge dialogue must be a subtitle, not title-card presentation.
3. Remove weak-point indicators when the boss dies.
4. Depth-test cinematic dirt sprites against geometry.
5. Pre-boss cinematic must use its native boss music.
6. Restore Energy Prism trails, light and camera sequence.
7. Title Load must fade to black without displaying overwritten VRAM.

Each issue receives a separate commit. No effect-specific fake positions,
blanket desaturation or renderer-side gameplay decisions.

## Performance baseline and first optimization

The supplied log has 131 mode-4 non-menu ScenePerf samples: median render
30,412 us, p90 36,085 us, maximum 39,450 us; 19 exceed 33,000 us. Median CPU
preparation is 9,180 us and finish wait 19,328 us. Mode-1 non-menu samples have
median render 26,286 us with the new compositor inactive. Finish wait includes
scheduling/queued rendering; these overlapping counters cannot be added or
interpreted as an isolated GPU timestamp. Heartbeat samples include transitions
and results, not an exact-route controlled benchmark.

Specialize only the published RBG format CHSZ=16, CHCN=8bpp, bitmap off,
one-word/flip-enabled pattern names. The variant includes the generic shader
and fixes only these four format constants, eliminating unused 4bpp/8x8 and
alternate-name branches. Rotation, coefficients, screen-over, wrapping, tile
flip, palette, window and compositing math remain shared. Other formats and
the frontend continue using the generic program. Each program's uniform
offsets are reflected independently and restored when switching routes.

Skip the final-target clear only when the compositor will overwrite every
pixel; its VDP1 input surface still clears. Legacy routes retain their clear.
This removes redundant transfer/finish work without exposing old pixels.

The specialized GXP is 7,980 bytes vs 9,460 for generic; binary size alone is
not a frame-time result. Exhaustive format eligibility and existing composition
tests pass. Hardware timing recovery remains unmeasured. A fresh log should
include `[RBG0TileFormat] specialized8bit16=1`; retain compositor ON for the
water/combat comparison.

## Post-boss subtitle presentation

Native 060590AE..B6 passes the speech descriptor to 0606D68A, not the boxed
attack-name helper. 0606D7A4 writes directly to text region (3,25,38,4);
0606D9D0 clears that region at timer completion. Restore this text-only task
for the post-boss descriptor, keeping its original index/duration and leaving
other battle announcements unchanged. Parent-task teardown also clears the
subtitle. This changes presentation, not speech playback or its audio clock.
The disc-backed fixture verifies native region/call anchors and descriptor,
then runs the production task through display, duration and cleanup. Host
fixture and battle VPK pass; hardware positioning remains pending.

## Boss target retirement

Retire all three Arachnoth form target lists at death-state entry using the
existing native `0x40000` retirement flag and distance-sort invalidation.
Previously this cleanup ran only after the death dialogue finished. This is
an intentional lifetime correction after Azel decides death, not a recovered
original call at that address. The generic cursor task already stops drawing
when this flag is set; no renderer-specific boss filtering is added. Retain
the later cleanup as an idempotent safety net. The host fixture exercises all
three lists and repeated cleanup. Host test and battle VPK pass; device
confirmation of indicator removal remains pending.

## Compositor register-decision lookup

The follow-up log currently at the supplied path includes
`[RBG0TileFormat] specialized8bit16=1 genericFallback=0`. The earlier statement
that specialization was unconfirmed must not be treated as evidence that it
failed. Compositor-active steady ScenePerf samples remain over budget; their
finish waits include queued GPU work, not isolated per-pass timestamps.

Move register-only priority, selector, condition and ratio decisions from the
full-screen fragment program into a 256-entry RGBA8 point-sampled lookup.
Build it from the same frame's published Azel state after previous GPU work
finishes. Retain per-pixel RBG presence, final source/destination colors,
RGB555 quantization, exact /32 ratio and saturating addition on GPU. No change
to water coverage, mesh stipple, geometry or scene eligibility. Adds 1 KiB
payload (allocator granularity may be larger) and one dependent texture sample;
reduces per-fragment selector arithmetic/branches. This is a performance
candidate, not a measured speedup: device timing is still required.

The exhaustive host fixture compares every alpha byte across all conditions,
thresholds, background priorities and rotated bank priority/ratio values against
the previous shader's independent decision oracle. Existing RGB555 ratio tests
also pass. Expected new device marker: `register decision lookup ready`.

## Follow-up: lookup did not recover 30 FPS

User reports the same 20 FPS outside cinematics with 503ab29. The new log
contains both specialization and lookup success markers; do not treat either
as a measured optimization win. Sampled final GPU waits still reach 21-25 ms.
The 55 compositor-active non-menu, non-rebuild samples average 33,452 us
renderer time, 22,824 us final finish wait and 5,694 us game tasks. These are
mixed-scene heartbeat samples, not a matched A/B comparison. Nevertheless
they confirm the deadline is still missed and cannot establish a lookup win.

Add opt-in `LAGI_GPU_PASS_TIMING=ON`, default OFF. Every 120 eligible native
frames drain after the RBG resolve EndScene and after the world EndScene,
before starting final composition. The existing final finish supplies the
third wait. `[GpuPassTiming] serialized=1` reports CPU submission and remaining
wait separately for RBG, world and final passes. These are wall intervals,
not hardware timestamp queries; serialization changes overlap. Sample frames
may hitch and must not be used as ordinary frame-rate acceptance samples.
Any coincident ScenePerf heartbeat carries `gpuSerialized=1` for exclusion.
Non-compositor samples have no separate world boundary and report that fact
with `composite=0`; interpret three-pass attribution only for `composite=1`.
The next device log can distinguish dominant pass cost without removing
water/effects or guessing at another shader change.

PowerShell (preserve the existing Vita toolchain/build cache):

```powershell
git pull --ff-only
cmake -S . -B build-battle-vita -DLAGI_GPU_PASS_TIMING=ON
cmake --build build-battle-vita -j 32
```

Turn the option OFF after diagnostic capture. The source-contract fixture
checks default-OFF, sampling cadence and completed-scene barrier placement.

## Pass isolation and RBG memory placement

The supplied diagnostic capture has 20 mode-3 compositor samples. RBG remaining
wait is 13.7-15.35 ms, world 6.52-7.93 ms, final 7.07-8.12 ms. These serialized
wall intervals identify RBG as the largest pass, not the exact GPU execution
time or a controlled A/B result.

The RBG raw VRAM/CRAM texture was allocated in uncached system RAM. Prefer
CDRAM for this unchanged 512x258 RGBA byte image; retain the same generation
updates, format, shader and cleanup. Fall back to the prior system allocation
if CDRAM allocation fails. The 528,384-byte payload rounds to 768 KiB CDRAM
(vs 516 KiB system allocation). `[RBG0Memory] cdram=1` confirms placement only,
not a performance win. Source-contract test verifies allocation, fallback,
descriptor and cleanup ordering. Device timing remains required.

## Completed world surface memory placement (separate commit)

The completed VDP1 world image was also in uncached system RAM, unlike the
ordinary main color surfaces. Prefer CDRAM for its identical 1024x544 RGBA
payload, retaining the prior allocation as fallback. GXM surface/texture
descriptors, synchronization, clear, composition and resource cleanup are
unchanged. The 2,176 KiB payload rounds to 2,304 KiB CDRAM. No duplicate copy
or extra texture upload is introduced. `[VDP2SpriteMemory] cdram=1` confirms
placement; tests check fallback and descriptor ordering. Both allocation
changes need a device timing capture; neither is a proven speedup yet.

Keep `LAGI_GPU_PASS_TIMING=ON` for the next short diagnostic flight capture
to compare pass waits and confirm both placement markers. Turn it OFF for
subsequent normal frame-rate acceptance.

## D5 name-entry frontend source memory (separate commit)

Runtime status 2 publishes `frontend_set_rbg0_state` and frontend VDP2 layout
2 for the complete D5 name sequence. This is not the ordinary menu raster
path. Its raw 512x258 VRAM/CRAM source was also allocated in uncached system
RAM. Prefer CDRAM with system fallback in `frontend_present_vdp2` only; movie
RGBA, Cinepak and accepted disc-selector paths remain unchanged. Existing
frontend resource ownership, upload bytes, layout and shader are retained.
The title and D5 share this VDP2 source allocation; both receive the placement
change. Expected marker: `[FrontendVdp2Memory] cdram=1`.

Unlike the native scene RBG resolve at 352x224, D5 currently invokes the raw
RBG shader across its enlarged 4:3 frontend quad. That remains another possible
cost; this commit does not change sampling resolution or claim to fix name
entry's frame rate. A short name-entry log with `[VDP2Perf] ... layout=2`
will establish its remaining cost. Source checks verify the VDP2-only scope.

## Active goal: efficient native VDP2 restoration

Resolve the measured RBG bottleneck while preserving restored water/blending,
then use that efficient foundation for additional authored VDP2 effects.
Target ordinary gameplay frame time ideally 25 ms, maximum 33 ms. Source,
shader compilation and allocation success do not establish device acceptance.
Keep atomic commits and retain Azel-owned state; no proxy scene/effect rules.

After CDRAM placement, nine serialized samples average RBG wait 15,116 us,
world wait 5,405 us and final wait 7,691 us. Prior values were 14,852 / 7,216 /
7,779 us. Scenes are not exactly matched; the user reports some improvement.
RBG remains the dominant unresolved pass. All three CDRAM markers are present.

### Native 8-bit RBG byte-view candidate

Alias the existing 512x258 RGBA raw upload as a 2048x258 R8 texture. No extra
pixel allocation, copy, decode or upload. Only the eligible native 8-bit/16x16
variant uses this view for character bytes; coefficients, pattern names,
palette lookup, rotation, windows and final RGB remain unchanged. Generic
formats and frontend RBG stay on the packed path. Failed alias initialization
or variant creation retains the generic path.

The byte address matches the current packed decoder's lane-selection thresholds,
including fractional rotation coordinates and lane-3 clamping without carry.
Host fixture covers every uploaded byte and eighth-byte positions, row
boundaries and normalized point-sampling centers. Shader/descriptor contracts
and existing format tests pass. Hardware equivalence and performance remain
pending. Expected markers: `[RBG0ByteView] available=1 sharedUpload=1` and
`[RBG0TileFormat] specialized8bit16=1`. Keep pass timing ON for the next short
flight log, then disable serialization for final deadline/visual acceptance.

### Byte-view follow-up and compiler resource verification

The new device log confirms byte view and specialized shader are active.
Nine serialized samples average 14,820 us RBG / 5,452 us world / 7,657 us
final wait, versus 15,116 / 5,405 / 7,691 us before byte view. This does not
establish a meaningful performance improvement. Do not call the RBG goal
resolved or request unchanged reruns as if more samples alone fix it.

`tests/rbg_shader_resources_test.ps1` compiles generic and specialized shaders
using the installed Vita compiler with production precision/optimization flags.
It validates actual compiler XML reflection against all 22 runtime uniform
names, fp32:4/default-buffer types, raw sampler unit 0, specialized byte sampler
unit 1, and absence of that sampler from the generic/frontend program. Both
pass. This catches binding/resource regressions, not GPU appearance or timing.
The fullres/dither compositor-OFF/battle-OFF fallback VPK also builds after
062ec10; the earlier compositor-ON/battle-ON VPK passed separately.

Next pipeline investigation: cache native tile/palette source decoding with
exact VRAM/CRAM dependency invalidation instead of redoing it per rotated
output pixel. Keep coefficient tables, A/B selection, windows and composition
live on GPU. Upstream `renderer_vdp2.cpp::sampleTileAtCoordinate` consumes
integer source coordinates after fixed-point rotation (lines 1179-1216), so
compare any decoded-page implementation with that source, not merely the
current float packed-sampler's fractional-lane behavior. Existing generation
tracking must distinguish source-page/character/palette dependencies from
changing coefficient blocks. This pipeline is not implemented or accepted yet.

### Decoded-page resource component (not yet connected to rendering)

`include/lagi/rbg_decoded_page.h` implements a native 512x512 source page for
16x16, 8-bit, one-word flip-enabled pattern names. It retains SCN character
extension, four flip modes, CAOS/palette wrapping and transparent-dot behavior.
It outputs normalized RGB555 in RGBA8, matching the current RBG resolve's
color representation rather than changing the five-bit color content.

Cache validity records exact dependencies at the renderer's existing 4 KiB
generation granularity: pattern-name page, referenced character spans, CRAM.
An unrelated coefficient-block change is a hit; referenced writes rebuild.
Invalid/unsupported addressing returns Unsupported and never marks a partial
decode ready. Allocation and GPU scene policy remain outside the component.
This implementation conservatively invalidates on any CRAM block change;
fine palette-bank invalidation remains future work if animated palettes make
that necessary. Caller must verify the fixed supported CHCTLB format.

`tests/rbg_decoded_page_test.ps1` extracts the actual upstream
`sampleTileAtCoordinate` into a generated scratch fixture (no upstream edits
or committed duplicate). Pixel comparisons cover 64 SCN/CAOS configurations,
all page pixels, authored-pattern flip/palette combinations and transparency;
five-bit colors match after RGBA channel-order/normalization conversion.
Separate checks exercise opaque zero, dependency hits/rebuilds and safe
unsupported-state caching. Host tests pass. No device rendering or performance
change is claimed until the GPU route is integrated and tested on hardware.

### Opt-in indexed-page GPU route

`LAGI_RBG_DECODED_PAGES=ON` connects the resource cache to a separate RBG
fragment program. Default remains OFF for a controlled device comparison.
The live route stores source palette indices and transparent-dot coverage,
not baked colors. CRAM contents and CAOS bank selection remain live on GPU;
neither triggers a full resource-page rebuild. Rotation coefficients, A/B
selection (RPMD), layer/parameter windows, native screen-over clipping and
final sprite/RBG color calculation continue through the existing pipeline.

The bounded first allocation is a 1024x1024 RGBA8 CDRAM atlas (4 MiB) holding
four 512x512 pages and a 1 MiB CPU scratch page. Source page/pattern/character
writes invalidate referenced slots using existing 4 KiB generation counters.
Unrelated coefficient blocks do not. Allocation failures (including scratch),
unsupported source addressing or more than four distinct required pages use
the raw renderer for the whole frame. There is no dropped-page approximation.
OVPNR screen-over mode 1 also falls back until its single-pattern resource is
cached; modes 0/2/3 are supported. The captured flight uses RPMD=2, PLSZ=0000,
mapA=60000/mapB=60800. No game-mode-specific layout or water position is used.

All active A/B pages are validated before the route becomes ready. Required
surviving slots are reserved before eviction. Native plane uniforms encode
four two-bit cache slots only in the cache shader; generic/byte-view shaders
retain physical VRAM addresses. Each shader reflects its own 22 uniforms and
restores the generic bindings on fallback/frontend draws. Palette indices are
decoded and colored from current CRAM after sampling the cached source image.

Telemetry: `[RBG0PageCache] active=1 reason=0 pages=... rebuilt=... hits=...`
reports preparation, uploads and cache behavior. Reasons 1/2/3/4/5 mean format,
layout/capacity, allocation, source-address decode, OVPNR respectively.
`[RBG0TileFormat] ... decodedPages=1 genericFallback=0` confirms the matching
shader was created. A ready resource cache alone does not prove shader success.
Rebuild/upload telemetry includes cold-cache cost; ordinary warm-frame deadline
acceptance must still include preparation plus game tasks, not only GPU waits.

Tests execute the production cache service with mock allocations/textures to
exercise cache hits, live palette/bank changes, referenced character rebuilds,
surviving-slot reservation, four-page encoding, capacity rejection, unsupported
addresses, allocation failure and cleanup. Upstream pixel-oracle comparisons
cover the indexed path with live CRAM reconstruction in addition to RGBA mode.
All three actual compiled shaders pass uniform/sampler reflection checks.

## Name-entry RPMD3 coverage lifetime

The fresh name-entry capture reports 189248 us/frame, including 185241 us
GPU wait (layout=2). The screenshot's lower corruption has a concrete service
bug: `frontend_present_vdp2` calls `freeMovieResources`, which releases the
RPMD3 window geometry, but previously did not recreate it. The draw routine
then skipped window generation and submitted A fullscreen over B. The raw
frontend now allocates and validates that coverage itself. Missing coverage
fails submission rather than substituting fullscreen A. Disabled 0xFFFF line
windows emit no inside span (previously a one-pixel A column).

No coefficients, rotation, palette, layout, or upstream Azel sources change.
The source-contract regression test is `tests/frontend_rbg_window_test.ps1`;
it is not proof of Vita pixels. A fresh name-entry screenshot is required to
accept the reflection. Performance remains a separate unresolved issue.

## Frontend RBG native-size resolve

The raw VDP2 frontend now shares the native 352x224 GPU resolve target and
pass implementation with gameplay. Its mapped vertex allocation contains a
second fullscreen quad for that target; the existing 4:3 presentation quad
is unchanged. Live frontend state, coefficients and window selection still
run every frame, using the generic shader (no scene cache contamination).
Only completed RGBA is scaled with the existing presentation filter.
Raw VRAM/CRAM is rebound before NBG, and text/cursors remain separate. The
RBG-only color offset runs in the resolve, not twice. Title, selector and
Cinepak retain their own paths. No upstream Azel sources or game data change.

`tests/frontend_rbg_resolve_test.ps1` checks source wiring, not GPU execution.
Expected device marker: `[FrontendRbgResolve] native=352x224 ...`.
Compare `[VDP2Perf] ... layout=2` against the 189248/185241 us frame/wait
baseline. No speed or reflection acceptance is claimed without that capture.

Validation: battle/cache-on and menu/cache-off Vita configurations both link
and package successfully with FULLRES and the existing dithering enabled.
`frontend_rbg_draw_test.ps1` extracts and executes the actual production draw
routine against mocked GXM: captured D5 state submits B fullscreen, then A
for exactly 112 rows; disabled inside windows submit B only; outside windows
cover all pixels without overlap; missing coverage storage fails before draw.
Program-switch and page-service executable regressions also pass. These tests
prove CPU submission behavior, not the device reflection or timing target.

## Opt-in gameplay coefficient-row predecode

`LAGI_RBG_COEFFICIENT_ROWS` defaults OFF. With decoded pages active, tables
whose per-X step is exactly zero can be decoded once per native row into a
4096-byte RG32F texture. Values keep full signed 15/24-bit precision and the
invalid bit. Both fused and unfused float index calculations must agree with
the exact accumulator floor for all 224 rows; ambiguous tables fall back.
Per-pixel tables (including D5) are unchanged. GPU projection, RPMD selection,
window coverage and palette remain live. Coefficient words are reread each
frame, not cached across register/table changes. Allocation/descriptor or
program failure preserves the existing page/raw shader fallback.

Exhaustive host testing covers every 16-bit and 24-bit payload; the latter is
compared against the actual extracted upstream decoder. Production-service
tests cover wrap, disabled/unused tables, live updates and failure cleanup.
Actual shader compilation/reflection validates the three removed accumulator
uniforms and the new TEXUNIT2 sampler; program-switch tests verify restoring
all fallback bindings. This remains an experimental unmeasured option, not
a claimed device speedup. Marker: `[RBG0CoefficientRows] active=1 reason=0`.
Device pixel equivalence, warm/cold behavior and performance are still pending.
Final fullres/dither Vita VPK builds pass for battle/compositor/cache/timing ON
and battle/compositor/cache/timing OFF. The latter retains the generic frontend
and menu fallback compilation path. Neither build substitutes for device tests.

```powershell
git pull --ff-only
cmake -S . -B build-battle-vita -DLAGI_RBG_DECODED_PAGES=ON -DLAGI_GPU_PASS_TIMING=ON
cmake --build build-battle-vita -j 32
```

Capture a short flight after the initial cache fill, checking water geometry,
palette animation, transparency and clipping. Keep the native scene/menu
fallback paths available. The goal remains active until device measurements
and visual acceptance prove the requested result; broader cache capacity,
OVPNR optimization and efficient remaining VDP2 effect coverage are not yet
complete.

### Shader-transition fallback audit

`tests/rbg_program_switch_test.cpp` executes the production shader-selector
include against mocked GXM program/reflection calls. It checks all 22 active
uniform pointers through generic -> byte -> cache -> generic -> cache -> byte
switches, including frontend/unsupported selection. It also injects a missing
cache uniform and fragment-program creation failure, proving restoration of
generic bindings and cleanup of registered resources. Host fixture passes.
This supplements actual compiled-resource reflection tests; it is not a GPU
pixel/timing test and introduces no new device-rendering change after 8084b3c.
The available Downloads log still has no cache-enabled marker. Hardware cache
hit behavior, water correctness and the frame deadline remain unverified.

## Frontend cache follow-up after device reflection acceptance

User confirms the name-entry background is now visually correct on 2005c2a.
The supplied log records four early layout=2 frames at 51691-53465 us
(GPU wait 46797-48732 us), then eight later frames at 76878-80458 us
(GPU wait 72246-75758 us). The native resolve fixed the lower image and
substantially reduced cost, but did not meet the frame deadline.

The same bounded indexed-page cache now accepts a published frontend source
as well as a native gameplay source. Frontend VRAM/CRAM generations are
tracked from the producer's ordinary cached-memory input, never CDRAM
readback. Switching producer domains resets cached page validity even if
their numeric generation counters match. The 4 MiB atlas is reused, not
duplicated. Per-pixel D5 coefficients, palette, rotation, wrapping and
windows stay live; the row-coefficient option is not used for frontend.
Unsupported layouts/allocation failures retain the raw generic path.
The existing LAGI_RBG_DECODED_PAGES flag controls both domains; OFF retains
the previous device-accepted frontend resolve.

Production cache-service tests cover equal-generation domain switches and
the captured D5 RPMD=3/PNCR=8008/shared-60000-page format. Coefficient and
palette changes preserve atlas hits; character changes rebuild. Frontend
source/lifetime checks and actual draw-coverage tests pass. Both Vita
configurations are compiled separately; no device speedup is claimed yet.

Expected capture: `[RBG0PageCache] active=1 reason=0 ... frontend=1`, then
stable `rebuilt=0`, `uploadBytes=0`, and `[VDP2Perf] ... layout=2`.
Build the existing Vita cache with decoded pages ON and GPU timing OFF;
leave LAGI_RBG_COEFFICIENT_ROWS OFF to isolate this name-entry test.

## RPMD3 complementary coverage

The next supplied cache-enabled device log confirms a one-page frontend
cache: initial fill 23679 us, subsequent hits 20-21 us with zero upload bytes.
Layout=2 frames have two plateaus: 33019-34298 us and 61337-65265 us. The
remaining display-resolution keyboard decoder is a separate next target;
these measurements do not include the changes below.

RPMD3 now submits mutually exclusive A/B geometry, matching the upstream
CPU renderer's parameter selection. Previously B shaded fullscreen before
A overlaid its selected region. This also let transparent A pixels expose
B, rather than the underlying lower layer. Source windows now partition
the native raster into intervals once per row. Both parameters get immutable
geometry ranges; no already-submitted vertices are overwritten. There is no
scene-specific split: rectangular/line windows, area bits, AND/OR selection,
disabled windows and wrapped line-table addresses remain live native state.

For captured D5 coverage, the shader domain drops from 118272 to 78848
native pixels (one third fewer invocations); this is not a measured time win.
The host differential fixture checks 2944 window states and 232128512 pixel
decisions against the prior general predicate, requiring complete exclusive
A/B coverage. The actual production draw fixture verifies both 112-row
parameter submissions. Marker: `[RBG0WindowCoverage] complementary=1`.
Device visuals/timing are still required.

## Native frontend NBG resolve

After cache hits, the remaining raw keyboard decoder was still running at
enlarged display resolution. NBG now draws into the same native 352x224
resolve after RBG's own color-offset pass, before final scaling/text/VDP1.
It uses the unchanged native-map shader and live BGON visibility/scroll/
VRAM/CRAM. Disabled NBG submits no draw. The expensive raw decoder is removed
from the display-resolution scene; title/selector/Cinepak and gameplay NBG
paths are unchanged. Completed RBG/NBG pixels share the existing presentation
dither; keyboard edge filtering therefore requires visual acceptance too.

No extra surface or texture allocation is required. The production NBG
submission fixture verifies live uniforms, native mapped quad, raw source,
title exclusion and submission failures; source-order tests require NBG
after RBG/color offset and before EndScene. `[VDP2Perf]` now includes `nbg=0/1`
to identify visibility in the two timing plateaus without serialized probes.
`[FrontendRbgResolve] ... nbgNative=1` identifies this build path. The supplied
33/61-65 ms measurements predate both this and complementary coverage, so
neither change has a device time or visual acceptance claim yet.

Final-tree validation: battle/cache-on and menu/cache-off Vita builds both
link/package successfully. The raw title fallback remains available during
Azel's high-resolution setup, before decoded TITLEE is ready. Native NBG,
window partition, actual draw, page-service, coefficient, program-switch and
compiled shader-resource tests all pass. The final device capture is pending.

## Acceptance-log scope audit

`tools/analyze-vdp2-log.ps1 -LogPath <capture>` produces a non-invasive JSON
report. It keeps layout/framebuffer/NBG visibility groups separate, preserves
unknown legacy visibility instead of inferring it, separates cache fills from
hits and source domains, and isolates serialized diagnostic samples. Native
renderer/task/preparation times remain separate: tasks and nested CPU stages
can overlap, and final completion wait is already included in render time.
Completed field renderer/presentation windows and incomplete-tail outliers
are retained through the existing flight analyzer.

The report applies exact 25000/33000 us sample thresholds. It never treats
heartbeat samples, renderer-only windows, or 30 Hz pacing as proof of the
whole-frame/whole-route objective. It records the log hash and build markers,
rejects a log modified during analysis, and explicitly requires device build
identity and visual acceptance separately. Missing counters remain null.

The current e356619-era log has nine layout=2 samples: median 61337 us,
maximum 65265 us; every sample exceeds 33000 us. Four logged cache hits have
median 20.5 us, maximum 21 us, with no uploads. It has no native ScenePerf or
completed field windows and no newer nbgNative/complementary-coverage marker,
so it cannot validate 7545157 or ordinary gameplay deadlines. Synthetic and
empty-capture analyzer tests pass. This tooling commit changes no runtime;
7545157 remains the pending device-test renderer.
