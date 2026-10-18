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

## Hardware follow-up and LUT cache correction

The fresh log SHA256
`F4CAA46A72D0EB8DCDE107EDAFAFB49CC28E8F1DC667BAA83A4321E70B968B9B`
confirms `shard=1`, enabled native area 00207680, rate 8 and rowDanger/danger 1.
Native rolls 93,65,74,46 fail, then 7 triggers subbattle 4. The user identifies
the encounter order as Baldor Queen, Magnata/Gelata urchins, Baldor Queen.
All three have return markers. Random encounters are therefore hardware-proven
in this run; the blue-only radar is not evidence they are disabled.

The shared `findOrUploadVdp1UiTexture` returned descriptor hits without checking
live palette contents. Radar uses LUT mode 1 and updates the same 32-byte LUT
at m3C, while CMDCOLR/CMDSRCA/CMDSIZE remain unchanged. That left the GPU texture
with old colors. Cache entries now retain the 16 resolved RGB555 LUT colors,
including CRAM references. Changed colors rerun the existing decoder and update
the existing padded texture allocation; equal colors retain the cache hit.
No radar-specific palette selection, encounter changes or extra waits are added.
The prior frame is finished before this scene's in-place upload, consistent with
the renderer's existing synchronous frame boundary.

`ui_lut_palette_test.ps1` checks direct/CRAM changes, BE decoding, equality,
bounds and repeated padded in-place row updates, plus production routing order.
This does not prove on-device GPU cache visibility. The existing unimplemented
native palette-fade task also remains: this fixes stale colors, not a verified
smooth native fade. Pixel-data-only and other color-mode cache dependencies are
outside this targeted LUT freshness correction.

## Separate audio findings

The unlocked-audio run contains `PCM field file=EV04_1.PCM (stream path pending)`
and subsequent EV04 voice requests. That service currently does not play files.
Upstream `playPCMFile` decodes signed big-endian 16-bit mono at 22050 Hz by
default. A faithful Vita service must mix it alongside SCSP, not replace its
shared output stream. Menu SFX routing and DSP scheduling remain separate
investigations; adding a DSP worker cannot implement this absent PCM path.
