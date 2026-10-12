# Native Map menu restoration — in progress

Map entry must remain disabled until its real task/render/return pipeline exists.
This work is distinct from fixed pickup target flags and shared item ownership.

## Verified original task and data

COMMON menu dispatch `0020DCD0` has Map constructor `06017B84` at `0020DCE4`.
The constructor creates task definition `0020E8F4`, size 40 hex. That definition
contains Init `060179CE`, no Update, Draw `06017A68`, Delete `06017B74`.
Init has ordinary status-menu entry versus standalone entry paths: the former
scrolls layers for 16 frames; the latter saves vsync and sets it to one. It
calls `06016D88` for page availability/current selection, `06016E64` for map
resources, and creates cursor definition `0020E8B4`. Delete restores vsync only
for the entry mode that changed it. Draw states 0..4 cover opening wait,
selection setup, native mode-specific input, closing and the final wait.

`06016C54` derives field context from actual field/dragon state, including
special field indices 0F/12 and subfield cases 0/6/9/17/1A. Its descriptor
lookup at `06016D36..46` reads field table `0020E848` (23 BE32 pointers), then
a subfield pointer and 12-byte descriptor. No descriptor is a valid null result,
not permission to fabricate a map from radar geometry. Descriptor +0/+4 are
authored SCB/PNB filenames; +8..B are original presentation bytes.

`lagiAzelReadFieldMapDescriptor` now reads that native hierarchy from the live
COMMON image, validating address ranges, filename termination and the caller's
authoritative subfield count. It returns borrowed filenames and the original
four presentation bytes without inventing their meaning. It does not replace
the context-selection computation or implement a page.

Host production-reader execution against the user's original read-only Disc 1
COMMON finds 13 available first-subfield descriptors across all 23 field slots.
Field 1/sub 0 resolves descriptor 0020E4B4: MAP_A20.SCB / MAP_A20.PNB with
bytes FF/12/04/1F. Field 2/sub 0 resolves 0020E4C0. Null table slots,
out-of-range field/subfield, null input and truncated image are rejected. Vita
syntax passes. No game assets or upstream sources are copied/modified.

## Remaining required work

Native loader service is now implemented as `lagiAzelLoadFieldMapResources`.
Read-only MAIN 06016E64..06016F0C confirms SCB sizes strictly greater than
4000 select VDP2 69000 (otherwise 6C000). PNB sizes at most 800 first fill
800 bytes with big-endian 0238 words; then the PNB loads into the caller's
pattern storage. The service preserves file order and native thresholds, adds
bounds/missing-file checks, and returns the selected graphics VRAM offset.
Failure does not promise rollback: the future task must restore backed-up
graphics if a load fails after mutation. No constructor is enabled yet.

Host execution tests the actual loader body with file/VRAM mocks across 20
threshold/fill combinations plus invalid capacity, missing graphics, oversized
graphics and failures of each load. Service Vita syntax passes. This does not
verify packaged asset lookup, rendered maps, task lifetime or device input.

Restore the actual setup/availability/context, resource loading and display,
cursor task, all native mode-specific input functions (`06017762`, `06017882`,
`06017964`), and return lifecycle before enabling the constructor. Preserve
both authored world-map and field-map choices and their native availability.
Use real SCB/PNB data, not an approximate flight-radar substitute. Device gates
are unowned/owned entry, current-area image, switching/navigation, cancel,
reentry and save/reload ownership. Source/host tests do not establish them.
