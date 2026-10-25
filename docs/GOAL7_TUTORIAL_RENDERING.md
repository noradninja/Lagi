# Goal 7 tutorial rendering checkpoint

Hardware report: first Tutorial 1 section runs, but weak markers overlay the
dialogue panel, backgrounds are ordered incorrectly, and frame rate is 20 FPS.
The fresh log enters BTL_T0.PRG, overlay 15/sub 0. Steady samples have about
33-35 ms render time, 22-25 ms GXM finish, cache-hit sprite textures and no
static rebuilds. These are observations, not proof of a single GPU cause.

## Native packet order

PDS.cpp flushVdp1Sub2/4/5 and flushVdp1Sub3 perform stable descending radix
sorting of the low 15 packet-key bits. Lagi captured those keys but published
commands in emission order. The shared publication boundary now performs the
same stable ordering; equal keys preserve emission order and commands without
packets remain zero-key overlays. No texture IDs or tutorial-specific sorting
rules are used. The host fixture covers all 65536 keys, stable ties and an
emission-order panel/marker/background example. Vita package build passes.

This fixes the missing VDP1 ordering contract, not every VDP2 composition
priority band. Hardware acceptance of the panel/markers remains outstanding.

## Remaining investigation

The tutorial RBG cache reports reason 5: OVPNR screen-over pattern mode is not
yet supported by decoded pages, so it takes the complete raw decoder. Add
general resource support without dropping screen-over semantics. The current
log does not isolate that shader's exact GPU cost. Keep VDP2 priorities,
normal-combat acceptance and quadrant visibility as separate checks.
