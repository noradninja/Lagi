# Native world-ray recovery (Goal 7, in progress)

Original Disc 1 MAIN: LBA58, 253952 bytes, base06006000.
MAIN0602E136..0602E1A8 transforms two world endpoints, invokes fixed-width
projection0602DE68, clips the projected quad through0602D946, then emits via
unshaded packet helper0602D9BC. It does not select a desktop color-line fallback
based on CMDSRCA==0. The port's desktop helper must not be adopted wholesale.

Unshaded emission uses CTRL1002, PMOD480|mode, native palette/size/source,
no Gouraud write and the low32 product of endpoint depth and far reciprocal,
logically shifted16 for its packet bucket. Existing shaded battle-ray emission
is a separate service and must not be silently changed by this integration.

## Projected-quad clip component

`native_ray_clip.h` ports MAIN0602C7E6..0602CA98's fixed four-vertex clip
actions and MAIN0602D946's plane order. The action table retains all16 masks,
including native rejection of masks5/10/15, nearest inside-neighbor selection
for a single outside vertex, tie behavior and native edge-processing order.
Intersections change only X/Y, not the third component. This is not generic
polygon tessellation or a floating-point line substitute.

Fractions reproduce DIV0S, eight DIV1 steps, ROTCL, EXTU.B and SHLL8 after
the operands' wrapped SHLL8. There is no final signed correction: exact
negative/negative divisions retain the preceding bin (-1/-2 => 7F00).
Do not replace this with an ordinary ratio rounded to nearest or 16-bit
fixed-point division. Coordinate multiply/add retains signed32 wrap/truncation.

Host tests cover positive and negative fraction domains through denominator
2047, all16 masks for both axes/sides, exact action coordinates, preservation
of inside vertices/third components, rejection without mutation and sequential
four-plane clipping. ARM C++ syntax check passes. Evidence is an original
instruction audit plus numerical/action fixtures, not native-machine execution.

## Remaining integration requirements

This component is not connected to the renderer yet. Complete the native
near/far endpoint clip (0602D912), fixed-width projection handoff, unshaded
packet emission and renderer-side world-depth metadata. Preserve native
camera/input/animation ownership. Test the whole production call chain,
capacity and visibility failures, then full Vita link/package and hardware
beam/ray output/performance. Tutorial2 dispatch remains disabled until its
other shared dependencies and spline endpoint bounds are resolved as well.
