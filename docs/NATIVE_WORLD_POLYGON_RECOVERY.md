# Native untextured world polygon service

Goal 7 Tutorial2 dependency. No extern/Azel edits. The service is retained in
the Vita link audit, but the spark draw callbacks are not yet connected.

Original Disc1 FLD_A7.PRG (LBA244836, 217660 bytes, base06054000):
06069AD0 transforms four world vertices through the current matrix and calls
0606993C. The latter near-clips with MAIN0602C848, rejects if any resulting
vertex reaches far (0602C7C6), projects through FP_Div and nested fixed-point
multiply0603563C, then applies right/left/top/bottom fixed-quad clipping. A
negative sort-depth argument selects the first near-clipped vertex's Z.

MAIN0602CAF4 emits CTRL1004, PMOD0400|mode, color and signed16 screen XY with
negated Y. Its packet key is the low32-bit product of sort depth and far
reciprocal, logically shifted16. A7 supplies mode01C0, hence PMOD05C0: an
untextured mesh polygon, not a textured sprite or an ordinary alpha quad.

`lagiDisplayWorldPolygon` restores this shared service. Input world vertices
are not modified; only local view/projected arrays are clipped. Native XY,
material and packet-key calculations stay fixed point. Renderer-only inverse
depth follows the quantized screen-edge interpolation and is recorded through
the existing shared world-depth service; it does not decide native geometry.
No heap allocation, new GXM scene, texture or draw pass is introduced.

The existing native quad action table now supports the near-depth callback:
MAIN0602CA9C uses DIV0U and interpolates both X and Y before setting Z to the
near boundary. Existing signed screen clipping remains unchanged. Tests cover
all16 near masks with17 nonuniform-depth/neighbor variants, both-coordinate
interpolation, unchanged/rejected ownership and every far-rejection mask.
Existing world-ray and screen-ray fixtures pass as regression checks.

The production world-polygon wrapper fixture executes depth/screen clipping,
native scalar projection arithmetic, exact polygon packet fields, sort-key
overflow/default depth, input ownership, current-transform use, world-depth
metadata and command/packet capacity guards. Its transform service is mocked;
this is not a device geometry or performance claim. The full Vita VPK builds
and the ELF retains the service. `NATIVE_SOLID_MESH_MATERIAL.md` records the
subsequent shared GXM material correction. Both native spark draw callbacks
are restored in `NATIVE_BEAM_SPARK_RECOVERY.md`; device testing remains pending.

Follow-up `NATIVE_QUAD_CLIP_BOUNDS.md` corrects the visible-mask versus
guard-intersection boundary distinction in the shared screen clipping path.
