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

Known remaining work includes native battle audio sequence push/pop (current
Vita entry is empty and pop only logs), other overlays, generic-cache lifecycle
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
