# Menu and save/load restoration

Branch: `feature/menu-save-load-restoration`, based on main `b6c7bfe`.

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

Remaining gates: verify save-layout detection from native registers, VDP2
map/font/scroll interpretation, visible VDP1 selectors, title-to-Continue
handoff, zero-world-submission menu frames, all in-game pages and return,
actual save write plus reload/Continue into the saved location, and failure/
cancel behavior. Source-only tests cannot establish these Vita gates.
