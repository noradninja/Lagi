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
cmake -S . -B build-battle-vita -DLAGI_RBG_DECODED_PAGES=ON -DLAGI_RBG_COEFFICIENT_ROWS=ON -DLAGI_GPU_PASS_TIMING=OFF
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

## Exact recommended profile build verification (2026-10-21)

The earlier successful configurations bracketed the recommended options:
battle/cache ON with rows/timing ON, and menu/cache OFF with rows/timing OFF.
The exact recommended battle profile is now also fully compiled, linked and
packaged at source revision 1df2378 (runtime code unchanged since 7545157):

- FULLRES, STOCHASTIC_FILTER and DITHER_LOOKUP: ON.
- BATTLE_NATIVE_BUILD, BATTLE_LINK_AUDIT and BATTLE_EXECUTE_A3: ON.
- VDP2_SPRITE_COMPOSITE and RBG_DECODED_PAGES: ON.
- RBG_COEFFICIENT_ROWS and GPU_PASS_TIMING: OFF.

Local build-battle-dev artifact SHA256 values (not user-installed identities):

- Lagi.vpk: `7E1DD35BF8326504CAE626B7E0D228BC6F1F8C1B03A2B797B3750BC9D8441090`.
- lagi.velf: `94DA6B3BEDC52B69A7C305DB5318E6284D30178BEE41FB02B3B9AA57C98426DE`.

The remote branch was independently checked with ls-remote at 1df2378.
No further runtime change is made by this verification. A fresh capture with
the complementary-coverage/nbgNative markers, native gameplay pass/workload
timings and visual acceptance of water/transparency are still required.

## Fresh name-entry and water captures; rectangle-coalescing follow-up

The user supplied separate captures after 7545157:

- Name-entry log SHA256 `FA57641123EDF89CA24E698EDDF452643EC76FD9236B43783326792C01E3E48F`.
  Complementary coverage (112 A strips/112 B strips) and nbgNative=1 markers
  are present. Six keyboard-visible samples have median 44765 us, maximum
  46804 us; the one NBG-hidden sample is 41643 us. Three cache-hit records
  have median 21 us, maximum 22 us, with no uploads. The screenshot shows the
  reflected background and keyboard still present; this is static visual
  evidence, not proof of animation/palette correctness throughout the route.
- Water log SHA256 `D510DD27C6CC7B726EF57AB80C61B0AA6905EA21544B95D41AC66F1132B8729E`.
  The 34 RBG-enabled, non-menu, non-serialized ScenePerf samples have median
  render 27764.5 us, maximum 30294 us; none exceed 33000 us. Median task time
  is 7116 us, but it must not be blindly added to overlapping renderer work.
  Across 19 complete renderer windows/2280 frames, 2041 exceed 25000 us,
  two exceed 33333 us, and the maximum is 108020 us. Two baseline-less
  rebuild outliers are 64598/108020 us. Completed presentation windows have
  eight intervals over two vblanks; further outliers exist outside those
  complete windows. Thus the whole-route deadline is not established.
  The screenshot shows layered water, waterfall streams and impact splashes
  at a displayed 30 FPS; this does not erase the logged outliers.

The next change coalesces only horizontally identical, vertically adjacent
RPMD3 spans. D5's native geometry becomes two rectangles rather than 224
one-row quads; no pixel selection, coefficient, texture, palette, scroll or
shader precision changes. Both ranges are built/extended before either is
submitted to GXM. Six active slots per parameter prevent a new span from
evicting a later match from the previous row. No cross-frame geometry cache
or new GPU allocation is introduced. Non-rectangular windows remain exact.

The existing 2944-state/232128512-pixel differential test now rasterizes both
spans and coalesced rectangles and compares them. The actual renderer draw
fixture verifies the two D5 quads and 128 split/moving window cases, including
coverage exclusivity and native pixel-center UV mapping. This is host proof
of geometry construction, not a measured GPU win. New device marker:
`[RBG0WindowCoverage] ... aSpans=1 bSpans=1 ... coalesced=1`.
The 25/33 ms goal remains active; name entry and route outliers still need work.

The coalesced final tree links/packages in both the exact recommended native
battle/cache-ON/rows-OFF/timing-OFF profile and the menu/cache-OFF fallback.
Its device timing and visual acceptance remain pending. Native water scenes
using RPMD0/2 do not receive this RPMD3-only geometry optimization.

## Name-entry device acceptance after rectangle coalescing (2026-10-21)

The user reports "30fps now!" and supplies a screenshot showing the keyboard,
entered EDGE text, mirrored background and 30 FPS counter. The fresh lagi.log
SHA256 is `329245D4F4B29577E2AF91B972FCB95DF2A1376512F976233B6305865C8466D2`
(97018 bytes). It contains complementary coverage with aSpans=1, bSpans=1,
coalesced=1 and nbgNative=1, confirming the 5ff269d rendering path is active;
the log does not independently identify the installed executable revision.

