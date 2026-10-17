# Native battle graphics handoff evidence

This is an implementation map, not a completed restoration or runtime pass.
Battle execution remains disabled. Original disc bytes are inspected read-only;
no executable, font, texture or other game resource is included here.

## Proven entry ownership

`moduleManager_Init` creates the battle manager alongside menu/field tasks.
Normal `encounterTaskUpdate` pauses its field task, saves graphics, runs loading
setup and calls `loadBattleOverlay` directly. Module-manager slot 4 belongs to
a different entry route. Tutorial debug loading is not the normal handoff.

## Original backup and restore

US Disc 1 `1ST_READ.PRG` has 253650 bytes at LBA 58 in the inspected MODE1/2352
image, load address `06006000`. Backup is `0602849A`, restore `06028668`.
These addresses are version-specific. `tools/battle_disc_handoff_audit.ps1`
checks the relevant literal references against the locally supplied image;
it does not write or extract game files. This is structural evidence, not a
complete disassembly verifier or proof that a port executes correctly.

The original allocates a task-owned 0x28-byte header, then separately allocates
the following blocks. Each allocation is null-checked.

| Header | Bytes | Source / purpose |
| --- | --- | --- |
| +00 | 0x50 | `0604FD00`, projection state |
| +04 | 0x260 | `0604B224`, VDP2 register controls |
| +08 | 0x50, optional | `0604B484`, fade controls; only with nonzero argument |
| +0C | 0x120 | `0604B4D4`, additional VDP2 state |
| +10 | 0x2C | `06052B2C`, light setup |
| +14 | 0x1000 | `25F00000`, CRAM |
| +18 | dynamic | Size `0600FFC4`, capture `0600FFDE`, restore `06010002` |
| +1C | 0x400 | `25E3E000`, VDP2 VRAM |
| +20 | 0x800, optional | `25E3C000` then `25E3E000`, 0x400 each |
| +24 | word | Back-screen color at pending BKTA address |
| +26 | word | Line color at pending LCTA address |

The dynamic block is **not a rotation-parameter snapshot**. `0600FFC4` walks
the linked list at `0604A780` and returns `(node count + 1) * 8`. `0600FFDE`
copies two words at node +4/+6 and its destination at +8 into 8-byte entries,
ending with a zero word. Restore skips destinations in `[25A0B000,25A80000)`
and invokes `06010184` for the others. That routine resolves a file record and
calls the load path (`0600FEE2`). This establishes resident-resource reload
semantics, not a raw copy of rotation state. Mapping original file IDs to the
port's filename/bundle ownership still needs implementation. The synchronous
Vita file service currently tracks completion, not resident destination ranges.

The 0x120 VDP2 block is consistent in size with the original 14-entry transfer
pool plus two heads (`14 * 0x14 + 8`). The current `vdpVar1`, `vdpVar2`, and
`vdpVar3` have that conceptual structure, but address-to-symbol identity needs
verification before wiring a typed snapshot. It is not the four menu scroll
layers (those reconstructed entries are only 0xA bytes each).

Restore copies/releases each present block. Light restoration calls
`0601DF14`, `0601DF8A`, `0601E200`, respectively matching `setupLight`,
`generateLightFalloffMap`, and `dragonFieldTaskDrawSub1Sub1`; use these native
operations to regenerate derived lighting, not just assign `lightSetup`.
The existing Dragon menu restoration uses the same three operations.

## Port requirements before entry is enabled

- Replace fixed-size raw copies with current typed state. Rebase pending VDP2
  register and transfer-list pointers to their live owners; preserve both
  register buffers and all current fields, not only Saturn byte counts.
- Establish projection-command pointer lifetime across command-list resets.
  Do not restore stale command pointers merely because the native size fits.
- Restore resident field resources overwritten by battle loads, with actual
  filename/destination ownership and error handling. Bundle pointers and raw
  VDP1 upload destinations are not interchangeable.
- Restore the verified CRAM/VRAM spans, pending transfer state and native light
  setup; optional fade/VRAM spans must respect the original argument.
- Wire both return/game-over paths and teardown cleanup without double-free.
  The current upstream encounter Delete does not release its backup.
- Exercise allocation/read failures, repeated encounters, game over, normal
  rewards/return, and hardware field rendering after return. A retained A3
  link PASS does not cover any of these runtime requirements.
