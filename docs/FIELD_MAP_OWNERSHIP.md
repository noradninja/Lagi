# Field map collection and menu availability

## Current map-only repair (2026-10-12)

On c01bc89 the user confirms ordinary collection works again; only map pickups
remain repeatable, with the correct uncollected particle presentation. The
production adapter now redirects **only** type1 map-update `m20` accesses to
its embedded target disable byte. Normal-box callback/status bytes and save
stations are untouched. Original FLD_A3 Update 0607CDC2 confirms native box
+20 is target +18; 0607CEF4 reads visibility byte +8D, and 0607CEFE checks
ownership using item word +88 plus F3. The reconstructed visibility test
incorrectly checked whether the item ID was zero; this is corrected to the
native nonzero visibility-byte test. Callback and delayed reward remain native.

`tests/map_pickup_lifecycle_test.ps1` executes the actual generated callback
and complete update with mocked matrix/collision/target services: collection
advances state 0 -> 1 -> 2, stops rotation, ownership disables registration,
and visibility disables registration without touching duplicate host bytes.
Ownership delivery is mocked: this does not prove real laser reward, model
rendering, area reentry, or save persistence. Actual map translation unit passes
Vita syntax. No VPK build or device test was performed here.

Hardware gate: collect a map once, confirm the collected presentation and no
repeat targeting, then verify reentry/save-reload. If ownership is still absent,
inspect actual target item/quantity and delayed laser reward rather than
granting inventory from a map-specific callback. Actual Map page remains
unfinished and disabled. The generic repair described below is historical
and rejected, not the current implementation.

## Hardware regression invalidates the flag-alias repair

2026-10-12: user reports adf29ee allows infinite item-box collection, permits
two map pickups, and displays map models opposite to the expected uncollected
particle model / collected non-particle model. The generic flag redirection is
removed from production for all three item-box types. The earlier explanation
below is a rejected source hypothesis, not a verified fix. Its host-tail test
did not execute the full callback/laser/model lifecycle and must not be treated
as proof of native flag aliasing or correct collection behavior.

User explicitly requests restoring and enabling the actual Map page, alongside
the map-specific pickup/model lifecycle. Finish its native task and presentation
before removing the disabled entry; do not enable the existing null constructor.

User reports map pickup can be collected repeatedly and Map menu cannot be
accessed. These have distinct current source causes, not one proven missing
inventory flag.

Native item identity is `mB2_fieldMap` (B2). Existing laser trail deletion
retains target item/quantity, calls receive-item presentation, then the shared
`mainGameState.addItemCount`. For key items, that method sets ownership at
F3+item (field map: 1A5). The map pickup update already checks this same flag
and intends to disable its LCS target. Save persistence carries the shared
bitfield, but a fresh on-device pickup/save/reload remains the acceptance gate.

The reconstructed item-box structure has an embedded target at native +8,
then separate `m20`/`m21` members. Original box +20/+21 are target +18/+19:
disable and callback/status flags. Writing the separate host members does not
affect `updateLCSTarget`, which reads `m18_diableFlags` and returns immediately
when bit 1 is set. This disconnect permits owned maps/opened boxes to continue
being offered as targets. The generated adapters for all three item-box types
now direct those accesses to the embedded target's actual fields. No item-ID
special case, additional inventory grant or invented pickup state is added.
Existing native rotation, ownership condition and targeting calls remain intact.

Host test executes the actual upstream item ownership methods and actual
generated map-update tail: initially targetable, reward sets bit 1A5, repeated
updates stop registrations, recreated pickup remains disabled, duplicate grants
stay boolean ownership. It checks the duplicate member bytes remain unchanged.
Actual generated normal-box/map/save-station translation units pass Vita syntax.
This is not runtime laser-reward, save/reload or station hardware execution.

Map entry remains disabled by the earlier bring-up guard because
`createMapTask` is still an assert/null native-page stub. Item ownership cannot
make that missing page work. Restore the original Map task/data/navigation and
return lifecycle before removing the guard; do not substitute the flight radar
or enable the null constructor. Books work remains pending separately.

Hardware gate: collect the map once; confirm it cannot be retargeted, including
after area reentry and save/reload; verify working save/load stations remain
unchanged. Map-menu accessibility is a separate unfinished implementation gate.
