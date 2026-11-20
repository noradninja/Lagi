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

## Native draw audit

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

`NATIVE_WORLD_POLYGON_RECOVERY.md` now records the restored shared clipping,
projection and untextured packet service. Its production-wrapper fixture,
near-plane quad coverage, existing ray regressions and Vita link/package pass.
`NATIVE_SOLID_MESH_MATERIAL.md` records shared GXM command4/6 mesh and
mesh+half material support, exhaustive production-selector/coverage tests and
shader resource checks. These dependencies are now used by both restored
callbacks below; no visible ring or Tutorial2 execution is claimed yet.

## Both native draw callbacks integrated

The generated fieldBeamSpark source replaces its empty alternate draw and
Unimplemented normal draw with `lagi_field_beam_spark_draw.inc`, guarded by
the exact upstream source boundaries. Spawn/init/update remain the production
source with the previously corrected callback assignment. The historical
UpdateAlt symbol is retained to avoid an unrelated API rename, but executes
the native unrotated draw.

Both native paths start at local(radius,0,0), from cosine table20217E80 and
sine table20216E80. Step01000000 selects trig indices256 apart, emitting16
segments and closing at index0. Bottom vertex slots alternate0/1, top slots
3/2; consequently winding alternates, just as in the original packet sequence.
Do not rebuild all segments with a uniformly reordered four-corner layout.

The normal path starts an identity matrix, rotates by m10 around Y via
MAIN06035E68, then mC around X via06035DC8. The existing fixed-point matrix
services preserve the combined-product truncation before transforming each
local endpoint with06036038. Both paths add m0_pos with native32-bit wrap,
use radius1C and vertical extent20, and submit direct color2C/default depth-1
to the shared untextured mesh polygon service. m14 is not consumed by either
original draw. Neither path mutates the task, current matrix stack, lifetime,
color or velocity. The zero-angle callback does not build/rotate a matrix.

The production draw fixture runs the actual generated callbacks with original
CosSinTable/getSin/getCos, MTH_Mul and extracted identity/X/Y/transform function
bodies. The polygon emitter is mocked to inspect geometry. An independent
native coefficient oracle covers every4096 table index on both axes plus1024
wrapped cross-axis/radius/height/position cases, all16 emitted quads, alternating
slots, closing seam, original color/default depth, unmodified state/callbacks
and the20-frame authored lifecycle. The separate lifecycle fixture continues
to mock draws and verifies1/20/255-frame retirement. World-polygon/solid-mesh
fixtures cover the next service boundary separately; this is not a substitute
for integrated device rendering. Full Vita package and retained callbacks pass.

Tutorial2 remains disabled pending its other native services and spline bounds.
No hardware appearance/performance acceptance for the restored spark is claimed.
