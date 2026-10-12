# Native point-light restoration — in progress

Current renderer integration remains directional-only. The portable evaluator
is a prerequisite, not a working point-light release or hardware result.

Original MAIN `0601FD52` subtracts view/model translation (0C/1C/2C) from
preprocessed point XYZ, performs wrapping 64-bit squared-distance accumulation,
then XTRCT retains bits 16..47. It compares that unsigned value with params[4]
(not a fixed 1000 threshold). Below it, `0601FD86..9E` performs a 16-step integer
square root and divides 00100000 by that root. Otherwise params[3] is the
unsigned cutoff AND signed division numerator; greater distance exits with
three zero output words. This corrects older notes treating initial executable
data 04000000/1000 as immutable runtime constants.

`0601FE32` preprocesses argument R7 into params[3]=R7<<12 and params[4]=
signed-low-word(R7>>8) squared. Field input 14000 therefore yields 14000000
and 19000. `0601FCDC` scales each delta by the quotient, then multiplies through
the native model-matrix columns and stores three signed 16-bit components.
The evaluator must run against the original view/model matrix, before the bridge
removes the view transform, and its model-space result must not be transformed
a second time by Neptune.

`include/lagi/lagi_point_light.h` preserves those non-overflowing operations.
Tests execute its production code for 49,540 distance/square-root/cutoff cases,
signed directions, model translation and coincident light/model. Host execution
and Vita syntax pass. DIVU overflow is explicitly reported. All-zero delta is
resolved as zero independent of quotient; other overflowing division inputs
remain unresolved pending exact SH7604 peripheral behavior. No guessed
inverse-square formula or guessed saturation is wired into rendering.

Remaining integration: verify selector push/pop/reset (`0601FDD0`, `0601FDF8`,
`0601FE14`); restore generated 3dEngine selector services; capture the original
matrix and parameters at submission; consume signed model-space light once in
all lit mesh paths; validate small nonzero/overflow cases; measure the final
Vita collection route and visually compare local illumination and reset.
The goal is not complete and this helper does not alter active rendering.
