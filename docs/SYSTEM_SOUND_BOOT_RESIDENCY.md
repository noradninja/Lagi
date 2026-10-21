# Native system sound bootstrap residency

The device log starts with title sequence 75, then field/boss/cinematic banks.
It shows system sound IDs 0/1/2/3 starting data bank 0/control 1, but no load of
`SYS_SE.SEQ` or `SYS_SE.BIN`. COMMON sequence 0's native list (pointer at
002152D8) includes those files at 25A0B000 and 25A0B300, followed by initial
player/DSP resources. Title sequence 75 instead starts at the player list and
does not establish system bank 0. Thus correctly queued events alone cannot
prove audible system sound playback.

The SCSP worker now loads COMMON's complete boot list once after its residency
epoch reset and before processing sequence/control events. No hardcoded file
destination, guessed effect number, new channel or replacement sound is used.
The normal native sequence loader subsequently selects area map/mixer/effects
and marks transfers as before. Existing resident-file/overlap tracking avoids
reloading unchanged player data and handles later replacements.

Expected startup evidence is `SCSP loaded SYS_SE.SEQ`, `SCSP loaded SYS_SE.BIN`
and `native boot banks ready=1`. The direct-disc descriptor test verifies COMMON
ownership and placement before the worker event loop. The production loader's
existing residency/overlap fixture also passes. Audible menu movement/confirm,
target selection and weak-point toggles remain device acceptance, not a claim
based on command logs. Missing non-system streams or authored effects may have
separate causes.
