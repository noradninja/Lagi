# A3 waterfall and water-layer investigation

## Typed waterfall emission positions

The six native waterfall emission helpers used Saturn task byte offsets (+8
for the data pointer, +C for the small waterfall frame counter) on polymorphic
C++ work areas. Those offsets do not describe the host/Vita task layout.
The generated-source adapter now infers the concrete task type and accesses
`m8` / `mC_frameCounter`. The pinned Azel source is unchanged. Native positions,
velocities, quad resources, RNG ordering, visibility gates and audio are retained.

`tests/waterfall_position_test.ps1` extracts the generated helpers and exercises
all six with deliberately non-native C++ layouts, checking the emitted positions,
frame-counter lookup and 26 RNG draws. The host fixture and fullres Vita VPKs
(battle enabled and disabled, stochastic filtering and lookup enabled) pass.
This is not on-device visual validation.

## Still open

The particle-pool billboard draw currently ignores its authored scale and depth
sorting bias; the native A3 06078A2A path calls MAIN 0602E5CC with both arguments.
This must be restored independently of the task-position correction.

The Saturn reference also shows layered water coverage absent from Lagi.
Collision geometry and quad-debug screenshots alone do not identify whether the
missing visible contribution is model submission, VDP1 material processing or
VDP2 rotation-plane composition. No guessed water planes or blanket transparency
override have been introduced.
