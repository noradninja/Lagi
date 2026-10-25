# Shared field spline coefficient recovery

Goal 7 dependency, not Tutorial 2 hardware acceptance. No extern/Azel edits.

US Disc 1 FLD_A7.PRG LBA244836, length217660, base06054000 contains:

- allocation 0607712C: seven contiguous count-word arrays (28*count bytes);
- finalize 060772A8: cumulative distances, normalized parameters, three axes;
- coefficients 06077488..06077636;
- three-axis evaluator 06077638 and axis evaluator 060776E0..06077798.

The upstream coefficient builder is not equivalent to the executable. Native
stores interval slopes at scratch[i+1], computes y2[1] from slopes[2]-[1],
reuses scratch[i] for each tridiagonal diagonal, and divides every backward
substitution by diagonal[k]<<4. The upstream body uses different indices and
omits that backward divide. Merely compiling it would expose incorrect paths.

`lagi_field_spline_coefficients.inc` restores the native body through a guarded
CMake source transformation. Arithmetic shifts/subtractions retain 32-bit SH2
wrap before existing shared FP_Div/MTH_Mul operations. Workspace is 8*count
bytes and is released through the existing heap service. Allocation failure
retains the upstream early-return behavior. Native callers use six knots;
the builder's original valid domain requires at least three strictly ordered
parameters. No invented interpolation or renderer-side path solver is used.

Production-include host checks cover exact three-knot curvature (+/-512),
nonuniform affine paths, translation invariance, natural endpoint coefficients
and scratch ownership/allocation failure. These are numerical/invariant checks
plus manual original-instruction audit, not native-machine differential
execution or hardware beam/camera acceptance.

The shared fieldSpline.cpp service now compiles in the battle build. Its A7
dispatch remains disabled while other services and bounds are audited. In
particular the axis interval search advances to count-1 for t equal to the
last knot, then reads i+1; the native executable has the same search and the
beam passes 0x10000. Do not label this safe or silently change it based on a
textbook spline implementation. Investigate actual data/layout/caller behavior
and establish an explicit safe adapter boundary before enabling the route.

Still required: shared model-context/LCS callbacks, world ray helper, spark
and ring draw bodies, horizon integration, native field graph execution,
all dialogue topics/exit, hardware rendering and frame-time acceptance.
