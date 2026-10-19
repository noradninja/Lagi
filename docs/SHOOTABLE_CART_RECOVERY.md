# Shootable world cart recovery (2026-10-19)

## Crash evidence

Supplied `psp2core-1792417649-0x00271d3793-eboot.bin.psp2dmp` is complete:
5,893,368 expanded bytes, all 65 segments contained. Lagi code base is
`8101B000`. Main PC `811FFE22` resolves to `_kill_r`; r5 is 194, r6 points to
`"0"`, r7 points to the generated `field_field_a3_a3_dynamic_mine_cart.cpp`.
The supplied ELF's relocated strings identify `s_A3_0_Obj3::Update2`; its
generated line 194 asserts in the default switch case. The callback sets state
1 on a successful shot, while that update only handled state 0. This is not
the separately pending E011 cinematic crash or a multi-enemy battle failure.

SHA256 of supplied evidence (not replaced by local verification builds):

| Input | SHA256 |
| --- | --- |
| lagi.log | `9580F26BBACFABCFE855DCD2DE5F2538ED17CBC8190D7B665DC9A8F68A482C87` |
| core | `D6F57A2F22DDDEC55117A5BCF7C0FFAD7C471CB8EE440848DBAE3D03D55EB3E7` |
| lagi.velf | `82C20DAD2852CDD9575952720EE4B1348C24743509FFB33EC540D7D6A585B796` |

## Source-faithful recovery

Original FLD_A3 `0605E4E6..0605E5EA` implements three states:

- 0: idle, still update targeting, dragon proximity and visibility.
- 1: move X/Z using authored velocity, publish nearest cart position; when
  both coordinates are strictly within `10000` of the authored endpoint,
  create native smoke task `0605E928` once and advance to state 2.
- 2: continue moving until the original signed crossing conditions hold;
  retire through the normal task owner. Preserve the native X-coordinate
  read in the Z-endpoint comparison at `0605E594`; changing it is unrelated.

The generated adapter includes `lagi_a3_shootable_cart_update.inc`, restoring
all states rather than switching to looping waypoint logic or suppressing the
failure. Original callback `0605E5EC` remains in use: set persistent bit `49C`,
disable the target and enter state 1. Constructor variant 3 checks that bit on
recreation; variants 1 and 2 keep their original looping updates. No item-specific
coordinates, new flags, global assertion suppression or upstream edits.

Following the child task exposed a second native-layout assumption in the shared
cart smoke producer: `(sVec3_FP*)pThis` points at a C++ task header, not the native
`m0_position` payload. The generated `o_fld_a3.cpp` adapter now uses
`&pThis->m0_position`. This corrects the existing looping carts' even-frame smoke
as well as the newly reachable shot-cart emitter; odd frames retain the live
parent position and velocity references. Particle mode, resource, drag, RNG and
32-frame emitter duration are unchanged.

## Validation and limits

`tests/shootable_cart_test.ps1` compiles the production update include, extracts
the live generated shot callback and smoke task, and exercises idle, shot,
positive/negative motion, strict proximity, smoke exactly once, endpoint
retirement, visibility and native unknown-state fallthrough. It verifies the
callback's persistent bit/target disable and guards constructor variant wiring.
The smoke fixture deliberately uses a nonempty C++ task header; it checks both
position sources, velocity, all particle parameters, 96 RNG calls over 32
emissions, expiration and missing-pool behavior. Disc-backed instruction checks
cover native motion, threshold, signed crossing, callback and smoke helper EA.
This is host/source coverage, not a save-file round trip or Vita gameplay test.

Both battle and normal full-resolution Vita VPK compilation pass. Hardware
acceptance still requires shooting this object, seeing its native movement/smoke
and retirement, then revisiting/loading to confirm it remains consumed. No claim
that every unfinished world-object or battle callback is now implemented.

## Long-run combat evidence

The supplied log contains five completed startup-to-audio-restore spans:

| Native formation sub | Full-window intervals | Over two vblanks | Worst window FPS |
| --- | ---: | ---: | ---: |
| 8 | 1680 | 10 | 29.478 |
| 9 | 2880 | 152 | 23.738 |
| 4 | 2760 | 1 | 29.845 |
| 4 | 1080 | 1 | 29.845 |
| 4 | 1080 | 1 | 29.845 |

165/9480 measured intervals exceed the two-vblank cadence: approximately 98.3%
meet it. These windows exclude startup/return boundary windows but can include
intro/results; heartbeat timings are samples, not whole-frame averages. Four
spans remain near 30 FPS; sub 9 still has a meaningful slow window. Its median
992 polygons / 5952 vertices matches sub 8, while median sprite draws rise from
39 to 45 and clipped sprite bounding-box area from 45,182 to 120,400.5 pixels.
That is a workload correlation, not proof of overdraw as the sole cause; there
is no new isolated GPU timer or exact encounter match. Median sprite cache
misses and slot skips are zero throughout. The cart fix does not alter rendering
or battle timing and makes no performance improvement claim.
