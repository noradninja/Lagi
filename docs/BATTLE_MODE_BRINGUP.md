# Native battle-mode bring-up

Branch: `feature/battle-mode-bringup`, based on accepted menu milestone
`c2cef69`. Goal: working native battle entry, combat and exit—not a substitute
battle scene or compile-only completion. Azel decides. Lagi services. Neptune
renders. Do not edit `extern/Azel` or regress accepted frontend/flight/menu paths.

## Entry audit

The default Vita generation deliberately disconnects battle in these places:

1. `kernel/moduleManager.cpp` overlay dispatch slot 4 is replaced with null.
2. `battle/battleOverlay.cpp` startup calls for BTL overlays and debug teardown
   are removed by the source adapter. The explicit A3 option below reconnects
   only the real `BTL_A3` encounter entry, not other overlays/debug teardown.
3. Generic battle-data initialization is removed from generated `PDS.cpp`.

Do not merely populate slot 4 and call this complete. Native `loadBattle`
is in `mainMenuDebugTasks.cpp`; manager/debug-list/overlay tasks and the actual
field loading transition require a complete ownership/call-order audit.
`battleManager.cpp` itself contains debug behavior and unfinished Delete.
The real `loadBattle()` reads signed battle/subbattle IDs from COMMON
`0x212F96 + 2*r5`, resets temporary allocators and calls `loadBattleOverlay()`.
The latter requires an already-live `gBattleManager`, remaps the authored IDs,
rejects an empty PRG name, then creates the overlay task. Restoring slot 4
alone does not enable ordinary encounters. `moduleManager_Init()` already
creates the manager with `createBattleManager(pWorkArea, 1)`; the generated
module retains that call. Do not create a second manager or introduce lazy
ownership to compensate for a missing audit.

The normal path is `fieldEncounterTask.cpp::encounterTaskUpdate`: pause field,
back up graphics, run loading setup, then call `loadBattleOverlay(mC,
battleIndex)` directly in state 3. It does not traverse module dispatch slot 4.
`startBattleTutorial()` uses this encounter route unless the field debug bit
is set. Only its debug branch creates `field/battleStart.cpp`'s loading task.
Read-only original FLD_A3 inspection confirms that task really tests debug
keyboard toggles; replacing it as the ordinary gameplay path would be wrong.

The experimental entry uses the graphics/resource handback detailed
in [BATTLE_GRAPHICS_HANDOFF.md](BATTLE_GRAPHICS_HANDOFF.md). The generated battle
build now uses typed graphics capture/restore at both return sites and discards
storage on encounter teardown. Shared video-file spans are tracked and staged
for reload before sound cleanup, including menu backup ownership scopes;
heap-owned bundles are preserved. Upstream remains untouched. Host service
tests and both full VPK builds pass, but do not prove native battle/field
execution or cover every shared-resource path that live integration may expose.

`BTL_A3.cpp::overlayStart_BTL_A3()` loads its real overlay data, selects
`gCurrentBattleOverlay`, and constructs `createBattleMainTask()`. That task
waits for native sound loading before resetting 3D/camera/grid and invoking
the A3 initialization callback. A3 then constructs the battle engine using
authored overlay data. Preserve that pipeline and real encounter/formation
selection rather than hardcoding a different encounter.

Battle generic initialization remains stripped from ordinary boot. The A3
execution option initializes it at overlay preflight, before native fonts and
engine creation; its native constructor reads `BTL_A3.PRG`.
Task allocation already zeroes work areas through
the native task base; audit that contract before inventing extra state resets.

## Ray/resource services: retained-root link now passes

The three initial link gaps listed below are restored. The full-resolution,
dither-enabled retained-root build now produces a VPK. The ELF test checks
actual retained A3 entry, engine, HUD, radar, commands, results and service
functions. This is link evidence, not battle execution.
The default battle-disabled full-resolution VPK was also rebuilt successfully
after the shared file-service change; both dithering flags remain enabled.

