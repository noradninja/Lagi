# Goal 7: tutorials, then items and Berserk coverage

Active. User requests tutorials first, including their relationship to the
title-screen tutorials. Goals1-6 are accepted; preserve their behavior and
performance. No tutorial execution or device acceptance claimed yet.

## Current source entry audit

moduleManager_Init(menuID) initializes normal game/dragon/rider/field/battle
state. menuID2 requests status71, menuID3 requests72; the latter selects
Valiant Wing while the former uses Basic Wing. The native status rows map
71 to field mode3/field15hex and72 to mode3/field16hex. These are field
entries, not direct calls into a tutorial battle overlay.

The field A3 entry creates its tutorial script task; that task starts field
script21. Its native story/field checks must remain intact. battleManager's
remapBattleOverlayId maps A3 subbattles B..10hex to overlayF/sub0..5, while
subA maps to overlay10/sub0. Follow authored COMMON overlay names rather
than guessing that all tutorial modes use T0.

The current generated battleOverlay preflight only admits BTL_A3.PRG and
BTL_A3_2.PRG. Remaining overlayStart_BTL_* calls are deliberately stripped
by CMake. Thus source tutorial routines existing upstream do not mean the
Vita runtime can execute them.

BTL_T0.cpp contains a substantial six-phase controller and native entry,
music/bank selection, resource list, environment and formations. Remaining
stubs include the tutorial display panel (0605AD40), nonzero text-condition
branch, water Update/Draw and some overlay invokes/effect callbacks. Its
secondary Baldor formation uses T0-specific data, not A3's overlay addresses.
Do not redirect unsupported tutorials into normal A3 fights or enable the
dispatch before its required resources/callbacks are serviced.

## Implementation and acceptance sequence

1. Trace title/field script selection through authored overlay and subbattle
   records; audit generated/shared battle services and native return behavior.
2. Integrate the actual tutorial overlay data/controller, panel/text/input,
   formations and effects using guarded generated adapters. Keep native
   progression, confirmation, timers, equipment and cleanup ownership.
3. Verify title and field entry share the correct native services, complete
   all available lessons and return correctly without retaining tutorial
   inventory/state in ordinary gameplay. Host tests and Vita package build
   are necessary but not device acceptance.
4. Hardware-test the lessons and accepted normal combat/rendering baseline.
5. Continue with the deferred Berserk menu/reload verification, first-section
   item coverage, Blast Chip and Energy Prism trails/light/camera/timing and
   cleanup. Keep independent corrections as atomic commits.
