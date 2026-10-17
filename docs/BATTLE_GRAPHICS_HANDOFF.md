# Native battle graphics handoff evidence

This is an implementation map and partial restoration, not a runtime pass.
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
semantics, not a raw copy of rotation state. The port uses filename identities
instead of native file IDs, and separately heap-owned bundles instead of
reused Saturn DRAM destinations. The shared-video-memory adaptation below
preserves that ownership without replacing existing bundles.

The 0x120 VDP2 block is the original 14-entry transfer pool plus two heads
(`14 * 0x14 + 8`). Original initialization `06032574` clears 14 entries at
`0604B4DC`, stride 0x14; `0603258C` sets both heads at `0604B4D4` to state +8
and clears the first entry's next pointer. This matches `initVdp2Var1` and
`initVdp2Var2`, establishing the mapping to `vdpVar1`, `vdpVar2`, `vdpVar3`.
The disc audit now checks those initialization anchors as well. It is not
the four menu scroll layers (those entries are only 0xA bytes each).

Restore copies/releases each present block. Light restoration calls
`0601DF14`, `0601DF8A`, `0601E200`, respectively matching `setupLight`,
`generateLightFalloffMap`, and `dragonFieldTaskDrawSub1Sub1`; use these native
operations to regenerate derived lighting, not just assign `lightSetup`.
The existing Dragon menu restoration uses the same three operations.

## Implemented graphics storage/return

`lagi_battle_graphics_vita.cpp` is compiled only with the native battle option.
The generated encounter source uses its typed capture, both restore sites,
and discard-on-delete. Capture failure unpauses the field/restores menu access
without starting loading. Repeated restore/discard is harmless because the
owner pointer is cleared. This does not enable encounters. The resident video
service now stages/reloads the captured file spans as described below.

Both complete VDP2 register buffers are saved; the pending pointer is rebuilt
from its actual buffer identity, not assumed to match the double-buffer index.
Transfer nodes and heads retain pointers into the permanent native pool, with
owner validation on capture. Projection command pointers are excluded from
the saved state: restored coordinates are written to the current command-list
headers rather than resurrecting the old list's pointers. CRAM, the original
VRAM spans, optional fade state and native lighting calls are restored.

`tests/battle_graphics_test.ps1` executes the production service body against
mock service types, checking both pending-buffer owners independently of the
double-buffer index, current versus recycled command headers, transfer graph,
light callback order, exact restored spans/colors, optional fade behavior,
invalid ownership and repeated restore/discard. This does not test native
task scheduling, heap failure, pointer lifetime on hardware or resource reload.
The full Vita VPK compiles against the real native types with retained A3 link
audit and fullres/dither ON. Build-boundary tests check generated restore/delete
hooks and the absence of this service in the normal configuration.

## Resident video-file restoration

The opt-in build records successful raw `loadFile` writes into permanent VDP1
RAM, VDP2 RAM and CRAM. The ledger stores filename, owner, destination range,
file size and source offset, not a duplicate of each file. Overlapping loads
replace only the overwritten spans, retaining correct source offsets for
remaining fragments. Failed loads, temporary heap buffers and bundle outputs
are not registered. Existing field bundles remain at their original addresses;
they are not reallocated or patched twice. Native CGB/SCB/PNB calls reach the
same file service, without item-, encounter- or filename-specific substitutes.

Capture copies this metadata. Post-battle state 5 first stages each unique file
and verifies file/range sizes before native sound-stack cleanup. Failed reads
leave memory untouched and the field paused for retry; bounded state-change
diagnostics identify the file or disc mismatch. A successful restore copies
the staged slices, restores the captured ledger, and signals the existing
texture-publication epoch. Explicit graphics/palette/color backups are then
restored on top, preserving their native final-state precedence. No reads or
per-frame file scans are added to normal flight rendering.

Existing Map, Books and Save/Load byte-backup scopes also preserve metadata
for their exact backed-up video-memory ranges. Nested scopes and overlapping
ranges are supported, and unrelated loads outside a scope remain registered.
These hooks are battle-build-only; they do not change normal menu byte copies.
Disc changes clear active ownership/scopes and reject old captured snapshots.

`tests/battle_resident_video_test.ps1` executes the real ledger and raw/bundle
file-service bodies with mocked disc bytes. It checks partial overwrites,
source offsets, staged failure/retry, exact file-size validation, nested and
overlapping menu ranges, heap/bundle exclusion, disc changes, and renderer
publication notification. The graphics test also checks that failed resource
staging prevents graphics/light restoration and preserves the backup owner.
Full battle and ordinary VPK builds pass with fullres/dither ON; generated-source
checks confirm prepare-before-audio, both return sites and save scope hooks.
These are not on-device encounter/return, audio, allocator or GPU tests.

The ledger covers raw file writes into permanent video memory, not arbitrary
decompressed/transformed writes or all native Saturn memory regions. Original
sound-memory reload exclusion is respected by not tracking sound memory at all.
Further live battle integration must audit any additional shared-resource path
it exposes, rather than registering arbitrary heap addresses with stale lifetime.

## Port requirements before entry is enabled

- Verify the implemented typed graphics restoration under native task/command
  scheduling, especially projection headers and pending transfer sources.
- Verify resident video reloads across real field/battle/menu transitions and
  audit any additional shared-resource writes exposed by live execution.
  Bundle pointers and raw VDP1 upload destinations are not interchangeable.
- Verify both wired return/game-over paths and teardown cleanup without
  double-free in actual execution, not only generated-source tests.
- Exercise allocation/read failures, repeated encounters, game over, normal
  rewards/return, and hardware field rendering after return. A retained A3
  link PASS does not cover any of these runtime requirements.