The ray service uses the pinned native fixed-point projection body copied
mechanically into a generated include. Verification normalizes line endings
only. Native distorted-sprite/Gouraud packet fields and zero bucket are
preserved, with capacity guards and separate Neptune depth metadata. World
endpoints use the actual current matrix; existing flight ray helpers are
unchanged. Tests compare 1,024 packet cases with upstream and reject full
buffers/null colors. Source equality and Vita compilation do not prove native
projection results or battle occlusion on hardware.
Visibility still uses the existing shared near/far guard; upstream's
`isGunShotVisible` is an unimplemented always-visible stub. Original-game
ray clipping/visibility fidelity remains an explicit runtime validation task.

`isSoundFileLoaded` is a synchronous file-service completion query, not audio.
It becomes true after raw copy or bundle publication, false during reads,
failed retries, unknown/null requests, and after selected-disc changes.
Filename keys are case-insensitive. Production-body tests cover these paths
with mocked I/O. This tracks the last load's completion, not ownership of a
destination after later overwrites; callers still own lifetime/error recovery.
The upstream unconditional-true stub is not used.

Direct module dispatch remains disabled. The explicit A3 option now enables
the ordinary field encounter's existing overlay/main-task/engine pipeline.
This is experimental bring-up, not combat or return-path acceptance.

## Experimental native A3 execution

`LAGI_BATTLE_EXECUTE_A3=ON` requires `LAGI_BATTLE_NATIVE_BUILD=ON` and defaults
OFF. It reconnects the native A3 entry in generated overlay Draw, after a
resource preflight and before entering the original music/wait/engine pipeline.
It does not select a different encounter, alter stats, grant rewards, force a
battle from a timer, or enable module-manager debug dispatch.

Preflight checks the existing manager, COMMON indices and exact authored PRG
identity. Unsupported overlays wait with a bounded `[BattleStartup]` diagnostic;
they are not redirected to A3 or treated as victories. Generic-data reads have
a missing/truncated-file guard before parsing; read failure releases the failed
owner for retry. This is not a general malformed-disc parser. The native task
base already clears allocations. Main-task allocation retry keeps the overlay
in startup and does not push its two fonts twice. A disc change after generic
initialization is explicitly rejected pending cache-lifetime restoration.

Checks: production preflight plus generated native overlay Draw with mocked
engine/parser; retry/font ownership; retained startup/generic/engine symbols;
both full VPK builds (execution ON and default OFF), fullres/dither ON.
The read-only real-disc test runs the actual generated generic constructor and
shared quad reader on US Disc 1 `BTL_A3.PRG`: 385044 bytes, 2751 bounded reads
through `060B08F8`, three 10-node laser tables and nine animation groups.
No game files are extracted or committed. These checks do not execute combat.

Known remaining work includes audible/native-loader validation of battle audio
(the opt-in worker stack below is now implemented), other overlays, generic-cache lifecycle
across discs, live task/allocator/formation correctness, gameplay commands,
results/rewards and repeated battle-to-field return. Hardware validation is
still required; neither a retained link nor the data-parser test establishes it.

Experimental hardware build, separate from the accepted normal build:

```powershell
cd E:\dev\Lagi
git switch feature/battle-mode-bringup
git pull --ff-only
cmake -S . -B build-battle -DLAGI_FULLRES=ON -DLAGI_STOCHASTIC_FILTER=ON -DLAGI_DITHER_LOOKUP=ON -DLAGI_BATTLE_NATIVE_BUILD=ON -DLAGI_BATTLE_LINK_AUDIT=ON -DLAGI_BATTLE_EXECUTE_A3=ON
cmake --build build-battle -j 4
```

Retain the matching `build-battle/lagi.velf` and `lagi` ELF. First capture an
authored A3 encounter entry and its log/video; if it crashes, retain the complete
dump with those matching symbols. Expected markers are `[BattleStartup] generic
data ready` and `enter BTL_A3`; neither marker alone proves combat success.

Runtime initialization also logs `[BattleBuild] native=... executeA3=...
linkAudit=...` from the actual compile options. The experimental command above
should report all three as 1; default builds report 0. An absent startup marker
alone cannot distinguish a disabled build from a run with no encounter.

The read-only disc test now also uses the actual COMMON remapper and native
overlay-entry reader, deriving supported function addresses from A3's source
dispatch cases. On the inspected disc, all 10 remapped A3 subbattles and their
10 formation records reference supported map/formation cases and bounded
position data. This is coverage of authored dispatch data, not execution of
formation callbacks, enemy attacks, model loading or combat.

