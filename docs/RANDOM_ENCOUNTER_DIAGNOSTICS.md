# Random encounters and radar danger

Hardware feedback confirms Telepathy Shard ownership and only scripted Baldor
Queen encounters. Native item ID is 38 hex / 56 decimal, the same slot checked
by `fieldRadar_udpate`. Missing ownership must not be assumed from a blue orb.
The evaluator writes `m34_radarDangerLevel`; the radar consumes that value and
selects native palette/fade resources. No danger values or encounter chances
are overridden by this diagnostic change.

Earlier NativeEncounter observers used `PDS_Log`, a no-op in Lagi compatibility
builds. They could never establish whether the task was running. They now use
the platform logging sink, as do the table-selection and actual roll messages.
Global upstream task logging remains disabled. State messages are deduplicated
on field/subfield, enable/status, selector/row, rate/danger, movement, shard and
failure reason; timer ticks alone do not log each frame.

Read a fresh flight log for:

- `NativeEncounterTable`: native story-bit selection and overlay.
- `NativeEncounter`: reason 0 disabled, 1 no area, 2 sentinel row, 3 selected row;
  includes actual shard count, field status and movement gate.
- `NativeEncounterRoll`: original chance, roll and attempt counters.

The host fixture deliberately leaves PDS_Log as a no-op, verifies real sink
output and deduplication, and compares native area/probability/RNG/counter code
after removing observation calls. The battle Vita VPK compiles. None of these
checks proves random encounters or danger colors work on hardware; the new log
is needed to identify the runtime gate rather than guessing a forced battle.

## Separate audio findings

The unlocked-audio run contains `PCM field file=EV04_1.PCM (stream path pending)`
and subsequent EV04 voice requests. That service currently does not play files.
Upstream `playPCMFile` decodes signed big-endian 16-bit mono at 22050 Hz by
default. A faithful Vita service must mix it alongside SCSP, not replace its
shared output stream. Menu SFX routing and DSP scheduling remain separate
investigations; adding a DSP worker cannot implement this absent PCM path.
