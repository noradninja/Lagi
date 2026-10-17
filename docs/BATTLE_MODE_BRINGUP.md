# Native battle-mode bring-up

Branch: `feature/battle-mode-bringup`, based on accepted menu milestone
`c2cef69`. Goal: working native battle entry, combat and exit—not a substitute
battle scene or compile-only completion. Azel decides. Lagi services. Neptune
renders. Do not edit `extern/Azel` or regress accepted frontend/flight/menu paths.

## Entry audit

The current Vita generation deliberately disconnects battle in three places:

1. `kernel/moduleManager.cpp` overlay dispatch slot 4 is replaced with null.
2. `battle/battleOverlay.cpp` startup calls for BTL overlays and debug teardown
   are removed by the source adapter.
3. The field encounter path creates upstream `field/battleStart.cpp`'s loading
   task. Its state 3 waits for desktop debug keyboard toggles, rather than
   implementing the actual gameplay battle transition.

Do not merely populate slot 4 and call this complete. Native `loadBattle`
is in `mainMenuDebugTasks.cpp`; manager/debug-list/overlay tasks and the actual
field loading transition require a complete ownership/call-order audit.
`battleManager.cpp` itself contains debug behavior and unfinished Delete.
The real `loadBattle()` reads signed battle/subbattle IDs from COMMON
`0x212F96 + 2*r5`, resets temporary allocators and calls `loadBattleOverlay()`.
The latter requires an already-live `gBattleManager`, remaps the authored IDs,
rejects an empty PRG name, then creates the overlay task. Restoring slot 4
alone does not establish manager ownership or make this null dereference safe.

`BTL_A3.cpp::overlayStart_BTL_A3()` loads its real overlay data, selects
`gCurrentBattleOverlay`, and constructs `createBattleMainTask()`. That task
waits for native sound loading before resetting 3D/camera/grid and invoking
the A3 initialization callback. A3 then constructs the battle engine using
authored overlay data. Preserve that pipeline and real encounter/formation
selection rather than hardcoding a different encounter.

Battle generic initialization is also currently stripped from ordinary boot.
Its lifetime/file dependencies must be integrated before a reachable engine
uses `g_BTL_GenericData`; its native constructor reads `BTL_A3.PRG`.
Task allocation already zeroes work areas through
the native task base; audit that contract before inventing extra state resets.

## Compile foundation

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