## Battle audio sequence return (experimental option only)

The original MAIN routines distinguish a pending marked sequence from a pushed
child: `0602B3DA` marks the current entry; `0602B29A` clears the parent's mark
while pushing, retaining its sound IDs and resetting volume sentinels to 0x80.
The upstream reconstruction incorrectly sets the parent's mark to 1. Pop
`0602B506` either clears a still-pending mark without a reload or decrements the
index and restores the previous sequence. Its parameter controls post-load
replay. `0602BADA` calls replay `0602B962`, which checks sound-config byte +2
and starts eligible saved sounds at volume 0x7F. Original byte loads sign-extend
the configuration's return-sequence offset. The read-only disc audit checks
these instruction anchors; no game resource bytes are committed.

With A3 execution ON, the existing SPSC queue now carries mark/pop events to
the audio worker. Only that worker owns the eight-entry stack, sound IDs and
bank transitions. Return uses the existing bank loader and stages saved IDs
for the authored post-load replay filter; parameter zero does not replay.
The loading-ready query also counts accepted bank/pop events so an older
completion cannot acknowledge a newer transition still queued for processing.
Default builds retain the accepted audio behavior (the new stack is inactive).

`tests/battle_audio_stack_test.ps1` executes production worker load/pop/replay,
queue/event processing and readiness bodies against mocked SCSP services. It
checks nesting, bound/underflow behavior, cancel-before-load, signed return
offset, parent mark clearing, volume sentinels, config-gated/no replay, and
accepted versus rejected queued control events. Both Vita VPK builds pass.
No listening, 68K/SCSP execution, concurrent worker scheduling or battle return
is established by these host tests.

The experimental loader now selects native states 0 (different area/no prior
sequence), 2 (same area/different sequence), and 8 (same sequence). It preserves
sound IDs instead of unconditionally clearing them. The push comparison uses
the authored signed return offset; pop compares with the outgoing child.

Original `0602B790` tests file identity and exact destination via `0600FDCA`,
ORs missing-file records' +8 masks, then stops only playing old sounds in those
banks. `0602B8EC` waits on the same bank mask using the driver's high-byte status
words. These are now service operations over worker-owned sound-RAM residency,
old-sequence sound configurations, and 68K words at `0x780 + 2*slot`. Matching
files skip redundant copies; any overlapping successful file write invalidates
the old whole-file identity. Disc changes/new worker memory epochs clear the
ledger. Transfer failure retains state 4 and cannot publish loading-ready.
Post-load replay requires both native config +2 and the stopped-bank mask.
Default audio retains its previous loader policy.

The expanded stack test executes the actual selective stop/wait and loader
state bodies, including a still-playing unaffected bank, failed transfer,
masked replay, and all 65536 driver-status word patterns. The production bank
loader test (`tests/battle_audio_banks_test.ps1`) checks exact residency skips,
partial-overwrite invalidation, missing/oversize/out-of-range writes and disc
change reload with mocked file bytes. Both full Vita builds and original-disc
instruction anchors pass. This still does not prove real driver handshakes,
audible continuity, or worker timing on hardware.

Still open: queue overflow remains an explicit logged delivery failure, not a
successful transition. Modes above 1, pending
sound requests across transitions, audio timing and repeated encounters need
native/hardware validation before this can be called complete.

## Battle view and scene publication

Generated `battleGrid.cpp` now captures the native view immediately after
`generateCameraMatrix`. That function writes `pCurrentMatrix`; the existing
bridge removes the captured root from model matrices and Neptune reapplies it
once. Ordinary encounters remain children of the paused field and keep module
mode 3. They must publish the battle grid's view, not reuse the paused field
camera. No camera pose, orbit, target or interpolation is recreated in Lagi.

Direct module battle mode 4 now shares the native frame/UI/fade publication,
material indexing, GPU texture pool, resident geometry capacity and authored
clip-plane paths. Its presentation adapter has no field/dragon-task pointer
dependency. Modes 3 and 4 share projected-world-sprite and mesh depth behavior;
the existing field radar depth/filter exception is unchanged. Field-only probes
and timing diagnostics remain field-only. Current dithering flags are unchanged.

