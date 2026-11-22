# Garil variant attack trace - 22 November 2026

Current source checkpoint: ea2d32b. This is original-data evidence, not a
runtime fix or device acceptance. Lathum testing remains independent.

## Confirmed missing launch

In `BTL_A5_variant.cpp`, enemy Update state 4 advances animation and, at frame
0x2E (46), only plays sound 0x6D and enters state 5. Original BTL_A5.PRG
Update 06055086 instead performs the following at 06055286-060552D8:

- Reads targetable 5 through model+0x54 (0x1F4 = 5 * 0x64),
  taking its live world-position pointer at targetable+8; copies XYZ to actor offsets 0x1D8/0x1DC/0x1E0.
- Calls factory 06054384 with actor, copied source vector and dragon+0xF0.
- Plays sound 0x6D and enters state 5.

No implementation of factory 06054384 exists in the current adapter or
upstream overlay. Restoring sound alone cannot restore this attack.

## Factory and child timing

Factory 06054384 allocates a 0x30-byte task using callback table 060AD104.
For non-null source and destination, it computes destination minus source,
then calls 0603570E and 060359AE. Each velocity component uses 06035664 with
literal 0x1000 followed by 06035644 with the corresponding delta component.
The precise math helper identities must be verified before implementation.

It initializes a quad from descriptor 060AD2D8 and resource slot 0xE, stores
signed counters 27 and 128 at offsets 0x10/0x12, allocates 0x45C bytes, and
initializes 31 records of stride 0x24 using 060545EC. Initial task flags at
0x28/0x2C are 1/0. A single RNG call sets byte 0x24 to (rng & 0x0E) + 13;
this yields odd values 13 through 27, not an unrestricted random range.

Update 060544DC decrements counter 0x10 and compares its old signed value
against zero to change flags to 0/1. Damage is checked against the NEW counter
matching byte 0x24. At that moment 06054532-06054550 calls shared dragon
service 060642F8 with targetable dragon+0x8C, damage 44, kind 2, impact
position dragon+8, zero knockback vector and stack flag 0x0800. There is no
sphere-contact test in this launch/timing path.

After the active flag switches, counter 0x12 uses another old-value negative
check. Expiry writes 0x1000 to dragon+0xD8 and finishes the child. Every update
also visits all 31 visual records through 06054680. The draw callback
060545A4 visits all 31 through 060546AA, then advances the shared quad via
0602CF38. There is no native delete callback in this task definition. Do not replace this task with a homing
projectile or contact-triggered damage merely because those services exist.

## Reproduction and remaining work

Disassemble the extracted original BTL_A5.PRG with GARIL_DISASM_BASE=06054000
using garil_saturn.exe --disasm. Inspected ranges: 06055086-06055488,
06054384-06054540 and 06054532-060545EC. Literal values above come from the
big-endian PRG words, not guessed structure offsets in the host port.

Next: trace 060545EC/06054680/060546AA and callback table 060AD104; identify
math helpers from common code; restore task through a generated-source
adapter and shared dragon-damage/quad services. Audit the formation selection
and release gates separately: current formation state 3 also lacks its native
turn selection. Prove both gates before presenting this enemy as functional.

## Callback and particle trace correction

The callback table at 060AD104 is exactly {0, 060544DC, 060545A4, 0}.
060546AA is a per-particle DRAW helper, not deletion. It copies the live XYZ
at record+4/+8/+C and calls quad draw 0602CF7C with owner-task+8. The wrapper
060545A4 draws 31 records then advances the owner's single shared quad once.
Do not create 31 independently advancing animations or release the quad from
this draw callback.

Each record initialized by 060545EC stores owner at +0, source XYZ at +4,
velocity XYZ at +0x10, random word at +0x1C and active=1 at +0x20. Velocity
starts at MTH_Mul(0x28000, launchVelocity) per axis (literal 060546E8).
Four RNG calls per record, in order:

1. X velocity += (rng & 0x1FFF) - 0xFFF.
2. Y velocity += (rng & 0x1FFF) - 0xFFF.
3. Z velocity += (rng & 0x07FF) - 0x3FF, then add engine auto-scroll Z.
4. Store rng & 0x7F as the word at +0x1C.

The fourth value is not used by the traced update/draw helpers; retaining its
RNG consumption is necessary. Across 31 records, initialization consumes 124
RNG calls, followed by the factory's one timing RNG call (125 total).

06054680 skips inactive records. Otherwise velocity Y += -0xB0, then position
XYZ += the resulting velocity. There is no per-record expiry, fresh RNG,
contact test or quad-step call. The native 32-bit additions must wrap.

## Formation gate omissions

Original formation Update 0605493C has states 0 through 8. The port has only
0 through 4 plus a relabeled death state 0xB. Original death state is 8.
The missing states are consequential, rather than harmless unused cases:

- State 3 checks player turn, formation byte +0xF2 and engine turn-ready
  m3CC->m8 == 1 before calling selector 06054C2E.
- State 4 uses the signed old counter >15, sets attack-finished, enters 5.
- State 5 waits for engine flag 0x400000 to clear, returns to 3 and clears
  m3CC->m8 and m3CC->m0.
- State 6 waits for engine 0x2000, displays attack name 15 for 30 frames,
  invokes group helper 060A2AB0 and enters 7.
- State 7 uses signed old counter >105, sets attack-finished, enters 5.
- State 8 uses signed old counter >75 then releases resource 0xE and finishes.

Original intro state 2 enters 5 after its signed old counter >84; it does not
enter the port's state 4. Selector 06054C2E marks each live actor selected by
formation byte +0xF3 with flag 0x04000000. If any selected actor exists it calls
060A28A4; success sets battle mode 7, plays quadrant camera and enters state 6.
Failure returns to state 3 and releases turn ownership. If no selected actor
exists, byte +0xF8 postincrement alternates live actor movement flags 1/2,
sets mode 7 and enters state 4. Full helper identities and actor/formation
position updates must be connected before this encounter is advertised as
restored. Avoid merely adding the frame-46 launch to an unreachable attack.

## Implemented timed burst checkpoint

The generated variant adapter now launches `lagi_garil_variant_burst.inc` at
frame 46 from model targetable 5's live world-position pointer. This follows
the same typed targetable mapping used by the existing single-enemy adapter;
it does not reinterpret host model or task memory using Saturn offsets.
Factory normalization uses the shared dot-product, square-root, division and
multiply services. Common RAM disassembly confirms 0603570E is the three-axis
MAC product shifted by 16; upstream sqrt_F is explicitly identified 060359AE.

The child retains all 31 visual records, 125 factory RNG calls, per-frame
ballistics and one shared Draw animation step. Damage uses the existing
applyDamageToDragon service; expiry writes the named dragon targetable m4C.
No contact gate, new battle engine, replacement resources or story flag is
introduced. The native hit-timer value 27 is preserved even though the first
update decrements 27 to 26 and therefore never matches that choice. Do not
silently turn this native outcome into guaranteed damage.

`tests/garil_variant_burst_test.py BTL_A5.PRG` compiles and executes the actual
production task/factory/launch code. It verifies every timing choice, exact
125-call RNG order and per-particle initial velocities, all 159 update ticks
to expiry, damage arguments/count, integer wrap, shared Draw stepping,
allocation failure and targetable 5 selection. It also checks native callback,
RNG, motion, damage and timer literals from the original PRG. Vita compilation,
linking and VPK packaging pass. This does not establish device acceptance.

Formation selection/release gates and actor position updates remain pending;
this checkpoint restores the effect implementation, not the whole encounter.
The earlier Lathum checkpoint remains the appropriate independent Vita test.
