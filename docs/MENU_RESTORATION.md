# Menu and save/load restoration

Branch: `feature/menu-save-load-restoration`, based on main `b6c7bfe`.

## Unavailable storage-device presentation

The original `1ST_READ.PRG` unavailable-device path at `060196BA` creates
a child task using the definition at COMMON.DAT `0020F790`. Its Init
(`060190BE`) retains layer/position configuration; Draw (`060190C8`)
selects the device position from `0020F770`, subtracts NBG0 scrolling,
and draws the covering sprite at `0020F724` with palette `0x500`.
The port previously skipped this task, leaving background device icons
visible. The generated-source adapter now restores that native task for
each unavailable device, without hiding or disabling Internal's three
save-file slots. Original disc files and `extern/Azel` remain unchanged.

Validation: patched native source passes Vita C++ syntax checking through
`tests/save_overwrite_test.ps1`; visual coverage and device behavior require
an on-device Save/Load/Continue check. Cursor movement/blinking is a separate
remaining investigation, not claimed fixed by this change.

Scope includes opening menu/Continue, load selection and transitions, native
in-game menu pages, system Save/Load/Return, visible slot selection and
confirmation/cancellation, and save persistence. The user reports saving a
file while navigating blind, so existing backend functionality must be
preserved. Do not overwrite/delete existing Vita saves during diagnostics.
The save adapter currently uses `ux0:data/lagi/save/0/PANDRA_3_01` through
`PANDRA_3_03`; preserve those files and any sidecars.

Initial source audit found that `decodeSaveMenuNbgPixel` ORed the CAOS=5
palette base into the palette address. Native `renderer_vdp2.cpp` uses
addition: `CAOS * 0x100 + (paladdr | dotColor)`. The adapter now follows that
rule. This is a specific color-address correction, not proof that all menu
rendering or functionality is restored. The palette model/source test
exercises all eight pattern palette fields and nontransparent dots, including
cases where the old OR differs; existing out-of-range handling is unchanged.

The shared 16-color CHSZ=1 decoder now also adds CAOS to the pattern/SCN
palette address, following the same native rule. The address test covers all
eight CAOS banks, sixteen pattern palette fields, eight supplemental palette
fields and fifteen nontransparent dots (15,360 cases). Status-menu SCN=12
does not itself overlap CAOS; this correction preserves those addresses while
fixing shared-decoder configurations whose supplemental palette bank does.
It is not evidence that the missing menu layers are fixed.

Save overwrite safety: the unfinished upstream progress task bypassed its
overwrite question and case 0 fell through even for empty slots. The generated
Vita adapter now returns after classifying a slot, presents a confirmation
message for an existing file, and waits for a fresh Confirm or Cancel edge.
Cancel (including simultaneous Confirm/Cancel) finishes with native result 1
without reaching saveData. Empty slots proceed without an overwrite question.
This uses the native message plane/input mapping and save state machine; it
does not claim to reconstruct the original Saturn yes/no cursor dialog.
Hardware gates include readable prompt text and parent-task recovery on cancel.
The parent now also handles native write-failure result 2 without asserting:
the failure message stays visible and slot selection remains usable for retry
or cancel. No disk-full or device-write failure has been exercised on Vita.

The System adapter now preserves its pending VDP2 register values, all four
layer scroll/animation states, active font and fades before opening Save/Load.
When that child finishes without a successful Load transition, these values
are restored before redrawing System. Register values are copied into the
current pending buffer, without resurrecting an old double-buffer pointer.
This complements the native child's VRAM/CRAM restoration; it does not replace
the outer native menu's scene backup. Successful Load still transitions to the
saved scene. Repeated Save/Cancel/Load/Cancel/System/Return needs Vita testing.

Remaining gates: verify save-layout detection from native registers, VDP2
map/font/scroll interpretation, visible VDP1 selectors, title-to-Continue
handoff, zero-world-submission menu frames, all in-game pages and return,
actual save write plus reload/Continue into the saved location, and failure/
cancel behavior. Source-only tests cannot establish these Vita gates.

## Full-scope source audit

The current hardware capture checkpoint is `1482528`; that is a partial fix,
not an all-menu-functionality release. These gaps remain in the pinned native
source and must not be mistaken for renderer failures or silently excluded:

| Requirement | Current authoritative source evidence | Remaining work |
| --- | --- | --- |
| Opening selection and Continue | Native title task and Continue dispatcher are present; Vita adapter checks save availability | Validate visible selection, disabled entries, cancel/reentry and successful saved-location handoff |
| Save/load selection and persistence | Native slot tasks and Vita persistence adapter present; confirmation and failure recovery now patched | Hardware readability, existing-save preservation, new-slot write, reload and restart persistence |
| Inventory browsing and item use | `menu/inventoryMenu.cpp` has browsing, but `useInventoryItem`, `inventoryMenuTaskDrawSub2` and `inventoryPrintBerserk` are unfinished; category 2 asserts | Restore original item-use/details/Berserk pipelines from native evidence, then validate actual state changes |
| Dragon menu | `menu_dragon.cpp` contains menu flow but several statistics/cursor/sound subtasks are unfinished | Audit and restore native subtask behavior, then verify navigation and changes |
| Enemy List | `createEnemyListMenuTask` in `mainMenuDebugTasks.cpp` only asserts and returns null | Port the actual native page; current CMake disabling is a bring-up safeguard, not fulfillment |
| Map | `createMapTask` in `mainMenuDebugTasks.cpp` only asserts and returns null | Port the actual native page; do not enable the null constructor or substitute the flight radar |
| System return | Adapter snapshots presentation around Save/Load | Verify repeated nested opens/cancels and return to the paused scene |

Do not remove the two stub safeguards before replacing their constructors.
No invented menu data or generic item-effect guesses establish native fidelity.
The user's requested all-functionality end state remains unachieved until these
paths and the hardware gates above have evidence of completion.

Inventory paging: the generated adapter now tests each absolute item index
against the populated count and array capacity before printing a page row.
The upstream `i < totalCount` test could expose stale tail entries on a partial
last page. Model/source checks cover counts 0..100 and pages -1..20. This is
an entry-bound correction only, not implementation of item use or Berserks.

Continue crash triage: a user reports crashing when entering the save-slot
list from Internal storage. The supplied older gameplay log does not capture
that failure, so its cause is not established. Raw slot-preview reads now
validate native checksum/version before metadata is consumed, and the location
display bounds its six-bit save value against the 32-entry location table.
Out-of-table locations get a display-only fallback; save bytes are not changed.
`[SavePlatform] read begin` records slot/size before IO. Obtain a fresh crash log
and matching dump before treating these safeguards as proof of a crash fix.