`tests/battle_presentation_test.ps1` checks the generated camera capture anchor,
shared capability/depth gates, and production battle view publication against
a mocked renderer (exact matrix, fixed-point clip conversion, neutral fallback
camera). Both full Vita VPK configurations and the retained-root A3 link audit
pass with fullres/dither ON. This is not task-order, camera motion, lighting,
occlusion or on-device rendering acceptance. Overlay startup and generic data
are now connected only by the separate experimental A3 option described above.

## Expanded shared/A3 graph and initial retained-root audit

The opt-in source graph now contains all 49 shared battle `.cpp` modules and
all 14 A3 modules, including formations, enemy bodies, HUD/radar, commands,
items, Berserks, damage and result screens. It also compiles the real scalar/
vector fixed-point interpolators and `kernel/monsterPart.cpp`. The latter
three resolved dependencies exposed by a stronger retained-root audit. Native
interpolator headers retain their original local include directory per source.

`LAGI_BATTLE_LINK_AUDIT=ON` requires the native-build flag and retains the exact
compiled `_Z19overlayStart_BTL_A3P10s_workArea` entry through the linker. It does
not execute that entry or enable overlay dispatch. Unlike a trimmed compile
build, this follows its real function/vtable dependency graph.

The initial compile-only VPK succeeded with the audit OFF. Its first retained
root link failed, exposing these boundaries (now restored above):

- `displayRaySegment`: native fixed-point world-to-view/projection and VDP1 ray
  packet path, not the upstream BGFX immediate-render branch.
- `sendRaySegmentToVdp1`: projected ray-quad packet emission, Gouraud and
  native ordering/depth metadata for Neptune.
- `isSoundFileLoaded`: despite its name, the reachable Berserk scroll caller
  passes filename pointers for SCB/PNB graphics loaded through `loadFile`.
  Upstream's implementation simply returns true. Do not invent an audio-bank
  query or accept an always-ready stub as a faithful restoration; establish
  the actual resource-completion contract.

Keep the audit enabled when evaluating link readiness; disabling it can hide
missing services through link trimming:

```powershell
cmake -S . -B build-battle -DLAGI_FULLRES=ON -DLAGI_STOCHASTIC_FILTER=ON -DLAGI_DITHER_LOOKUP=ON -DLAGI_BATTLE_NATIVE_BUILD=ON -DLAGI_BATTLE_LINK_AUDIT=ON
cmake --build build-battle
```

The native battle main-task production-body test checks zero-state factory
entry, its single music callback, repeated sound-loading waits, parent callback
ownership, 3D/camera/grid/engine ordering, task completion and allocation failure.
Services are mocked: this is not native audio, battle or GPU execution.

Generated native sources now retain timestamps when their contents do not
change, so toggling the link audit does not unnecessarily recompile the entire
accepted runtime. Output bytes and upstream-source ownership remain unchanged.

## Initial nine-module compile foundation (historical checkpoint)

`LAGI_BATTLE_NATIVE_BUILD` defaults OFF. With ON, the ordinary generated-source
adapter compiles these real native dependencies into the Vita target:

- battleEngine, battleEngineSub0, battleEngineSub1
- battleMainTask, battleGenericData, battleDragon, battleGrid
- BTL_A3 entry and BTL_A3_data

All nine compile under GNU VitaSDK 15.2.0 without modifying upstream sources.
The initial link caught a duplicate `initVdp1Quad` definition in battle generic
data and the existing shared service. The generated battle copy now declares
and reuses that service. A production-body differential test compares both
readers over 128 synthetic sequences, every field, signed fixed-point values
and terminal records. The upstream copy is untouched.
This flag does **not** enable encounters, undo dispatch safeguards, initialize
battle data, or establish usable combat. Link trimming may discard unreachable
functions; a successful VPK is not evidence that a reachable engine links or
runs. Remaining dependencies include HUD/radar, intro/formations/enemies,
commands/items/Berserks, effects, damage, results and battle teardown.
The compiled engine's actual undefined-symbol list confirms dependencies on
`battleEngine_CreateHud1`, `battleEngine_CreateRadar`, `createBattleIntroTask`,
`createBattleCommandMenu`, `createBattleTextDisplay`, `createBattleEndTask`,
`createBattleResultScreen` and the native Berserk/effect factories.

