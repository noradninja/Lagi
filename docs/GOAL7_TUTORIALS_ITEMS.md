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

### T0 startup connected for initial hardware testing

Native T0 entry is now force-retained in the existing link-audit configuration;
the full ELF/VPK links its transitive code and the retained-symbol check
requires overlayStart_BTL_T0. This supersedes the earlier compile-only gate.
Checked battle preflight admits the exact BTL_T0.PRG name, validates the
COMMON subbattle bound and a minimum392928-byte native program before font
allocation, and dispatches explicitly to overlayStart_BTL_T0. Unsupported
names are rejected by the entry service too, never silently mapped to A3.
The generated overlay's T0 call passes through the existing white loading
hold, generic-data setup and font retry guards used by A3.

The original T0 first formation at060A76F4 has one type config060A76B8,
state060A7694, with null initialization effect pointer. This narrow audit does
not prove all future effect invocations are serviced. Remaining virtual
default callbacks still need execution coverage/native mapping as encountered.

moduleManager's existing status71/72 return branches return-1 and finish its
normal parent rather than advancing ordinary game story; in-game battle end
uses the original manager teardown and field-script return. Source trace is
not completed device lifecycle validation. Title Tutorial2 also needs its
FLD_A7 route verified; do not assume T0 integration implements A7.

Production startup tests pass tutorial indices0..5, out-of-range/negative,
truncated program and allocation failures, explicit factory selection without
A3 fallback, plus accepted cover/font ownership cases. Retained-link audit
and full Vita build pass. First hardware gate: title Tutorial1 and/or the
native in-game tutorial prompt, including text/panel/input and exit, with a
fresh log. All lessons, performance and title Tutorial2 remain unverified.

### Tutorial environment drawing callback

T0 06054260..060544A0 now emits its original tiled/rolled distorted sprite
quads through native VDP1 packets. It reads grid+6C (camera-rotation target Z),
uses fixed-point sine/cosine and per-multiply truncation, rotates about the
computed horizon and retains the native inclusive right-edge tile condition.
Texture, size, palette and PMOD come from the task's authored fields; shared
native helper0602D9BC confirms CTRL1002, PMOD480|argument and the packed
depth bucket. No extra GPU pass, replacement plane or invented blending.

Production-include host tests verify all emitted coordinates and packet
fields at four cardinal roll angles and three signed horizontal offsets,
tile continuity/count, draw-enable equality and capacity/stride guards.
Vita compiles the actual generated task. This is not rendered device proof;
the lesson launch path is still disconnected pending startup/return audit.

### Tutorial environment projection update

T0 06054156..06054216 restored through lagi_tutorial_water_update.inc.
The generated task exposes original signed offsets+8/+C instead of padding.
Camera height is arithmetic-shifted twice, including negative heights; native
atan2, FP division, ordered multiplies and setDividend determine the vertical
projection. Pitch wraps as a signed word and trig indices mask toFFF. Yaw
uses native staged multiply, division by800, width projection and remainder.
MAIN06035488 uses DIV0S/sign correction; reconstructed performModulo takes
unsigned arguments, so the adapter preserves signed remainder directly
without changing unrelated callers. No floating-point projection replacement.

The production helper fixture passes320 signed height/pitch/yaw/angle cases
with mocked trig services, native fixed-point stage-order checks and word
wrap. Full Vita compilation/package passes. The task's Draw callback remains
unimplemented; this checkpoint alone does not restore environment output or
enable tutorial dispatch. Continue with its rotated strip quads and native
startup/return checks before hardware lesson testing.

### Contextual prompt condition restoration

Original T0 0605A214..0605A53A specifies all nine text sets. Sets0..3 retire
on the matching unsigned quadrant byte and clear both flags10/08. Set4
retires at mode0/submode>=3; sets5,6,7 at modes3,1,4 respectively with a
nonnegative signed submode. Set8 accepts any of those four mode/submode
conditions. Sets4..8 clear08 only, preserving the native menu latch.
The shared menu preamble clears10 if command menu is absent or its bit8
is unset, otherwise raises10 and clears the area on the rising edge.
Unknown text-set values do not invent a completion condition.

`lagi_tutorial_text_condition.inc` replaces the entire stubbed function in
the generated overlay. Its production-include fixture passes430848 cases
across text sets0..10, flags0..31, three menu states, modes0..16, six signed
submode boundaries and all four quadrants, verifying exact clears and flags.
Vita compiles the actual generated overlay. This restores contextual prompt
retirement, not full lesson completion; dispatch remains disconnected until
required remaining effects and entry/return lifecycle are ready.

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
