# Green Oasis activation audit - 23 November 2026

Audited source: 6ee67a9 on diagnostics/garil-cliff-current. Original Disc 1
FLD_A5.PRG base 06054000, SHA256
3743201c4b369da9bd88377c315711be5fd18246ba6ac3d1d656048c026efeb4.
This is original instruction/data and current source evidence, not hardware
acceptance. The monolith rendering crash fix does not restore activation.

## Discovery is distinct from activation

The sandfall discovery callback 0605A236 disables its target, sets byte96 bit40,
starts script 4 with flag 5B9, and starts camera data 0608CFB4. The current
adapter also owns/decodes the camera target at 06099FD4 correctly. Script table
0609D4A8 entry4 is 0606044D. It contains dialogue, hide/restore-dragon,
fade and camera-retirement calls already handled by the shared script service.
It does not perform the later monolith activation.

## Missing central activation path

Original sky/monolith Init 0605ABD4 registers callback 0605B694: the callback
pointer literal is at 0605AD1C and is passed to field-target setup 06074A84.
The current a5SkyEntity_Init passes nullptr instead. There is no CMake adapter
for a5_skyEntity.cpp restoring that callback. The monolith can therefore be
rendered and registered without its authored activation behavior.

Native callback 0605B694 calls startFieldScript(5,5BA), sets byte96 bit20,
sets native F4=333, counter114=0, adds 16C16C to104, integrates10C/108,
changes Update to 0605AFAC, emits helper0605B900 and starts camera data0608C8AC
with callback0605B58A and live monolith-position anchor E8. Script-table entry5
is060604ED. Its short dialogue is only one part of the sequence.

Activation Update0605AFAC advances rotation/offsets, has original effect calls
0605B900/0605B9C8/0605BAE0/0605BB9C, and at counter>=120 retires the secondary
model, calls0605AEF4(reverseMode=0), corrects accumulated rotation, sets byte95
bit02, plays PCM70/system sound73, assigns F0 at114 and changes Update to
0605B1FC. The current port contains none of these activation-state functions.
Do not replace them by immediately setting the completion bit.

Native0605AEF4 builds a shared debris-scatter descriptor for hierarchy18/poseB50
and calls06085E46, then installs Draw0605AD7A. It is also called with mode1
when Init sees byte96 bit20 already set; that reload branch is an explicit
Unimplemented in current a5_skyEntity.cpp. Restoration must include both the
live activation sequence and returning to an already activated Oasis.

The settled Update0605B1FC retains the original sound countdown and debug-only
reset conditions; it must not keep running the pre-activation secondary-model
Draw on an uninitialized/retired model. Native callbacks/effect descriptors
need named host data and shared services, not Saturn offsets into C++ tasks.

## Connected downstream consumers

| Consumer | Current source behavior |
| --- | --- |
| Sandfall/weather | Both read byte95 bit02: sandfall switches to static geometry; weather stops updating/drawing. Sandfall Init handles byte96 bit40/bit20 and sets its persistent stopped bit on reload. |
| Trapped worm | Existing Oasis adapter rescue sets byte95 bit08; authored pit spawning checks that flag. |
| Segmented Baldor | Existing adapter completes its authored lifecycle, sets byte95 bit01 and byteA3 bit80. |
| Later story worm | Night proximity task checks byteA3 bit80 and byte96 bit80. Although upstream shows stubs, CMake replaces NPC creation/tracker/camera with shared story-worm services; do not mistake raw upstream stubs for missing generated behavior. |
| Field scripts | Discovery script4 and activation script5 are loaded from the original script table; shared VM handles dialogue, camera retirement, fades and hide/restore. No invented event script is needed. |

## Remaining work and acceptance

Restore the monolith callback, activation/settled Update and Draw, shared debris
binding, original effect producers and camera callback before calling activation
implemented. Verify original state/order/timers and save/re-entry using production
host fixtures. Then one Vita run must show normal target activation, dialogue,
transient effects, stopped sandfall/weather, restored player control, later
Baldor/story-worm progression and persistence after leaving/re-entering.

No implementation or hardware-success claim follows from this audit alone.