`tests/battle_build_boundary_test.ps1` checks the two real configured build
graphs and compiled objects: opt-in dependencies are present only in the
battle configuration, and both generated dispatch/overlay safeguards remain.
It is not a runtime battle test.
The final opt-in full VPK build passes with full resolution and both existing
dithering flags ON. No deployment, battle entry, combat or hardware success
is claimed. The ordinary build's OFF configuration and source graph were
checked; it was not freshly rebuilt for this compile-only checkpoint.

Use an isolated build directory so the accepted normal configuration is not
silently changed:

```powershell
cd E:\dev\Lagi
git switch feature/battle-mode-bringup
git pull --ff-only
cmake -S . -B build-battle -DLAGI_FULLRES=ON -DLAGI_STOCHASTIC_FILTER=ON -DLAGI_DITHER_LOOKUP=ON -DLAGI_BATTLE_NATIVE_BUILD=ON
cmake --build build-battle
```

## Required progression and acceptance

Extended hardware capture (2026-10-17, `12-16-55.mp4`): floating numbers and
gauge fills are visible and multiple A3 battles complete/return. Battle-only
log intervals (startup/sequence-return boundaries) include renderer samples
at 31.7..33.8 ms with 992 polygons, ~5.0..5.3 ms scene build,
~12.8..15.2 ms CPU preparation and ~14.6..19.0 ms GXM wait. Logged scanout
intervals reach ~50 ms (three vblanks). These are overlapping timing scopes,
not additive independent costs or proof of one sole bottleneck.

Charge-ray repair: original BTL_A3 `0608523C..060852F0` supplies two contiguous
endpoints to MAIN `0602F610`, widths {0, animated size}, sprite source base+CF8,
size 0320, color base+2EF0, mode 8 and Gouraud table `060ACE54`. Upstream passed
one local vector (the shared service reads a second endpoint beyond it), then
used the wrong texture and non-tapered sizes. The generated draw now preserves
the original pair and runtime table. Disc-backed tests execute that actual
generated draw, verify endpoint ownership/arguments and original literals.
This routine supplies sprite Gouraud colors; it does not itself establish a
new point-light source. Device ray appearance remains pending.

The later complete dump `psp2core-1792255547-0x001d662d59-eboot.bin.psp2dmp`
(187280 bytes), matched to the October 17 12:36 `build-battle-vita/lagi` ELF
and `lagi.velf`, identifies a deliberate town-dispatch assertion, not a GPU
fault. Runtime text base is 8103D000; PC offset 1E5BE2 maps to ELF 811E5BE2
(`_kill_r`), with `raise`, `abort` and `loadTownPrg` on the captured stack.
The return at runtime 811763E1 maps to ELF 811393E1; the preceding call at
811393DC invokes `__assert_func` with source line 346. Generated
`town_town.cpp` line 346 is the unknown-overlay assertion. Stack bytes retain
`TWN_E011.PRG`; module-manager status 08 also selects E011. The preceding
TWN_RUIN load is the existing shared-data initialization, not proof that the
requested destination is Ruins gameplay. The user confirms this exit leads to
an in-engine cinematic.

The experimental native-battle build now retains E011's authentic dispatch
and compiles `town/e011/twn_e011.cpp` through the generated adapter boundary.
Its native overlay data, NPC registration, script task, background task, main
logic and camera startup are retained; it is not rerouted to E006. The ordinary
build keeps its prior dispatch and does not compile E011. The fullres/dither
Vita VPK links successfully, and `battle_build_boundary_test.ps1` verifies
that isolation and the native startup anchors. This removes the identified
dispatch assertion in source; the complete cinematic, its special background
and exit still require device testing. Upstream's unknown object-definition
assertion and unimplemented subtask factory remain limitations if reached.
The local parser does not symbolize modules
named `lagi` automatically: resolve offsets against the original ELF LOAD
base, not against the relocated VELF symbol addresses.

Color-clear optimization candidate: the capture spends ~3.4 ms clearing the
960x544 color buffer on the CPU. Native scene rendering now requests an ABGR
zero fill through the GXM transfer engine and finishes that transfer before
BeginScene. A rejected submission retains the exact pitch-sized CPU clear;
failed completion rejects the frame instead of racing GPU writes with CPU
fallback. No actors, geometry, resolution, filtering or effects are removed.
`native_color_clear_test.ps1` checks production API arguments, completion order,
padding bounds, rejected-transfer fallback and failed-completion behavior with
a mocked transfer engine. Builds are not hardware API or performance proof;
compare fresh battle `clear`, CPU preparation, render and scanout intervals on
the same multi-enemy route before calling this a measured improvement.

