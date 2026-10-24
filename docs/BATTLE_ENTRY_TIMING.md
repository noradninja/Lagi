# Battle-entry timing investigation

## Hardware report: 191f844

The user corrected the tested revision to **191f844**. The Downloads log
written 2026-10-24 01:28 contains six completed random encounters (sub IDs
7, 0, 6, 7, 0, 6). The user reports smooth 30 FPS after loading, including
multiple instances of the same enemy. This does not validate the later
8fc1bea CPU-storage optimization; be4fd3b only adds cadence tests.

First-publication renderer outliers at those entries are 73.153, 69.042,
68.790, 71.598, 69.244 and 65.316 ms. RBG preparation contributes
26.239, 26.277, 26.180, 26.561, 26.046 and 25.680 ms respectively.
These are overlapping component timings, not independent costs to sum.
The log has no scene-fade RGB on these frames, so it cannot prove whether
the expensive publication was entirely hidden by white.

## Current reconstructed source sequence

- `battleOverlayTask_Init` starts both fades toward white over 0x3C.
- Overlay draw case 0 sets graphicEngineStatus.m4; case 1 starts the
  authored overlay and font loading. There is no explicit fade-stopped
  check in this function. Do not assume every load already waits for white.
- `battleMainTask_Draw` runs music setup, waits for sound readiness, then
  resets the engine/camera/grid and invokes battle resource/engine setup.
- `battleEngine_Init` initializes environment/actors and sets fade0 to
  white. Battle intro submode 1 waits several updates, then requests the
  0x5A white-to-neutral fade and starts the intro task.

This is a source audit of the current reconstruction, not proof of original
Saturn instruction ordering. Do not add an arbitrary delay or advance a
second fade clock on that basis. Preserve native decisions and investigate
the actual publication boundary first.

## Original MAIN cross-check

The subsequent disc audit resolves the task descriptor at 06042D50 to init
06006814, update 060068AC, draw 060068D8 and delete 06006956. Original init
0600682A..06006852 requests current-to-FFFF over 3C for fade0 and fade1.
The original draw callback has the same case-0 state increment, followed by
case-1 overlay load/fonts/entry. At 060068FA..06006912 it looks up the
overlay PRG in COMMON 002005E0 and calls 0601016C with destination 06054000.
There is no explicit fade-complete check between dispatch and that call.
Thus absence of such a check in the reconstructed callback is not itself
a mismatch. Adding a wait there is not justified by this evidence.

`tests/battle_entry_native_audit_test.ps1 -DiscImage <US Disc 1 BIN>` checks
the original descriptor, both fade call sequences and the complete draw
instruction stream against the disc. The existing battle-main-task test
separately executes the reconstruction's sound-wait/reset/engine order.
Neither test proves original interrupt advancement during blocking CD reads,
the complete original loader implementation, or wall-clock fade cadence.
Those distinctions matter: the native CD loader is not equivalent to host
file reads plus deferred GPU resource preparation on a later publication.

## Diagnostic addition

Existing FlightPresentOutlier records now append fadeR/fadeG/fadeB from
the renderer's published g_sceneColorOffset, the same values used by scene
composition. Positive values clamp to 255 in the additive pass: all three
at least 255 imply full white for that pass. This is not a measurement of
every earlier CPU load or proof of display scanout timing.

Existing bounded RBG0PageCache records append decodeUs (page validation and
decode combined) and atlasCopyUs (scratch-to-CDRAM row copies). They retain
the existing log budget and heartbeat; no additional log lines or effects
are introduced. Timers add two clock reads per required page and two per
rebuilt page. Total prepUs still includes other service work and allocation.

Host page-service tests exercise the actual production include, including
mock-clock attribution on two rebuilt pages. Native pixel-oracle tests
check output. Combat profiling tests check that the logged fade values
match the composition source. None proves a speed improvement or Vita
transition smoothness. Goal 1 remains active pending hardware evidence.

## Shared cold-decoder work reduction

The next candidate replaces per-output-pixel native character flip arithmetic
with an immutable, compile-time 4 x 256 byte address table. Palette wrapping,
indexed/RGBA selection and transparent dot-zero handling are resolved in the
existing 2048-entry color table once per rebuild. The table still occupies
8 KiB of local storage; the immutable offsets add 1 KiB. No persistent page,
actor or encounter cache is introduced. Indexed pages retain GPU-live CAOS
and CRAM, and unchanged pages still use the existing generation hit path.
Dependency collection, exact tile-name memoization and unsupported fallback
are unchanged. GPU composition, rotation, fades and simulation are untouched.

Validation includes the actual Azel pixel oracle plus exact baseline RGBA/index
bytes for repeated/unique names, four flips, all eight CAOS values, opaque and
transparent zero, and indexed/nonindexed output. The work fixture compares
against fc94097, alternates measurement order, and forces 192 cold rebuilds
per implementation and workload. One host run measured baseline/candidate
24.374/13.749 ms (repeat 1), 155.038/30.335 ms (repeat 73), and
160.958/31.665 ms (repeat 1024). These are totals on the Windows host, not
Vita frame times or a promised speedup factor. Hardware must confirm entry
timing, fade coverage, and unchanged water/name-entry presentation.

## b90d201 hardware result and next shared preparation change

The fresh user-supplied log (2,318,228 bytes, host file timestamp 02:01)
contains eleven completed fights plus a partial final encounter. The user
reports all combat frame rates excellent once loaded. Complete-span window
rates range 28.771..29.970 FPS, with 91/9720 intervals over two VBlanks;
these spans include intros/results, not only active combat. Encounter render
medians are 23.321..28.021 ms. This is good steady-combat evidence, not proof
that every frame meets 33 ms or that ideal 25 ms is consistently achieved.

