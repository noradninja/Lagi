# Menu and save/load restoration

Branch: `feature/menu-save-load-restoration`, based on main `b6c7bfe`.

## Unavailable storage-device presentation

Extended string glyph map: native `06025DA0..06025DB0` records the bottom-left
tile at `(x,y+1)`. Upstream incorrectly overwrote `(x,y)`, losing the top-left
reference. The generated adapter restores the missing Y increment. Tests
check the exact patch, 3969 non-overlapping 2x2 map placements and compile
the corrected native VDP2 source for Vita. Text clear/redraw behavior still
needs on-device verification; this is not a cursor-motion fix.

Cursor investigation: input-triggered `[MenuCursor]`/`[MenuCursor2]` logs now
record selected entry, blink counter or spinning frame/countdown, vsync count
and pressed buttons. No per-frame logging, animation speed changes or forced
pause bypass were added. The cursor syntax test applies all four production
patches to native source and compiles it for Vita. To distinguish controller
state from sprite presentation, capture Save/Load: enter Internal, wait a
second between Up/Down presses across all slots, cancel and reenter, then
compare the log with a short video. These diagnostics are not a cursor fix.

Key-item next boundary: `06016ADA` looks up an item in the 22-entry COMMON
table at `0020DDC8`; the inventory stores that index at task offset `0x18`.
`06018388` suppresses two text planes, finishes both cursor child tasks,
unloads the inventory font and enters state 4. Restoration must include the
native key-item screen and return sequence, not merely call a generic item
effect or enable an unfinished constructor.

Consumable healing: `0600FAB8` uses the nine-entry table at COMMON.DAT
`00208A20`. The adapter reads that table directly, refuses an empty inventory
or an effect that cannot benefit HP/BP, consumes one only on success, and caps
both resources. The menu refresh follows `06018420`: rebuild the owned list,
keep selection near the consumed row, disable an emptied category and return
to category selection. The old position is bounded if removal exhausts the
last row (memory-safety guard, not an invented effect). Tests model 5050 row
refresh cases and syntax-compile the actual patched native inventory source.
Hardware resource changes, quantity rendering, sounds, depleted-category
navigation and persistence remain unverified. Key-item callbacks remain
unfinished; this does not complete the overall menu goal.

Healing Berserk action: native table `06042E3C` contains IDs AA/AB/AC with
base heals 400/900/20000. The adapter follows `060073F8`'s fixed-point SPR
scaling and `06018634`'s full-HP, nonzero-heal and sufficient-BP gates. BP is
deducted only on success, HP capped at maximum, and sound selected by the
native dragon-level rule. It uses the same cost helper as the Berserk rows.
Patched inventory Vita syntax checking passes; actual HP/BP change, refusal,
sound and save persistence still require hardware tests. Consumable and
key-item handlers remain unfinished.

Inventory weapon confirmation: native `06018570` checks the item's context
flags before dispatch. Its category-3 branch at `0601869E` toggles the selected
weapon against `mA_weaponType`, plays sound 7, then redraws rows and details.
The generated adapter restores this branch, with a bounds guard on the
selected row. Other categories still reach the unfinished native-handler
boundary; this is not a generic item-effect replacement. Actual equip/
unequip state and gameplay gun power require hardware checks. Consumables,
key-item callbacks and healing Berserks remain to port.

Inventory details: native `06017F7A` is restored for ordinary descriptions,
Berserk descriptions/unlearned placeholders and the six class strings from
the pointer table at `0604329C`. Weapon details use the original `weaponTable`
and `06017F4C` modifier formula: rounded signed percentage of multiplier minus
one, with native encoded prefixes/suffix and widths. The adapter preserves
the `3,0x38,0x26,4` text region and second-row placement. Tests check exact
stub replacement, six weapon modifier cases and actual patched Vita syntax.
Descriptions, class text and percentage alignment need hardware verification;
actual item use remains unimplemented.

