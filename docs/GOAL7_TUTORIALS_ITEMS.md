# Goal 7: tutorials, then items and Berserk coverage

Active. User requests tutorials first, including their relationship to the
title-screen tutorials. Goals1-6 are accepted; preserve their behavior and
performance. No tutorial execution or device acceptance claimed yet.

Additional user-reported combat issue: weak-point icons and text currently
show from every angle. Audit original target/quadrant visibility and restore
the shared native condition as a separate atomic correction after tutorial
bring-up; do not add enemy-specific facing rules.

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

### Tutorial source compilation checkpoint

BTL_T0.cpp is now included in the battle Vita source list, using the same
generated adapter boundary as A3. Its local header is qualified for the
flattened generated directory. The native panel replacement consequently
compiles against actual Vita task, allocator and VDP1 command types now.
Full Vita VPK build passes. Dispatch/preflight remain intentionally disabled:
this is not a playable tutorial or proof its retained symbols all link when
the entry becomes reachable (dead-section elimination still applies).

Restored font member access: original0606C5DE..0606C60E selects signed font
byte+18 for argument0, otherwise+17. The reconstruction read raw bytes from
the host task pointer; its task header/pointer layout is not Saturn's. The
generated adapter selects pText->m18/m17 instead. A generated-function test
passes all65536 signed font pairs, nonzero argument variants and a deliberately
padded host task object. Panel's512 cases also pass again. No extern edits.

### Native panel component checkpoint

Disc1 directory: BTL_T0.PRG LBA262947, size392928, base06054000;
BTL_T0.FNT LBA263146, size7154. Native0605AD40..0605AE72 confirms
the panel frame counter wraps as a word, updates the controller's bit4
acknowledgement from bit2/substate, and emits only while bit1 is set.
With bit4 set it masks frame to7FFF and selects sprite1 for modulo8<4,
otherwise sprite0. COMMON/global UI guesses are not used: the two sprite
record pointers are overlay060AADE8, and texture/palette offsets are added
to native DRAM file entry14's VDP1 base. Position is parent(C-192,E-144).
Native PMOD and size come directly from the selected descriptor.

`lagi_tutorial_panel.inc` restores that routine through a guarded T0
generated-source replacement. The overlay is **not yet in the runtime
source list or dispatch**, so this component does not change device output
yet. Its production-include host fixture passes512 flag/substate/frame
cases plus wrap, packet-capacity and missing-allocation guards. Existing
Vita package build is checked separately; that does not compile/validate
the disconnected T0 overlay as a whole. Continue with its actual integration
and remaining stubs before requesting a playable tutorial test.

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
