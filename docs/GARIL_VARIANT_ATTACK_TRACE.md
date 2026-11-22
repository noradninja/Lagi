# Garil variant attack trace - 22 November 2026

Current source checkpoint: ea2d32b. This is original-data evidence, not a
runtime fix or device acceptance. Lathum testing remains independent.

## Confirmed missing launch

In `BTL_A5_variant.cpp`, enemy Update state 4 advances animation and, at frame
0x2E (46), only plays sound 0x6D and enters state 5. Original BTL_A5.PRG
Update 06055086 instead performs the following at 06055286-060552D8:

- Reads the model hotpoint vector through model+0x54, entry offset 0x1F4,
  pointer at entry+8; copies XYZ to actor offsets 0x1D8/0x1DC/0x1E0.
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
service 060642F8 with damage 44, kind 2, impact dragon+0x8C, knockback
pointer dragon+8 and stack flag 0x0800 (plus a zero vector). There is no
sphere-contact test in this launch/timing path.

After the active flag switches, counter 0x12 uses another old-value negative
check. Expiry writes 0x1000 to dragon+0xD8 and finishes the child. Every update
also visits all 31 visual records through 06054680; deletion visits all 31
through 060546AA before releasing the quad. Those visual callbacks and the
math helpers remain to be traced. Do not replace this task with a homing
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
