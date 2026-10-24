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