Numeric display: native `060258C0` uses encoded digits `0x8002..0x800B`,
minus `0x8041`, and padding glyph `0x8001`. The generated number printer
now writes the native 2x2 tile ordering to VRAM and the column-major
character map, advancing X by two per character. It handles zero, negative
values and signed 32-bit limits without signed negation overflow. Widths
above the native eleven-character buffer are bounded (actual inventory
callers use 2/3). Font tests cover 91 formatting models and compile the
actual patched VDP2 routine for Vita. This enables the underlying quantity/
BP-cost rendering path; visual glyph/palette correctness needs hardware
verification. Item use, details and other menu pages remain unfinished.

Berserk browsing: original `06017C4A` keeps the category's fixed 30-row
ordering, writing zero for unlearned abilities. The generated adapter now
replaces the category-2 assertion accordingly. Native `060181A0` row display
is restored with icon/name, context-dependent style and the `0x20D93C`
encoded suffix. BP cost follows the exact `06007378` fixed-point routine
already reconstructed in `battleCommandMenu.cpp`; a local helper avoids
linking that omitted battle task translation unit. Tests cover native patch
targets, 32 learned-entry patterns and patched inventory Vita syntax.
The number printer beneath this display has now been restored; inventory
details have also been restored; actual item use remains unfinished. This is not full Berserk/menu completion and
requires hardware visual/navigation checks.

Outer-menu lifecycle: original graphics task Delete at `0601579A` clears
the menu task pointer and tail-calls font cleanup by the menu font ID. The
reconstruction cleared the pointer but left font cleanup commented out.
The generated adapter restores that call. It runs when the graphics task
is deleted, not each time the retained menu is hidden. The font-list test
checks the exact source target; runtime task destruction and field/title
transitions remain hardware verification gates.

Font-switch text clearing: native `06025166` clears `width` 16-bit cells
per row, starting at `0x25E06000 + 2*(y*64+x)`, advancing `0x80` bytes per
row. The generated adapter now replaces the `clearVdp2TextMemoryRect` stub
with this operation. `setActiveFont`'s existing call clears 44 by 32 cells;
the model test checks that the remaining 20 columns and following rows stay
untouched. Patched native VDP2 syntax compilation passes. This restores text
tile clearing, not cursor animation, and requires visual hardware checks.

Inventory font cleanup: original Delete at `06018B4C` reads the font ID from
task offset `0xC` and tail-calls `060250E6` (literal `06018B8C`). The generated
inventory Delete now calls the restored ID-based cleanup rather than the
empty stub. The production replacement is checked and Vita syntax-compiled
by `tests/inventory_page_test.ps1`. Hardware entry/exit verification remains
pending; item usage and other unfinished inventory routines remain in scope.

Save file-selection cancellation now returns after hiding the spinning file
cursor and starting the slot close animation. Previously it fell through to
navigation and reassigned the visible cursor index in the same frame. This
matches the already-returning Load cancellation path and prevents that
specific ghost-cursor state. The literal patch is checked and compiled by
`tests/save_overwrite_test.ps1`. It does not prove the reported stationary
or non-blinking cursor fixed; that still needs runtime investigation.

Font restoration: the native font loader writes the new node into the old
tail's `pNext` at `06024FB0`, after initializing ownership flags at
`06024F88`. Upstream omitted both assignments. The generated Vita VDP2
adapter now restores these operations, allowing tail traversal to allocate
distinct font IDs and `setActiveFont` to find older fonts. The regression
test is `tests/menu_font_list_test.ps1` (source-target check, linked-list
model, and patched native Vita syntax compilation). This is not hardware
validation. Native font unloading now follows `0602502C`: release font data
only when ownership bit 0 is set, unlink/free non-base controls, restore the
previous font when releasing the active control, then reset character maps.
The current-font entry (`060250E0`) replaces `unloadFnt`'s stub; the ID-based
entry (`060250E6`) restores the missing call in Save/Load Delete (`06019BD4`).
Vita syntax checks cover the actual generated cleanup and Save/Delete call.
Repeated screen entry/exit, subtitle and field transitions still require
hardware testing. Cursor blinking/movement is not proven fixed.

