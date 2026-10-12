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

## Submission/consumer integration

The earlier unconnected/overflow limitations above describe previous checkpoints.
The bridge now checks the live A11D selector, evaluates against the original
native view/model matrix and snapshots three signed model-light words per
submission. Both ordinary runtime and explicit environment context paths capture
it at the draw boundary. A separate `hasNativeModelLight` field prevents a second
world/view/model transform. Fresh meshes, cached static lighting and billboard
lighting consume that same native result while retaining native colors/falloff.
Directional submissions retain the existing path, and reset clears the point
flag. No gameplay sprite-depth, texture-filter or point-filter exceptions change.

Overflow resolution uses the original Hitachi SH7604 Hardware Manual, sections
10.3.3/10.4.2 (printed pages 293–294): with overflow interrupts disabled the
quotient is clamped by sign. Positive/negative zero-divisor cases and the positive
MIN/-1 overflow are modeled without C++ division UB. Source:
https://antime.kapsi.fi/sega/files/sh7604.pdf
The evaluator no longer leaves small nonzero deltas unresolved.

Actual extracted production capture and renderer consumer pass host execution
for selector activation/reset, original versus deliberately different world
matrix, zero distance and null matrix. Math tests add positive/negative small
nonzero deltas to 49,540 range/root cases. Bridge and renderer Vita syntax pass.
Eight bounded `[NativePointLight]` messages expose distance/divisor, outside /
overflow and signed result. No full link/VPK build, device appearance or timing
is claimed. Final hardware gate is local collection illumination and normal
lighting restoration on the same route; unfinished menus remain separate.

## Hardware rejection and scope investigation (2026-10-12)

User recording `2026-10-12 10-11-44.mp4` rejects this integration: collection
lighting affects the dragon/riders, not the environment, and does not match
the original colored light. Previous host/syntax passes do not establish a
working point-light restoration. Point lighting is the user's next priority.

Current source trace: dragon Draw activates A11D in DrawSub1, draws its models,
then DrawSub3 pops the selector and restores ordinary vector/color. Field
initialization in o_fld_a3 creates the dragon task before the visibility-grid
task; processTasks calls each task's Update/Draw before visiting later siblings.
The environment bridge captures the live selector, so later environment
submissions cannot inherit the dragon's point mode. This explains the scope
of the current implementation, but does not establish the intended native
environment scope or authorize moving restoration to a guessed frame boundary.

Read-only original disc inspection confirms FLD_A3 060741EA calls 0601E1C8
and 0607428C calls 0601DF14. MAIN 0601E1C8 invokes local pop 0601FE14 and
queues the slave-side routine 0601F098; 0601E200 invokes local parameter setup
0601FE32 and queues slave setup 0601F0B6. MAIN 0601DF14 sets the local light
and also queues the slave light command. The port collapses these two native
rendering paths into a single live submission capture. Next investigation must
follow original master/slave object submission and lighting command ordering,
including which path draws environment objects, before changing scope.

The native packed-color convention is not a generic RGB byte-order defect:
s_RGB8 packs member0 into the low byte and member2 into the high byte;
setLightVector_M reverses that packed word into its color array. Renderer
consumption reverses it back, matching the reconstructed ComputeColorFromNormal.
Actual original color computation still needs comparison; do not substitute
an item-specific tint or claim that byte-order reasoning proves visual fidelity.