Flight/tunnel audio regression: active sounds were stored by shared data-bank
number (config +0), while native stop/wait/status operates on eight playback
control slots (config +3). Multiple ambient sounds in one bank overwrite each
other in that bookkeeping and transitions can stop/wait on the wrong driver
slot. Sound requests now preserve control-slot ownership, including volumes,
sound-ID lookup and stack snapshots. Data-bank selection for actual playback
and residency masks remains independent. The production-request test covers
two sounds in bank 3 on controls 4/5, volume/stop routing and selective stop/wait
until both driver slots finish. This shared request fix applies in both builds;
the experimental same-area loader remains opt-in. Device tunnel audio retest
is still required.

Missing flight speech is a separate service gap: `fieldPlayPCM` routes script
filenames to `lagiAzelFieldPlayPCM`, which currently logs only, whereas the
desktop original plays big-endian signed 16-bit mono PCM at 22050 Hz. The slot
repair does not implement that raw-file playback/mixing path. Do not treat it
as restored voice or replace it with a sound-bank/channel guess.

Floating-digit checkpoint: restored the stubbed `damageDisplay_draw` through
a native VDP1 emitter port of BTL_A3 `0605BCDC..0605BDA8`, called at
`0605C0DE`. It reads runtime sprite tables `060B1F7C`/`060B1F04` and Gouraud
colors `060B1FF4`; no game font/color data is embedded. Native formatting,
timing, positions and HP/BP effects remain Azel-owned. The disc-backed test
executes the production emitter for all ten digits, both modes and all four
color sets, checking texture relocation, geometry, color slots and packets
against original instruction/table anchors. Context storage is mocked, so
actual floating-number GPU appearance is still a device acceptance item.

Effect scope: shared effect lighting must work for attacks, items, Berserks
and enemies, not only healing. Existing grid light interpolation and per-draw
capture are the integration boundary; do not add item-specific substitute
glows. Current source still stubs healing particle update/draw, portions of
status/attack effects, and the final dragon effect-light step. These missing
producer paths and setup/apply/restore ownership require original-code ports
and device validation; restoring numbers/gauges does not complete them.

First hardware battle acceptance (2026-10-17): user recording/log confirms A3
combat entry, commands and battle completion execute. It also shows missing
gauge fills/damage effects and a black field return despite resumed audio.
This is partial combat evidence, not full battle acceptance.

Return-fade repair: MAIN `0600C888..0600C8AE` starts both native fade channels
from their current colors to field targets +48/+4A over 0x1E frames, after
sound readiness and field unpause. The reconstruction omitted these calls and
left the preceding one-frame black hold active indefinitely, matching the log.
The generated encounter now restores those calls. `battle_return_fade_test.ps1`
executes the generated resume case with mocked services and checks audio wait,
both independent targets, 30-frame duration and one-shot state advance.
On-device corrected transition appearance remains pending.

Gauge-fill repair: native `drawGaugeVdp1` emits RGB555 VDP1 command 4 for
HP/BP/power/enemy life fills. These packets appear in the hardware log, but
Neptune accepted command 4 only when marked as radar-owned. It now draws all
command-4 solid polygons through the existing GPU solid pipeline, retaining
the separate radar-only transform and depth/pass ownership. This does not
implement the stubbed damage-display draw routine or missing healing effects.
Compilation is verified; actual gauge appearance remains a device check.

Task heap checkpoint: native-battle builds now drain task-owned allocations
on the scheduler's disposal pass, after child/parent Delete callbacks. The
production free service receives user data (not allocation headers) and unlinks
each node. Default builds retain the accepted prior policy. Host coverage in
`battle_task_heap_test.ps1` executes actual scheduler/heap bodies over 256 nested
lifecycles, with explicit frees/reallocations, delayed disposal and a paused
survivor. This fixes tracked raw heap leaks, not C++ work-area destruction,
placement-object destructors or every resource lifetime. Device repeated-battle
memory acceptance remains open.