The original `1ST_READ.PRG` unavailable-device path at `060196BA` creates
a child task using the definition at COMMON.DAT `0020F790`. Its Init
(`060190BE`) retains layer/position configuration; Draw (`060190C8`)
selects the device position from `0020F764` (the pointer stored at
`0020F774` in init data `0020F770`), subtracts NBG0 scrolling,
and draws the covering sprite at `0020F724` with palette `0x500`.
The port previously skipped this task, leaving background device icons
visible. The generated-source adapter now restores that native task for
each unavailable device, without hiding or disabling Internal's three
save-file slots. Original disc files and `extern/Azel` remain unchanged.

Validation: patched native source passes Vita C++ syntax checking through
`tests/save_overwrite_test.ps1`; visual coverage and device behavior require
an on-device Save/Load/Continue check. Cursor movement/blinking is a separate
remaining investigation, not claimed fixed by this change.

Hardware check of `9ecb699`: user reports loading works. Screenshots show
misplaced unavailable-device covers and incorrect text layering/overlap.
Read-only disc inspection identified the cover error: `0020F770` contains
pointers, not coordinates. The adapter now reads coordinates at `0020F764`.
This correction still requires device confirmation; text composition and
cursor animation remain unresolved.

Additional `9ecb699` hardware evidence: the user confirms inventory item
descriptions work, with the Hand Gun description visible in the supplied
screenshot. This validates that displayed case, not all item actions/pages.
Cancelling Save exposes Lagi's temporary text-only SAVE/LOAD/RETURN adapter;
that adapter is not the original System screen and must be replaced, not
accepted as native restoration. Original COMMON constructor table entry
`0020DCE8` points to `0601AE8A`, which constructs a `0x68`-byte task using
COMMON task definition `0020FDE0`. These addresses were read from the user's
disc image without modifying or copying assets into the repository.
The definition has Init `0601AC2C`, no Update, Draw `0601AD2E`, and Delete
`0601AE24`; these routines are the next implementation boundary.

User reports a brief malformed-tile frame when opening the in-game menu.
Source inspection finds menu ID is assigned in graphics-task state 0, while
the matching VDP2 setup runs in state 1 on a later frame. Scene publication
now enables the specialized status decoder only when native CHCN/CHSZ,
PNB/CNSM, palette offsets and map addresses match that decoder's layout.
This avoids interpreting the previous scene's VRAM as menu tiles. Save's
separate format detection remains unchanged. Vita syntax passes; the video
was supplied but not decoded frame-by-frame, and device retesting is needed.

Menu composition now separates NBG0/1 backing from the glyph plane. Native
setup assigns backing priorities 3/4, VDP1 decorations 5/6, and NBG3 text 7.
Previously glyphs were baked into backing before sprites were drawn, so
decorations covered text. The new sequence draws backing, sprites, then
scrolled glyphs. Backing has separately allocated/released GPU storage so
preparing glyphs cannot overwrite a queued background draw. Existing text
filtering and Save palette decoding are retained. Fullres/dither Vita syntax
and source guards pass; no full build, GPU execution or hardware acceptance
is claimed. This does not replace the temporary System task or prove its
cancel/return lifecycle fixed.

Separate user-reported flight issue: particles remain visible through world
geometry. Published VDP1 sprite rendering currently uses ALWAYS depth test,
disabled writes, and zero vertex Z. The bridge now also preserves each
command's original packet `m4_bucketTypes` key, independently of optional
desktop extended depth. Capture is a bounded linear packet scan into fixed
arrays, with command membership/alignment validation; no texture-specific
world/HUD guessing or heap allocation was added. At most 16 distinct flight
texture sources emit `[NativeSpriteDepth]` diagnostics. This is prerequisite
metadata/evidence, not an occlusion fix: key semantics and conversion to the
world depth convention must be established before enabling depth tests.
Runtime and fullres/dither renderer Vita syntax checks pass; hardware pending.

