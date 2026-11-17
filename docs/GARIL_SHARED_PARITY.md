# Garil shared runtime parity

Active goal, 2026-11-17: reuse the working Valley battle, result, story,
cutscene and field-transition services in Garil and later areas. Restore
area-specific authored actors and callbacks through Lagi adapters. Do not
force story flags, replace authored sequences, or modify extern/Azel.

## User evidence and reproduction

The user confirms targeting and attacks work with 1f0148a. Queen gun death
fails to Scatter/end combat; dead enemies lack death motion and retain life
bars; rewards are absent. A subsequent encounter crashes. After the pillars,
the worm story trigger runs without the red-worm cutscene/opening sequence.
These are separate acceptance gates; the earlier cliff/clipping fixes remain
accepted.

Latest Camp save: PANDRA_3_02, 1,252 bytes, SHA256
C87DC5621D297263B15BF81BF73748FE5EB3F658A3246C715696656F060BF395.
Leave Camp and select Garil. Preserve the original file. Saturn save conversion
must map the port's native C++ fields explicitly; a raw memcpy of its stats is
not a valid native Saturn save. Earlier diagnostic emulator checkpoints used
an incomplete conversion, so they are not progression-acceptance evidence.

## First checkpoint: death notification and motion

Original BTL_A5 0605E706..E778 cleans up targetables, increments the native kill
stat, notifies the existing life meter with bits 1 and 8, sets gravity -0x160,
and selects death angular speeds from two RNG calls. The port omitted those
calls and left gravity/angular speeds zero. The adapter restores this tail.

Valley Baldor and Garil Nanyd now call the same typed defeat notification.
Azel's existing battleEnemyLifeMeter remains the sole owner of COMMON-derived
XP/Dyne accumulation and shrinking/removal. Result screens, growth, inventory,
saving and field return retain their existing shared ownership. No independent
Garil XP table or result pipeline is introduced.

Validation: production meter-update host fixture checks a nonlethal hit,
one reward lookup/award, position detachment, meter retirement and a negative
record ID. Fixed-point arithmetic and engine services are mocked; this is
lifecycle coverage, not numerical/rendering parity. Vita VPK compilation passes.
Hardware death/reward acceptance remains pending. Scatter, enemy attacks,
subbattle 4 constructor/lifecycle, worm cutscene and end-to-end return/re-entry
remain open. This checkpoint does not claim those are fixed.

## Native trace findings still to implement

- Queen loss at formation 0605CDFA..CE4E waits for the player turn, checks
  m1D4 and mF0, enters battle mode 7, clears formation attack state and sets
  formation state 9. States 9/10 issue Scatter and signal surviving actors.
  The upstream simplified formation omits this path.
- Subbattle 4 calls BTL_A5_createEnemySingle. The port reads
  mE0_entityGroup.m8_dataTable despite never initializing it. Original
  0605BC3C..BC46 instead reads the enemy type at entity-group +0xC directly.
  Original position table is entity-group +8, selected from COMMON by
  0605AF6C. Model construction/target/damage/formation accesses also require
  typed conversion before treating this encounter as implemented.
- Shared formationTriggerAttack/Followup are stubs because native enemy
  work-area offsets cannot describe expanded C++ tasks. Implement a typed
  actor interface rather than adding byte-offset casts for each overlay.
