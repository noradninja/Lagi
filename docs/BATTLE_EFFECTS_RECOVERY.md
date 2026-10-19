# Native battle effects recovery

## Scope and acceptance

Continue the native producers and shared services, not substitute item effects.
`Azel decides. Lagi services. Neptune renders.` A registered berserk factory or a
successful damage calculation is not evidence that its visual task is complete.
No changes to the pinned Azel checkout or game assets are required.

The four recent encounters meeting approximately 30 FPS are performance evidence
for those encounters, not evidence that unfinished effects are rendered. Preserve
that baseline while restoring the authored workload.

## First verified correction: Chain / Prism trail palette

Original BTL_A3 `0609D9AC` draws trail segments through MAIN `0602E03C`.
At `0609DA3E..56`, the owner's VRAM allocation is added to **both** the texture
offset at config `+4` and palette offset at `+8`. The reconstruction relocated
only the texture. The generated `battle/berserk_laser.cpp` adapter now relocates
the palette too, with native 16-bit storage. Segment order, width interpolation,
Gouraud colors, camera, timing and damage are unchanged.

`tests/battle_laser_palette_test.ps1` executes the generated live draw function,
covering three allocation bases (including 16-bit wrap), active-tail boundaries,
texture/palette addresses, width, vertex colors, and segment order. Its optional
disc check verifies the original instructions in place without extracting assets.
Vita battle VPK compilation and this host/disc test pass. Hardware Chain/Prism
visibility and color remain unverified; this is not a claim that all of their
VFX or lighting is restored.

## Producer audit: confirmed incomplete paths

| Path | Current source evidence | Remaining native recovery |
| --- | --- | --- |
| Blast-family chips | `battleEngine.cpp` `sBlastChipEffectTask`: timed update, no Draw; constructor explicitly omits particle arrays and light animation | Original constructor `0607F7E4`, update `0607FE8C`, associated draw/model and light state machine; use AIRBOMB/GRDBOMB/ABOMB2/GBOMB2 resources already selected by native item logic |
| Chain / Prism | Native trail Draw and point source call exist; palette relocation corrected above | Verify original trail config, point-light publication, impact emitters and cleanup end-to-end; hardware acceptance pending |
| Laser Storm | `berserk_laserStorm.cpp` leaves model initialization and `sHomingLaserTask_Draw` unimplemented; particle-ring TODO | Restore shared homing-laser model/trail tasks and original ring producer |
| Lightning Storm / Plasma Swarm | `berserk_lightningStorm.cpp` `sLightningBoltTask_Draw` has no body; trail update consumes RNG without constructing the full trail | Restore native trail data/update and `0609DD4A` strip draw, not a screen-space approximation |
| Dragon Phoenix | `berserk_dragonPhoenix.cpp` wing-fire subtask unimplemented | Restore original child effect and resource ownership |
| Astral Phantoms | `berserk_astralPhantoms.cpp` model/orbit creation and target iteration TODOs | Restore producer/model lifecycle before visual acceptance |
| Phantom Slashers family | `berserk_phantomSlashers.cpp` documents missing projectile rendering | Recover native projectile producer and its complete timing/data flow |

This is a **gap inventory**, not a completed audit of every usable or enemy.
Enemy animation callback coverage in `battle_generic_disc_test` covers the active
A3 formation readers, not every child effect or every battle overlay. Continue
from each attack's native callback through its child tasks, resource descriptors,
light setup, draw and restore. Do not attach arbitrary lights to every attack.

## Shared lighting checks still required

`battleGrid_updateLightColorInterpolations` updates native color/falloff state;
`battleGrid_draw` passes it through `setupLight` and `generateLightFalloffMap`.
Chain additionally calls `dragonFieldTaskDrawSub1Sub1` at the transformed beam
source. Existence of these calls alone does not establish that every effect uses
the correct signed channels, coordinate space, lifetime or cleanup. Inspect each
native producer and the renderer's capture at the actual draw boundary. Preserve
combined lighting and lighting-only/full-mode agreement from the existing bridge.

## Pre-Arachnoth E011 cinematic crash

The previously supplied partial core and log narrowed this to an unregistered
one-argument script call after E011.EPK startup (see `BATTLE_MODE_BRINGUP.md`).
The original E011 script calls scheduler `0605890C` with descriptor `060600C4`.
Its callback table `0605FD10` requires quad `060589E2`, model `06058B2A` and ribbon
`06058FF8`. The staged quad/model tasks exist, but the scheduler remains unwired
because the native ribbon producer is not restored. Do not skip this callback.

Later script functions `0605809E`, `060557E0` and `06055872` must also be restored.
Fresh disc inspection confirms `060557E0` and `06055872` configure distinct
state modes (1 and 2) on the original shared event task, including descriptor
fields, VDP2 state and callbacks; they are not interchangeable no-op delays.
Recover those owners/updates as well as the ribbon before claiming cinematic
playback fixed. The old partial core cannot prove a new fault location, and a
currently overwritten ELF must not be represented as its matching executable.

No cinematic crash fix is claimed by the palette correction.
