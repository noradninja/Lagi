# Native beam-spark lifecycle and draw recovery

Goal 7 dependency. No extern/Azel edits. Tutorial 2 remains disabled.

Original Disc 1 FLD_A7.PRG: LBA244836, 217660 bytes, base06054000.
Init06069EC4..06069F14 copies the argument record and masks fade decrement
with7FFF. If the C/10 angle words are both zero, 06069F0C loads0606A400 and
06069F0E..10 writes it to task-header+0C, the DRAW callback. Update remains
06069F16. The port mistakenly replaced m_UpdateMethod with an empty function
named beamSpark_UpdateAlt, so zero-angle particles never aged or retired.

A guarded generated-source correction replaces that assignment with
m_DrawMethod. The legacy upstream alternate name remains until its body is
recovered, but it is explicitly a draw callback. fieldBeamSpark.cpp now
compiles in the battle build; link audit retains beamSpark_spawn and its
task definitions. This is not proof that the unfinished draw bodies render.

The production-generated-source fixture invokes the actual spawn/init/update
functions and checks zero/nonzero angle combinations, original record copy,
masked fade, wrapped scalar velocity/radius/color updates and exact retirement
at1/20/255 frames. Host scheduler/allocation services are mocked. Vita package
build and retained ELF check pass. No device rendering acceptance is claimed.

## Remaining visual service

The native alternate0606A400..0606A64A is a substantial draw routine, not
dead code. Both it and06069F58 form a ring of world-space quads using angle
steps01000000 and16 segments. The ordinary path constructs a local rotation
matrix before adding position; the zero-angle path omits those transforms.
Radius is1C, vertical extent20, direct color2C. Native06069AD0 transforms
the four vertices and calls the projected polygon service0606993C.

Recover the complete geometry and shared polygon clipping/emission contract
before replacing either draw body. Do not substitute a billboard sprite,
solid line, guessed particle texture, or leave the alternate initializer
connected to a no-op update to avoid the missing draw work. The remaining
callback and GXM untextured-world-quad path require tests and hardware visual
and frame-time acceptance. Other A7 services and spline bounds remain open.