All 22 keyboard-visible layout=2 samples are below 25000 us: median 23944.5 us,
p90 24305 us, maximum 24855 us. Final completion wait is included in these
render times, not additional work. Eleven logged cache hits take median 20 us,
maximum 23 us with no uploads; the initial page fill takes 23871 us and uploads
1 MiB. These are rate-limited samples, not every frame or cache operation.

Name-entry responsiveness is now hardware-accepted at the reported 30 FPS,
with sampled render work meeting the 25 ms target and the reflected background
retained. This closes the pending name-entry test above, not the broader VDP2
performance goal: this capture contains no ordinary-flight timing windows.
The earlier water capture's approximately 27.8 ms renderer median and route
outliers remain outstanding. This acceptance update changes no runtime code
and does not require rebuilding the tested executable.

## Next ordinary-flight experiment: coefficient rows, normal pacing

The earlier coefficient-row handoff command accidentally omitted the enabling
option. The corrected command above explicitly enables it and leaves serialized
GPU timing OFF, so the first capture measures normal pacing rather than
diagnostic stalls. Keep the accepted fullres/dither/native battle/compositor
options unchanged. This is an experimental comparison, not a default change.

For an existing Vita-configured build directory in PowerShell:

```powershell
cd E:\dev\Lagi
$env:VITASDK='E:/dev/VitaSDK'
$env:PSP2CGC='E:/PSVITA/sdk/host_tools/bin/psp2cgc.exe'
$env:PATH="$env:VITASDK/bin;$env:PATH"
git switch feature/battle-mode-bringup
git pull --ff-only
cmake -S . -B build-battle-vita -DLAGI_RBG_DECODED_PAGES=ON -DLAGI_RBG_COEFFICIENT_ROWS=ON -DLAGI_GPU_PASS_TIMING=OFF
cmake --build build-battle-vita -j 32
```

Capture normal flight through water, turns and a tunnel transition; an exact
replay is not required. Check water layers, animated palette, transparency and
clipping. The log must show `[RBG0CoefficientRows] active=1 reason=0` in an
eligible scene before attributing any speed change to this path. Reason 1 is
an ineligible/precision-ambiguous table, reason 2 an allocation/descriptor
failure; both retain the existing renderer. Name entry uses per-pixel
coefficients and is intentionally unaffected. Compare warm samples separately
from page fills, rebuilds and transitions, preserving presentation outliers.

To return to the accepted baseline, reconfigure with
`-DLAGI_RBG_COEFFICIENT_ROWS=OFF -DLAGI_GPU_PASS_TIMING=OFF` and rebuild.
Only if pass attribution remains necessary, make a separate short diagnostic
capture with GPU_PASS_TIMING=ON; do not mix its serialized samples with the
normal-pacing comparison. No visual or device performance win is claimed yet.

