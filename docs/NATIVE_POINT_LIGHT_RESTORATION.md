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

Selector lifecycle now has generated-source restoration. Native `0601FE06`
pushes the previous selector and selects directional 6103; `0601FDF8` pushes
then selects A11D, the branch encoding from `0601FDD0`; `0601FE14` restores
and advances the pointer. The reconstructed directional push wrote stack[0]
without updating the pointer, and point activation/pop were stubs. The adapter
restores all three with bounded eight-slot assertions. Original field cleanup
`060741CC..06074290` calls pop under EC/EB, recomputes the ordinary directional
vector and restores its normal color through setupLight. This cleanup is also
restored, so activating point mode does not leave the stack growing each frame.

Actual production-generated selector functions pass 1,000 nested activation /
directional push / pop / reset lifecycles plus all eight stack slots on host.
Generated 3dEngine and fieldDragon Vita syntax pass. No upstream or game files
are edited. Renderer consumption remains unconnected; this is not a visible
point-light fix and awaits complete division and submission-path integration.
