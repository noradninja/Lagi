# Goal 3: transition presentation and cinematic idle service

Scope: saved-game black transition/hold, Valley-to-Excavation fade, in-engine
cinematic keep-awake. Azel owns simulation, fade endpoints and readiness;
Neptune presents them. Independent corrections remain atomic.

## Saved-game first correction

Latest hardware log: LoadTransition states 2/3 have RGB 0 on both channels;
state 4 reaches RGB -256 on A and B. CLOFEN=007F, CLOFSL=0000. This differs
from older captures with CLOFEN cleared; do not reuse those as current evidence.
Original MAIN 06019B5C..06019B72 requests both fade channels from zero to zero
in one step. Do not guess a 30-update interpolation into that original routine.

Current menu composition skips the scene color-offset pass entirely. It now
uses the existing native-register offset draw service after all menu layers,
only when shared_color_offset proves all seven native layer enables and a
common selected RGB offset. No CPU pixel pass, new fade clock, or unconditional
black cover is introduced; mixed layer offsets are not flattened.

This restores a missing presentation path, not proof of a complete load fix.
Next verify destruction/resource lifetime and black retention through gameplay
readiness, then assess whether a separate enclosing native fade was omitted.
Valley-to-Excavation remains unaudited/open. Hardware
video/log acceptance is required for the final transition behavior.

## In-engine cinematic keep-awake service

The shared EPK sCutsceneTask::Draw services the platform idle timer while
npcData0.mF0 is zero; the field camera-cutscene Update does likewise after
its missing-cutscene-data early exit. Both call one wall-clock-gated service,
at most once per second across active tasks, matching Cinepak's existing
sceKernelPowerTick(DEFAULT) policy. There is no persistent power lock, catch-up
loop, playback-time adjustment or synthetic input. A long gap requests one tick.
No call is made by idle gameplay frames once these task callbacks stop running.

PlaybackPowerTick host tests cover first call at zero, subsecond suppression,
exact interval, long gap, duplicate call and clock restart. Generated hook
locations and the full Vita build pass. A bounded first-use log is
[AzelCinematic] keepAwake=power-tick intervalUs=1000000 result=....
Hardware acceptance still requires an uninterrupted in-engine cinematic longer
than the device idle timeout, without controller activity, plus confirmation
that ordinary gameplay idle/suspend behavior remains normal afterward.

## Shared field/module exit ordering audit

The generated field.cpp exitCutsceneTaskUpdateSub0Sub1Sub0 task sets the field
transition bit, requests both fades using its authored endpoint and 20-update
duration, waits on native primary fade completion, then invokes field teardown.
It waits for replacement subfield data before releasing its transition bit.
tests/field_exit_fade_test.ps1 extracts and executes that actual generated task;
it verifies requests occur once, no teardown while fading, one teardown after
completion, and replacement-data wait. This verifies the reconstruction's
ordering, not original-CPU equivalence or that a particular Valley exit uses it.

The existing lagi_module_exit_fade.inc also gates destruction for source modes
1/2/3/4 going to cinematic/movie/town destination modes 0/1/2. Its production
test and generated before-deletion placement check pass. It intentionally
excludes standalone Continue mode 9 and ordinary field-to-field transitions.
Do not broaden that gate or add a Valley-specific timer without tracing the
actual destination path. Valley route mapping and video acceptance remain open.

## Valley-to-Excavation route mapped

Generated field_field_a3_o_fld_a3_1.cpp retains exit index 2's authored camera
cutscene 06091CC4. Its Update2 waits until m48_cutsceneTask is null; with story
bit 86 clear it calls fieldA3_1_checkExitsTaskUpdate2Sub1(6). When m35 permits
direct module changes, that helper requests status 06; otherwise it delegates
to the native tutorial/field-dispatch path and records m3D. A set story bit
also delegates through native dispatch. These gates are not overridden.

Original Disc 1 COMMON (LBA 182, base 00200000), table 00212EAC, contains
status06=(mode1,param19), status08=(mode1,param20), status53=(mode3,param2).
Thus Excavation's cinematic is distinct from the later status08 boss cinematic.
The direct status06 handoff is inside the existing source-mode3/destination-mode1
module fade gate. No additional Valley-specific fade rule is warranted by this
source/data audit. The delegated branch and hardware visual acceptance remain
open; the test run supplied so far does not capture a new status06 handoff.

## Current saved-game lifecycle evidence

The supplied load trace reaches A/B=-256 in state4 while mode9 still publishes
the save layout, then both channels report stopped before mode -1. Module state1
subsequently changes status4A to status53 and issues the existing 30-step black
request. This proves black is requested before overlay loading, not that it was
visible: the old menu renderer skipped offsets. fde6409 addresses that gap.
The runtime leaves the previous published frame in place while synchronous
loading blocks; field entry requests its native fade-in afterward. Verify the
new capture before adding another cover or changing native fade duration.

## Late tunnel fade: corrected diagnosis and timeline build

The 44.352-second 2026-10-24 16-09-19 recording confirms that the camera changes
before the blackout hides it. The accompanying log has next=06 followed by
the 30-update module fade. The user clarifies that black must be reached at
tunnel entry; proving a pre-teardown fade is insufficient acceptance.

Original A3 descriptor 06091CC4 points to dragon keys 06091BEC and camera keys
06091C4C, flags01. The camera table contains one 91-update row then a zero
sentinel. Original readiness query 0606AD1C..0606AD34 really does test the
field script camera-task pointer at +48 for null; it is not an early remaining-
frames threshold. Original shared Init0607501C and Update060750F6 do not request
a fade. Therefore the prior explanation of a missing earlier trigger remains
a hypothesis, not a demonstrated native omission. Camera teardown/restoration
and field-script dispatch must also be considered.

Bounded [FieldExitTimeline] diagnostics now record cameraStart (duration/flags),
cameraEnd (frames/flags/fadeStopped), and scriptFade (native requested duration),
each with field/subfield and a maximum of 16 events per observer per launch.
There are no per-frame logs or changed thresholds/timers. The production-body
comparison test strips the observer lines and reproduces all three original
function bodies exactly. Vita build passes. This build is diagnostic, not a
claimed correction of the late fade. Reproduce the same tunnel transition and
retain its log/video to establish ordering of the native events.

## Reference correction: native hard cut, not an earlier interpolated fade

The user's 8.277-second Saturn reference (2026-10-24 16-37-00) shows the tunnel
camera view is expected, followed by a cut to black after the dragon enters.
The user explicitly corrects the earlier request: remove the added fade here.
The diagnostic run confirms the camera lasts its authored 91 updates and ends
with no script fade active, then our module gate adds 30 updates. Thus the
earlier missing-fade hypothesis is superseded, not a reason to move the trigger.

The verified field-mode3/status50 -> status06 handoff now requests black on both
native fade controllers in one update, then retains the existing publish and
both-channel completion barrier before teardown. Camera keys, durations,
readiness query and status request point are unchanged. This is an explicit
authored-transition policy correction based on the reference, not a renderer
scene mask or a new countdown. Other module transitions retain their prior
policy, including the boss cinematic; no global field-to-cinematic cut rule.

The production gate fixture checks one-step requests, no restart, both-channel
completion and existing fade/priority/exclusion cases. The generated timeline
body comparison and Vita build pass. Hardware acceptance of the final cut and
black hold remains pending. Saved-load visual acceptance and keep-awake idle-
timeout confirmation are also still outstanding.
