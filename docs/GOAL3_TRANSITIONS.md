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
