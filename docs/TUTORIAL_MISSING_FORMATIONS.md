# Missing native Tutorial 1 formations

Goal 7 remains active. Movement lesson progression was corrected separately in
8600ece; hardware acceptance is pending. Lessons 2, 3 and 4 still have missing
formation dispatch/implementations and must not be reported playable.

## Original Disc 1 evidence

Read BTL_T0.PRG directly from the MODE1/2352 disc: LBA 262947, 392928 bytes,
base 06054000. Encounter entries start at 060A989C, stride 0x20; the first
pointer references `{callback, argument, flags}` records, terminated by null.

| Sub-battle | Formation callback | Initialization | Update | Native allocation |
| --- | --- | --- | --- | --- |
| 2 | 060586E0 | 0605863C | 06058740 | 0xF4 |
| 3 | 0605B00C | 0605AEBC | 0605B036 | inspect descriptor 060AB05C |
| 4 | 06055A54 | 0605594C, argument 1 | descriptor 060A9DEC | 0x100 |

These are different authored model-enemy controllers, not aliases for the
generic Urchin formation. Never suppress normal battle completion or insert a
dummy persistent task to hide their missing formations.

Sub-battle 2's initializer allocates NPC/file slot 0x14, sets the enemy turn
timer's m2 to 120, invokes 06067B28 with zero, and displays formation name 6,
mode 0, argument 9. Its entity group has two entries, zero dead count,
position block at native formation +0x2C and data table 060ACCC8. Each entity
is created through 060587F0; null creation increments the group's dead count.
The constructor sets state zero and callback +0xD8 to the empty 060586DC.

The 06058740 update increments the engine's running-formation count. On all
entities dead it sets formation state 7 and resets the enemy-turn fields m8,
m0, m2 and m4. It then executes position service 0609DE44 with spring 0xFF,
damping 0xFFF, followed by shared state-machine service 0609E68E. State 7
uses the original delayed task completion, not an immortal formation.

Entity 060587F0 uses native model descriptor 060AADA8, file slot 0x14,
model/targeting services 0609ED96 and 0609EE7C, animation and damage handling
0609F0CA. Its update 06058996 includes target-dependent jitter, hit/death state,
reward counters, and delayed group dead-count increment. These must be restored
before enabling the formation. Raw native work-area offsets cannot be applied
to C++ task pointers: task headers, pointers and vectors have different layout.

## Shared formation spring recovered

`include/lagi/native_formation_motion.h` recovers one axis of native
0609DE44..0609DF72 using named state, without a host/Saturn layout cast.
The original three axes are independent with the same recurrence:

1. Subtract target/current with 32-bit wrap.
2. Sign-extend bit 27 by OR F0000000 when set, otherwise AND 0FFFFFFF.
3. Add signed fixed-point spring force to accumulated acceleration.
4. Subtract fixed-point velocity damping.
5. Add acceleration to velocity, velocity to current, current to world center.
6. Clear accumulated acceleration.

All adds/subtractions wrap; multiplication returns product bits 16..47.
Negative products round down, not toward zero. No floating point, allocation,
clamp, guessed interpolation, rendering state or new GXM work is introduced.

The literal constants at 0609E0BC/C0/C4 prove the difference is signed **28**
bits, not 24. Position source fields are +0x18 velocity, +0x24 acceleration,
+0x30 target, +0x60 current, and +0x00 world. The reconstructed upstream
two-argument `formationUpdatePosition` is not a valid substitute for this
four-argument spring service; its differently documented entry also must not
be silently treated as a matching function solely because the names resemble.

`tests/native_formation_motion_test.ps1` compiles the production header and
compares 897775 boundary/random/trajectory cases against a separate scalar
rendering of the SH2 recurrence. It also checks Vita C++ syntax. This does not
execute SH2 or a Vita, and the primitive is not yet called by an enabled native
formation. It does not repair the last three lessons or establish hardware
performance. Next restore their typed entity/model/target lifecycle and bind
the native spring with the exact authored parameters.