Local verification at runtime source 1a22f1d: the exact fullres/dither/native
battle/compositor/pages-ON, rows-ON, serialized-timing-OFF profile compiles,
links and packages successfully. Coefficient exhaustive/native-oracle,
production program-switch, page-service and diagnostic-timing contract tests
pass. Local artifacts (not the user's installed build identity):

- Lagi.vpk SHA256 `55F35C272C4C4460E983594981C3BCB283A44ECEBBE772296C07A0C14F540B14`.
- lagi.velf SHA256 `2756AADE28CFF4E5904B10C5AD60EC0847686DEDE8536264F26418910EF0F63B`.

## Fresh flight capture without coefficient-row activation marker

The next supplied lagi.log has SHA256
`CA4C59B13294C6B3E5AAE5146AF485AFD87E38310AB1361237C9524FB38EA090`
(494218 bytes). Decoded pages are active, but no RBG0CoefficientRows marker
appears. Production calls row preparation immediately after successful page
preparation; with the option enabled, its first eligible attempt reports either
activation or a fallback reason. Therefore this log does not establish that
the row-enabled executable ran, and must not be credited as a row-path win.

The 45 RBG-enabled non-menu, non-serialized renderer samples have median
26630 us, p90 27663 us, maximum 30753 us; none exceed 33000 us. The earlier
water capture's median was 27764.5 us, but these are different workloads, not
a controlled attribution. Median build is 3298 us, CPU preparation 8561 us,
final GPU completion wait 17525 us; nested/overlapping stages are not additive.

Across 23 complete renderer windows/2760 frames, 1824 exceed 25000 us, four
exceed 33333 us, maximum 108864 us. Presentation windows contain nine intervals
over two vblanks, maximum 50050 us. Rebuild outliers include 96992, 99004 and
108864 us; non-rebuild 50 ms presentation intervals occur even with renderer
work around 30.8-32.1 ms. Logged page fills take up to 62802 us, whereas 22
logged hits take median 25 us, maximum 27 us. Cold fills and warm throughput
remain separate problems. No serialized pass timings or new visual acceptance
were supplied. The 25/33 ms goal remains unproven; the next comparison still
requires explicit rows=ON configuration and the activation/fallback marker.

## Water/tunnel movement diagnostic (opt-in)

The E231D8E4140AFE12B32D90DB1031EF3C51858D7600DD9DB8C2F0E0593067B85F
capture does activate coefficient rows. Its 36 RBG-enabled renderer samples
have median 25578 us, maximum 28859 us; non-RBG median is 12313 us. Complete
presentation windows contain only one >2-vblank interval among 3120, so the
reported sustained water-motion judder is not explained by repeated deadline
misses. In the supplied 59.94 FPS video, broad scene-change detection at two
luma thresholds finds 239 consecutive two-frame change intervals in each of
the 1-9 s and 12-20 s water segments. This supports regular scene delivery,
not smoothness of individual player/camera motion or exact display latency.

Source inspection finds camera, player presentation state and RBG state copied
under the same acquired renderer slot. VDP2 is captured after native deferred
DMA; this is not evidence of a mixed-frame bug. Do not change native motion,
interpolate frames, or offset the water camera on this evidence alone.

`LAGI_FIELD_MOTION_TRACE=ON` adds two independent, bounded diagnostic streams;
it defaults OFF and does not change task scheduling or rendering. FieldMotion
records native fixed-point dragon position and view repeats, position L1 step
range, producer intervals and expected next publication serial. FieldSnapshot
records presented publication serial, exact view and RBG transform/coefficient
word repeats and presentation intervals. Each emits at most 24 summaries of
120 intervals, retaining the endpoint between windows. Key changes start a
fresh baseline. RBG word repeats do not compare coefficient-table contents;
stationary positions, unchanged transforms and changed input can be legitimate.
The position step is raw fixed-point Manhattan distance, not a physical speed.
Neither stream alone establishes the cause of judder. No per-frame disk log,
GPU readback or serialized GPU timing is added. Startup now reports the rows
and motion-trace build switches explicitly.

For a short diagnostic comparison, add `-DLAGI_FIELD_MOTION_TRACE=ON` to the
rows-ON/timing-OFF test configuration and rebuild. Fly/turn over water and then
in a tunnel, and send the log/video. Set the trace OFF for final performance
acceptance. Host tests cover exact repeats, sequence gaps, signed step extremes,
clock/key resets and inter-window interval accounting. Device diagnosis is
still pending; this is instrumentation, not a claimed judder fix.

## Name-entry exit: honor native RBG0 disable

D5's native fade-out task explicitly clears BGON bit 4 before the final
sprite sequence. The frontend previously continued decoding RBG0 regardless,
including VRAM subsequently repurposed by native tasks. RbgGpuState now retains
that enable bit. The native-sized target is still cleared and NBG can still
draw, but the disabled RBG pass and its page-cache preparation are skipped.
This does not invent a fade clock or suppress an enabled layer. Source-order
contracts and native NBG tests pass; fullres Vita linking/packaging passes.
Device exit validation remains pending. The separate D5 particle task still
needs its original position/update/transform behavior restored: the supplied
name-entry log records depth rejection, and original SH-2 code proves the
upstream positive-Z/random-velocity reconstruction is not faithful.

## D5 native particle adapter and layer order

Disc inspection of FLD_D5 060547F8-06054C26 establishes the original particle
initialization, wrap, update and draw path. The generated particle source now
uses a shared host header with the generated name-entry caller; extern/Azel
remains untouched. The adapter retains 64 particles, descending initialization
indices, original RNG ordering, negative object-space Z, native row positions,
angle-step/wobble, vertical wrapping, state 0/1 drift, state 2/3 contraction/
expansion and all 64 emissions during transitions. The local identity/Z-rotation
matrix precedes the existing native projection once. Sprite frames/palettes
come from the user's game resource, not committed data or substitute art.

Projected commands are drawn at native resolution between RBG and NBG0;
unprojected cursor/underline commands remain after the scaled keyboard/text.
Both passes share a monotonically allocated sprite-slot range so later cursor
vertices cannot overwrite queued particle vertices. Native-canvas coordinates
avoid applying the display's 4:3 aspect correction twice. Ordinary scene passes
are unchanged. The native name-entry exit sprite also has projected depth and
therefore reaches this same frontend path even after RBG is disabled.

The executable host fixture runs the production adapter with mock platform
services, testing RNG order, signed positions, wrap, state-service calls and 64
emissions. Layer/order tests are source contracts, not pixel proof; trig,
projection and interpolation remain the existing native services. Battle
fullres/pages/rows/trace-ON build links/packages successfully; the menu generic
fallback also links/packages with rows/trace OFF. Particle appearance, final
fade and exit garbage require a fresh name-entry completion capture. This
does not establish hardware acceptance or claim the whole fade pipeline is
complete: D5's BACK-only color offset and final sprite transition still need
on-device observation after restoring the missing sprites.

## Native name-entry action selection

The paired 09:03 Lagi/13:05 Saturn videos show a thin text underline used as
the Back/Space/Enter selection. MAIN 060235E0-0602363C explicitly selects
sprite record 3 there, whereas the reconstruction passed 0 and masked every
index to one bit. MAIN 060231A8 reads the four 0x1C-byte records from COMMON
00210768, including their leading animation bytes, texture, palette and signed
geometry. The generated adapter now reads those live resource records and
the native action origin at 00210830, rather than committing a replacement
sprite table. The underline/grid callers retain their distinct indices.
Production-reader tests cover all four records and signed offsets; the Vita
battle build links/packages. Appearance remains pending device verification.

## Frontend choice-window backing

The native drawBlueBox path writes 16x16 NBG1 cells at 0x5800. Native scene
composition already draws these resource-backed tiles, but the D5/movie
frontend drew only NBG3 glyphs. D5 now retains the full BGON snapshot and
calls the existing NBG1 atlas renderer only while that native plane is enabled,
before text/cursors. No window is synthesized from text bounds. Layer source
contracts and the production gameplay pass-partition fixture pass; the Vita
build links/packages. The latter fixture now recognizes the frontend pass
enumerators added with native particles. Hardware appearance remains pending.

## Native play-time VBlank service

The Vita loop never calls the desktop interrupt_VBlankIn; consequently Azel's
moduleManager_Update receives zero mC_numFramesPresented and the saved native
m3C_frameCounter stays at zero. Native save previews divide that counter by
0xE10 (3600) to obtain minutes, establishing 60 Hz time units rather than one
tick per task iteration. The platform now supplies elapsed monotonic 60 Hz
ticks before runTasks and clears the consumed input afterward. Azel retains
ownership of incrementing, resetting, loading and saving its counter. Fractional
microseconds are retained, so 20/30/60 FPS do not change clock speed. This does
not call the full VBlank interrupt or double-advance fades/VDP state.

The host fixture runs the actual upstream module-manager consumer and verifies
five-minute totals at all three cadences, fractional accumulation and clock
reset/regression handling. Vita fullres linking/packaging passes. Verify a new
save after at least one minute and reload it; previously lost play time cannot
be recovered from a save containing zero. App-suspend behavior is not tested.
# 2026-10-21 native fade correction

Device feedback confirms menu fades now affect sprites correctly (853544e).

Target-retirement follow-up: the shared target cursor draw callbacks now honor
the native 0x40000 retirement bit before emitting packets, including null target
references. Native task traversal calls Draw even when Update has just marked a
task finished, and also calls Draw while updates are paused. All three cursor
variants use the same guard, not a boss-ID or texture filter. Arachnoth's existing
death entry retires all three targetable sets. This closes packet emission after
retirement; a hardware death/reward capture remains required to prove no further
target-state producer is reviving indicators.

Follow-up: device confirms background fades, but VDP1 particles/selectors did
not fade. When CLOFEN enables all seven layers and CLOFSL selects a shared bank,
frontend composition now defers that native offset until after sprites and text.
RBG0/BACK do not receive a second offset. Selective layer/bank cases retain their
existing layer path; this is not a general implementation of selective sprite
offsets. Composition-order contract passes; Vita visual acceptance is pending.

The name-entry background/window tiles are device-confirmed correct. White
transitions remain pending device verification with this change. Earlier COEN
and half-scale interpretations are superseded: MAIN register reset preserves
the color-offset tail beginning at 0x110; native NBG1 setup case 45 writes the
CCRNA ratio high byte, not CLOFEN. Generated adapters now retain those semantics.
Native fade endpoints use -256..255 (negative times 16, positive times 17),
target-minus-current steps and arithmetic-shift RGB packing. Renderer offsets
no longer double native values. The D5 single BACK color is composed underneath
frontend layers and receives its own native color offset, including when RBG0
is disabled. This does not implement per-line BACK color tables.

Validation: exhaustive RGB555 round trips and signed component packing pass;
full-resolution stochastic/dither Vita battle VPK builds successfully. Hardware
white-out/white-in, title/movie fades and scene fades still require testing.
No changes to water motion are included in this commit. The supplied field trace
shows matched view/RBG changes without publication gaps in the first water
window, but that alone does not establish visually smooth movement.