Cold entry frames report RGB fade (255,255,255), confirming full-white
composition during those renderer stalls. Visible fade-in frames still show
50 ms presentation intervals, render 31..33 ms, CPU preparation 13..16 ms,
and background preparation roughly 0.2..0.3 ms. Thus repeated cold page
decoding is not the cause of those later fade-frame misses. The early rebuilt
page records show decode 11..13 ms and atlas copy about 13 ms. Do not claim
the host decoder benchmark's speedup factor materialized on Vita.

The next candidate removes eager screen-coordinate projection from shared
live Gouraud visibility preparation. The GPU subdivision, lighting and
textured paths consume only homogeneous visibility there. The single
nonsubdivided Gouraud screen-payload consumer now requests projection lazily,
using the exact original reciprocal/multiply order and the same WVP. Each
preparation resets payload validity, preventing reuse across frames/views.
Visibility/culling, geometry, shader selection, fades and simulation remain
unchanged; diagnostic/fallback screen payload is still available.

live_visibility_test extracts the actual production functions and b90d201
baseline. It verifies 16,112 synthetic quads (2497 visible, 13,615 hidden),
exact visible fallback XY bytes, identical visibility, full/half resolution,
near/behind-camera cases, invalid layouts, and repeated lazy requests. The
fixture also guards the single screen-copy consumer. The subdivision route
fixture passes. These are correctness checks, not a measured timing gain;
fresh Vita evidence must establish fade-in behavior and rendering correctness.

## Supplied a414440 video: outgoing cover failure

The next supplied log is 841,891 bytes. A close review of the supplied
`2026-10-23 22-58-51 - Lagi - Lagi` video, sampled at eight frames/second
around 23..30 seconds, shows the flight scene disappearing into a dark/gray
empty screen, held for roughly two seconds, followed by white and battle
fade-in. That is not a smooth outgoing scene-to-white fade. Log lines
1346 onward show fade count 59/color (4,4,4), immediately followed by two
BTL_A3.PRG reads, font/overlay/music setup and actor resource loading.
The later first full scene render at white does not cover those earlier
blocking operations. Previous interpretation of renderer-only outliers was
therefore incomplete; the user's video establishes the missing visible phase.

Platform correction: start a loading cover at successful native battle graphics
capture (and at overlay init for entries without field capture). While the
native field task is paused, preserve the prior immutable geometry/UI snapshot
and camera instead of publishing empty submissions. Continue normal frame/fade
publication. Delay synchronous preflight/file/font/overlay work until Azel's
primary fade is stopped at full white. There is no invented frame delay or
second fade clock. Retain through sound/resource loading until the native battle
engine exists and produces geometry; replace the outgoing snapshot then, under
the engine's own white cover. Azel retains the existing intro/fade-in sequence.

This barrier is a Vita loading/presentation adaptation justified by the supplied
visual sequence and synchronous host I/O. It is not a claim that original MAIN
has an explicit fade-complete test in its overlay callback; the original
instruction audit above still stands. A native interrupt-driven blocking loader
and this platform's frame-driven fade updates are not interchangeable.

Tests execute the production cover state, actual immutable publication block,
and generated overlay Draw. They check no early font/factory calls, incomplete
RGB rejection, retained geometry/UI/model ownership, continued frame serials,
empty-frame hold, engine-readiness gating, incoming replacement, repeated
encounters and existing graphics restoration. Both Vita configurations build.
Hardware must verify the complete outgoing fade, white hold, incoming fade,
normal HUD/particles/camera, audio and repeated random/story entries. This is
not declared visually fixed until that test; Goal 1 remains incomplete.

## a2bdf46 loading sequence accepted on hardware

The subsequent 1,632,905-byte log contains eight completed encounters, each
with `BattleLoadingCover loadUnderWhite=1` before overlay startup. Fade traces
show 59..0 advancing to (255,255,255) and stopped before synchronous loading.
The long 1.85..1.92-second publication intervals end under full white. The
user confirms the transition now looks and behaves as expected and explicitly
accepts this fix. Do not reopen the accepted loading sequence without new
regression evidence.

Combat render medians in this run are 26.963..27.610 ms. The eight complete
spans contain 125/6720 intervals exceeding two VBlanks. The span windows
include covered loading, intro and results, so the 20.09 FPS minimum is not
evidence of sustained combat slowdown. The user reports excellent combat
frame rates. Ideal 25 ms and an absolute 33 ms maximum are nevertheless not
proven; outlier records include roughly 50 ms intervals both at neutral color
and during visible offsets. Earlier wording suggesting only fade-in misses
remained was too narrow. Neutral color does not independently identify an
active-combat submode, either.

The analysis tool now exposes OutliersByFadeEndpoint (White, Black, Neutral,
VisibleOffset, Unknown), including record counts, timed interval count, maximum
interval and sampled outlier render median. It parses signed RGB values and
keeps legacy absent fields unknown. Baseline-zero records are not timed
intervals. No intervals are subtracted from original window/FPS statistics.
The endpoint color cannot prove that the entire preceding interval was covered;
the accompanying fade chronology/user observation supplies that context here.

The user also agrees that authored fade/loading/fade handoffs should follow
this principle elsewhere. General transition cleanup should retain the outgoing
presentation until the authored cover is reached, load/prepare while covered,
and hand off to the ready native destination. Preserve native colors and timing;
do not impose battle-white behavior on black fades or crossfades. This is not a
claim that other transition routes are wired or hardware-validated by this fix.