A3 enemy death/sound checkpoint: original BTL_A3 escape and death routines
read death mode at parameter +2C (`060763CE..060763F6`,
`06076C5A..06076C60`), hit sound at +2D (`0607750E..06077528`),
and death sound at +2E (`06076C16..06076C32`). Sound bytes are unsigned
with FF meaning no sound. Upstream incorrectly reused the +28 quadrant-color
array for death and sound, including an out-of-bounds index 4. The native
build generates an expanded formation header shared by all generated consumers,
parses the three distinct fields and routes only death/sound reads to them.
Quadrant color consumers and authored gameplay decisions are retained.

The real-disc test verifies original instruction anchors and executes the actual
generated formation parser for all four formation roots: seven per-type entries
and ten animation records, including both beam-triggered transitions. Parsed
death/sound bytes, independent quadrant colors, record counts, animation offsets
and timers are checked against their source fields. Model/hotspot loading is
mocked in this host data test. Enemy movement, death effects, sound playback and
resource lifetime still require device combat validation. No upstream header
or copyrighted data is edited/committed.

A3 enemy-animation checkpoint: the original table reader loaded exactly one
record and never advanced its source pointer. Actual Disc 1 beam-response
records at per-type parameters `060A8A04+20` and `060A7CF8+24` select state 1;
the latter's subsequent transitions also reach state 2. The four distinct
Urchin-family parameter sets require 1, 2, 1 and 3 records respectively.
The generated A3 reader now follows signed next-state indices from state zero
and both beam overrides, preserving -1 (keep current), and reads each record
at the native 0x24 stride. Original BTL_A3 instructions `06076234..06076244`
compute this stride before loading animation offset +1C. No upstream or game
data is modified or committed.

`battle_generic_disc_test.ps1` checks generated count/stride hooks, executes
the production reachability helper against real disc records with bounded
reads, and tests sentinel, recursive transition, cycle and signed-index limits.
This is data coverage, not enemy AI or beam-hit execution on Vita; full combat
and repeated returns remain unverified.

Allocator return checkpoint: original MAIN `060315B4` pops the allocator and
restores the parent's inline file table at +18. Upstream's C++ vector conversion
only clears the vector at the root and otherwise leaves the child table active.
The generated native-battle build now moves the parent's vector under its own
allocator identity during push and restores it on pop, preserving table storage,
filenames, refcounts and bundle pointers. Native task teardown is child-first;
this change does not replace task/bundle deletion or reallocate surviving assets.
Reset clears sidecar metadata. Default builds retain their prior allocator path.

`tests/battle_allocator_tables_test.ps1` executes generated native push/pop/reset
and the production owner service with mocked heap/resource I/O, covering nested
empty/nonempty tables, exact storage/bundle identity, root pop and reset. Full
Vita builds and original allocator instruction anchors pass. Native DRAM resource
reload, task-heap freeing, allocation exhaustion and repeated on-device returns
remain unverified; this table-ownership repair does not establish those behaviors.

1. Audit/test combat-relevant persistent player state: inventory healing and
   quantities, healing Berserk HP/BP costs, equipment, Dragon stat/morph changes.
   Existing implementations and menu appearance are not device acceptance of
   their gameplay effects or save/reload persistence.
2. Compile and link the real reachable battle dependency graph. Restore generic
   battle data and overlay lifecycle, then the native encounter-to-manager
   transition using original-game evidence. Keep normal flight safe meanwhile.
3. Publish native battle camera/models/VDP1 commands/VDP2/HUD to Neptune, with
   source-faithful priorities, palettes, light/depth and current dithering.
4. Validate intro, targeting/quadrants, movement, gun/laser/commands, items,
   Berserks, enemy actions, HP/BP/gauges, audio and win/lose paths.
5. Restore authentic rewards, persistent stat/item changes, cleanup and return
   to the correct field state; confirm save/reload and repeated encounters.
6. Build and run on Vita/Vita TV. Require matching logs and visible gameplay;
   source/host checks cannot prove rendering, input, audio, timing or combat.

Books, System settings, world travel and Enemy List completion remain separate
unless a demonstrated battle dependency makes one necessary. Menu transition
polish is accepted as good enough and is not a prerequisite for this goal.