Latest hardware screenshots: layering is now reported correct, but external
device covers remain corrupted. Original `0601906C` writes PMOD `0xA0`
(literal `06019196`); the adapter incorrectly called the `0x80` menu-sprite
helper. It now uses `drawMenuSprite2`, preserving the native 8bpp format.
Coordinates remain `0020F764`; Save/Load syntax and helper source guard pass.
New cover graphics still need a hardware check.

The user also reports missing level/time details and a garbled load fade-out.
Reconstructed `saveScreenDisplaySlotData` prints only the player name;
`saveScreenDisplaySlotData2` remains unimplemented. Restore the native preview
fields and read their original save-layout offsets before enabling them.
Loading itself is hardware-confirmed, but fade-out/layout handoff is not.

User explicitly rejected the temporary SAVE/LOAD/RETURN screen. It has now
been removed from the System adapter. The settings entry is disabled while
the real Controls/Sound task is unported; direct task calls finish safely
without drawing substitute UI. Native Save/Load tasks and file persistence
remain present; the removed adapter is no longer a way to launch them.
The prior System snapshot/roundtrip notes above describe the removed code,
not current functionality. Full native System restoration remains required.

Stale slot labels after cancellation: native `060193AE` loads exit countdown
9, whereas reconstructed slot Draw set `m14=0`, jumping to hidden state
without moving/clearing text. The generated adapter restores 9 so existing
per-position text cleanup runs through the native slide-out. Patched Save
syntax and System removal source guards pass; hardware checks remain pending.

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

Hardware note (2026-10-11): user confirms Dragon ability screen displays, but
the dragon model is behind its UI. Inspect the renderer's menu backing / 3D
model / decorations / glyph composition order. This does not authorize undoing
the separately accepted gameplay sprite-depth fix, nor prove Dragon input or
ability state changes are complete.

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
| System settings | Temporary SAVE/LOAD/RETURN replacement was removed; native settings entry is disabled | Restore actual controls/sound task and return lifecycle; no substitute text menu |
| Books/key-item reader | Native data readers, ownership selection and index-input helper exist, but no complete page task | Connect native entry, page/image rendering, input and inventory return; verify on hardware |

Current-state clarification (2026-10-11): the table above is a requirements
checklist, not evidence that every named upstream stub is still unpatched.
Generated-source inventory patches restore several details, Berserk/weapon and
consumable handlers; actual device effects still need acceptance. Dragon source
currently includes statistics and morph-cursor implementations, so audit its
remaining behavior rather than assuming those routines are absent. User confirms
in-game save/load stations work. Save-slot names/time/rider level/rank received
source-faithful patches; their final visual acceptance remains pending. Load
transition diagnostics are present, but the reported garbled fade is unresolved.
The old System snapshot descriptions in this document are historical and no
longer describe current code.

Do not remove the two stub safeguards before replacing their constructors.
No invented menu data or generic item-effect guesses establish native fidelity.
The user's requested all-functionality end state remains unachieved until these
paths and the hardware gates above have evidence of completion.

Inventory paging: the generated adapter now tests each absolute item index
against the populated count and array capacity before printing a page row.
The upstream `i < totalCount` test could expose stale tail entries on a partial
last page. Model/source checks cover counts 0..100 and pages -1..20. This is
an entry-bound correction only, not implementation of item use or Berserks.

Continue crash triage: the supplied complete gzip-wrapped Vita dump's
embedded TTY reports a `std::array<std::string,32>::operator[]` bounds
assertion. The matching user build at `1482528` has the unguarded
`locationTable[miniData.m2_location]` access. That establishes this failure
path; it does not establish that every reported crash has the same cause.
Raw slot-preview reads now
validate native checksum/version before metadata is consumed, and the location
display bounds its six-bit save value against the 32-entry location table.
Out-of-table locations get a display-only fallback; save bytes are not changed.
`[SavePlatform] read begin` records slot/size before IO. Obtain a fresh crash log
and matching dump from the updated build before treating these safeguards
as hardware proof that the crash is resolved.
