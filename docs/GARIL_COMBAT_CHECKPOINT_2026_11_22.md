# Garil combat checkpoint - 22 November 2026

Runtime/package source checkpoint: 1246e94, diagnostics/garil-cliff-current.
Later documentation-only commits do not require another build.

The mixed encounter now has its distinct swarm/single groups, live position
bindings, native spacing, selectors, attack phases, camera and defeat-count
completion connected. Production boundary fixtures pass for both actor groups,
all signed retirement timers, and handoff to shared battle result selection.
Lathum initialization, draw, damage, emergence, turns, attacks and main Update/
death checks pass. Full Vita packaging passes; GPU-pass timing is OFF.
These are host/source results. No new device acceptance is claimed.

## One practical gameplay capture

Use the existing Camp save and normal progression. Enter Garil, let ordinary
enemies attempt attacks, attack them, finish and check results/field return.
If the mixed swarm/lead encounter appears, observe both groups attacking and
finish it. Record its actual encounter naturally; no forced story flags.

For Lathum, the required trigger is its visible field mesh: lock on and select
it, observe the short cutscene, then battle entry. Finish the fight and check
rewards once, field return and authored main-exit availability. A direct battle
launch does not verify this progression. Keep the normal log and gameplay
capture; exact Saturn/Vita viewpoints are unnecessary.

Accepted red-worm passage and camera behavior remain accepted. Slowdown,
remaining rendering/effect parity and full Gash dialogue timing remain open.

## Preserved package

Local outputs/garil-1246e94 contains Lagi.vpk, matching lagi/lagi.velf,
CMakeCache.txt, BUILD.json hashes and a patch recording pre-existing staged
files. Those staged files were preserved and were not added to our commits.
The package reflects the current worktree rather than a claimed pristine
checkout. The runtime adapters used here were already present in that tree.

The fixture service probes do not verify actual collision, dragon health,
projectile scheduling, complete reward tasks, field restoration or device
visual/performance behavior. These are the purpose of the gameplay capture.

## Current speech and Lathum entry evidence

This package includes the later native camera/shared-speech changes for the
first swarm introduction and horn warning, and the native voice-to-Vita raw
PCM bridge. Earlier f379948 packages exclude those fixes. Raw PCM progress
excludes queued audio, and completion waits for drain. Voice bridge and speech
regressions pass; listening and timing on device remain open.

The current field mound production fixture also passes original five-placement,
one-interactive-variant, script/fallback, animation/burst and field-re-entry
checks against FLD_A5.PRG. Host rendering services are mocked: this does not
confirm that the target mesh is visible/selectable on Vita. Preserve the normal
mesh selection -> cutscene -> battle check in the same gameplay capture.
